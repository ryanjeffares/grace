/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceDictionary class, the underlying class for Dictionaries in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include "grace_dictionary.hpp"
#include "grace_keyvaluepair.hpp"

#include <fmt/core.h>

namespace Grace
{
  void GraceDictionary::DebugPrint() const
  {
    fmt::print("Dictionary: {}\n", ToString());
  }

  void GraceDictionary::Print(bool err) const
  {
    auto stream = err ? stderr : stdout;
    fmt::print(stream, "{}", ToString());
  }

  void GraceDictionary::PrintLn(bool err) const
  {
    auto stream = err ? stderr : stdout;
    fmt::print(stream, "{}\n", ToString());
  }

  std::string GraceDictionary::ToString() const
  {
    std::string res = "{";
    std::size_t count = 0;
    for (std::size_t i = 0; i < m_Capacity; ++i) {
      if (m_CellStates[i] != CellState::Occupied)
        continue;
      auto kvp = m_Data[i].GetObject()->GetAsKeyValuePair();
      res.append(kvp->ToString());
      if (count++ < m_Size - 1) {
        res.append(", ");
      }
    }
    res.append("}");
    return res;
  }

  bool GraceDictionary::AsBool() const
  {
    return m_Size > 0;
  }

  GraceDictionary::IteratorType GraceDictionary::Begin()
  {
    for (auto it = m_Data.begin(); it != m_Data.end(); ++it) {
      if (it->GetType() != VM::Value::Type::Null) {
        return it;
      }
    }
    return m_Data.end();
  }

  GraceDictionary::IteratorType GraceDictionary::End()
  {
    return m_Data.end();
  }

  void GraceDictionary::IncrementIterator(IteratorType& toIncrement)
  {
    GRACE_ASSERT(toIncrement != m_Data.end(), "Iterator already at end");
    do {
      toIncrement++;
    } while (toIncrement != m_Data.end() && toIncrement->GetType() == VM::Value::Type::Null);
  }

  void GraceDictionary::Insert(VM::Value&& key, VM::Value&& value)
  {
    auto fullness = static_cast<float>(m_Size) / static_cast<float>(m_Capacity);
    if (fullness > s_MaxLoad) {
      GrowAndRehash();
    }

    auto index = m_Hasher(key) % m_Capacity;

    while (true) {
      switch (m_CellStates[index]) {
        case CellState::NeverUsed:
        case CellState::Tombstone:
          // we can insert it here
          m_Data[index] = VM::Value::CreateObject<GraceKeyValuePair>(std::move(key), std::move(value));
          m_CellStates[index] = CellState::Occupied;
          m_Size++;
          return;
        case CellState::Occupied:
          if (m_Data[index].GetObject()->GetAsKeyValuePair()->Key() == key) {
            // error, use Update to update existing keys
            throw GraceException(
              GraceException::Type::DuplicateKey,
              fmt::format("{} was already present in the dictionary", key));
          }
          // keep going, might find a free slot
          index = (index + 1) % m_Capacity;
          break;
        default:
          GRACE_UNREACHABLE();
          break;
      }
    }

    GRACE_UNREACHABLE();
  }

  void GraceDictionary::Update(const VM::Value& key, VM::Value&& value)
  {
    auto fullness = static_cast<float>(m_Size) / static_cast<float>(m_Capacity);
    if (fullness > s_MaxLoad) {
      GrowAndRehash();
    }

    auto index = m_Hasher(key) % m_Capacity;

    while (true) {
      switch (m_CellStates[index]) {
        case CellState::NeverUsed:
        case CellState::Tombstone:
          // we can insert it here
          m_Data[index] = VM::Value::CreateObject<GraceKeyValuePair>(key, std::move(value));
          m_CellStates[index] = CellState::Occupied;
          m_Size++;
          return;
        case CellState::Occupied: {
          auto kvp = m_Data[index].GetObject()->GetAsKeyValuePair();
          if (kvp->Key() == key) {
            // update
            kvp->Value() = std::move(value);
            return;
          }
          // keep going, might find a free slot
          index = (index + 1) % m_Capacity;
          break;
        }
        default:
          GRACE_UNREACHABLE();
          break;
      }
    }

    GRACE_UNREACHABLE();
  }

  const VM::Value& GraceDictionary::Get(const VM::Value& key)
  {
    auto index = m_Hasher(key) % m_Capacity;

    while (true) {
      switch (m_CellStates[index]) {
        case CellState::NeverUsed:
          // nothing has been inserted here
          throw GraceException(
            GraceException::Type::KeyNotFound,
            fmt::format("Dict did not contain key {}", key));
        case CellState::Tombstone:
          // our desired value may have been inserted after this...
          index = (index + 1) % m_Capacity;
          break;
        case CellState::Occupied: {
          auto kvp = m_Data[index].GetObject()->GetAsKeyValuePair();
          if (kvp->Key() == key) {
            // found!
            return kvp->Value();
          }
          // keep going
          index = (index + 1) % m_Capacity;
          break;
        }
        default:
          GRACE_UNREACHABLE();
          break;
      }
    }

    GRACE_UNREACHABLE();
  }

  bool GraceDictionary::ContainsKey(const VM::Value& key)
  {
    auto index = m_Hasher(key) % m_Capacity;

    while (true) {
      switch (m_CellStates[index]) {
        case CellState::NeverUsed:
          // end of iteration - nothing has been inserted here
          return false;
        case CellState::Tombstone:
          // our desired value may have been inserted after this...
          index = (index + 1) % m_Capacity;
          break;
        case CellState::Occupied:
          if (m_Data[index].GetObject()->GetAsKeyValuePair()->Key() == key) {
            // found!
            return true;
          }
          // keep going
          index = (index + 1) % m_Capacity;
          break;
        default:
          GRACE_UNREACHABLE();
          break;
      }
    }

    GRACE_UNREACHABLE();
    return false;
  }

  bool GraceDictionary::Remove(const VM::Value& key)
  {
    auto index = m_Hasher(key) % m_Capacity;
    while (true) {
      switch (m_CellStates[index]) {
        case CellState::NeverUsed:
          // end of iteration - nothing has been inserted here
          return false;
        case CellState::Tombstone:
          // our desired value may have been inserted after this...
          index = (index + 1) % m_Capacity;
          break;
        case CellState::Occupied:
          if (m_Data[index].GetObject()->GetAsKeyValuePair()->Key() == key) {
            // found! remove
            m_Data[index] = VM::Value();
            m_CellStates[index] = CellState::Tombstone;
            return true;
          }
          // keep going
          index = (index + 1) % m_Capacity;
          break;
        default:
          GRACE_UNREACHABLE();
          break;
      }
    }

    GRACE_UNREACHABLE();
    return false;
  }

  GRACE_NODISCARD std::vector<GraceObject*> GraceDictionary::GetObjectMembers() const
  {
    std::vector<GraceObject*> res;
    for (const auto& el : m_Data) {
      if (el.GetType() == VM::Value::Type::Null)
        continue;
      res.push_back(el.GetObject());
    }

    return res;
  }

  GRACE_NODISCARD bool GraceDictionary::AnyMemberMatches(const GraceObject* match) const
  {
    return std::any_of(m_Data.begin(), m_Data.end(), [match](const VM::Value& value) {
      return value.GetObject() == match;
    });
  }

  void GraceDictionary::RemoveMember(GraceObject* object)
  {
    for (std::size_t i = 0; i < m_Data.size(); i++) {
      auto& el = m_Data[i];
      if (el.GetType() == VM::Value::Type::Null)
        continue;

      if (el.GetObject() == object) {
        el = VM::Value::NullValue();
        m_CellStates[i] = CellState::Tombstone;
      }
    }
  }

  void GraceDictionary::GrowAndRehash()
  {
    // keep a copy of current pairs
    auto pairs = ToVector();

    // grow arrays
    m_Capacity *= s_GrowFactor;
    m_Data.resize(m_Capacity, VM::Value());
    m_CellStates.resize(m_Capacity, CellState::NeverUsed);

    // refill them as all empty/never used
    std::fill(m_Data.begin(), m_Data.end(), VM::Value());
    std::fill(m_CellStates.begin(), m_CellStates.end(), CellState::NeverUsed);

    for (auto& pair : pairs) {
      auto kvp = pair.GetObject()->GetAsKeyValuePair();
      const auto& key = kvp->Key();
      // find the desired index of each key
      auto index = m_Hasher(key) % m_Capacity;

      while (true) {
        GRACE_ASSERT(m_CellStates[index] != CellState::Tombstone, "Shouldn't be any tombstones present during rehash");

        // if its never used, insert it
        if (m_CellStates[index] == CellState::NeverUsed) {
          m_CellStates[index] = CellState::Occupied;
          m_Data[index] = std::move(pair);
          break;
        }

        // otherwise keep going
        index = (index + 1) % m_Capacity;
      }
    }

    InvalidateIterators();
  }
} // namespace Grace
