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

#include <fmt/core.h>
#include <fmt/format.h>

#include "grace_dictionary.hpp"
#include "grace_exception.hpp"

static constexpr std::size_t s_InitialCapacity = 8;
static constexpr float s_GrowFactor = 0.75f;

namespace Grace
{
  GraceDictionary::GraceDictionary()
    : m_Size(0),
      m_Capacity(s_InitialCapacity),
      m_Data(m_Capacity),
      m_CellStates(m_Capacity, CellState::NeverUsed)
  {
    
  }

  GraceDictionary::GraceDictionary(const GraceDictionary& other)
    : m_Size(other.m_Size),
      m_Capacity(other.m_Capacity),
      m_Data(other.m_Data),
      m_CellStates(other.m_CellStates)
  {

  }

  GraceDictionary::GraceDictionary(GraceDictionary&& other)
    : m_Size(other.m_Size),
      m_Capacity(other.m_Capacity),
      m_Data(std::move(other.m_Data)),
      m_CellStates(std::move(other.m_CellStates))
  {
    other.m_Size = 0;
    other.m_Capacity = 0;
  }

  GraceDictionary::~GraceDictionary()
  {
    for (auto it : m_ActiveIterators) {
      it->Invalidate();
    }
  }

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
      if (m_CellStates[i] != CellState::Occupied) continue;
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
    return !m_Data.empty();
  } 

  GraceDictionary::IteratorType GraceDictionary::Begin()
  {
    GRACE_LOCK_OBJECT_MUTEX();

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

  void GraceDictionary::IncrementIterator(IteratorType& toIncrement) const
  {
    GRACE_ASSERT(toIncrement != m_Data.end(), "Iterator already at end");
    do {
      toIncrement++;
    } while (toIncrement != m_Data.end() && toIncrement->GetType() == VM::Value::Type::Null);
  }

  void GraceDictionary::Insert(VM::Value&& key, VM::Value&& value)
  {
    GRACE_LOCK_OBJECT_MUTEX();

    auto fullness = static_cast<float>(m_Size) / static_cast<float>(m_Capacity);
    if (fullness > s_GrowFactor) {
      m_Capacity *= 2;
      m_Data.resize(m_Capacity);
      m_CellStates.resize(m_Capacity, CellState::NeverUsed);
      Rehash();
      InvalidateIterators();
    }

    auto hash = m_Hasher(key);
    auto index = hash % m_Capacity;

    auto state = m_CellStates[index];
    switch (state) {
      case CellState::NeverUsed:
      case CellState::Tombstone:
        m_Data[index] = VM::Value::CreateObject<GraceKeyValuePair>(std::move(key), std::move(value));
        m_CellStates[index] = CellState::Occupied;
        m_Size++;
        return;
      case CellState::Occupied: {
        if (m_Data[index].GetObject()->GetAsKeyValuePair()->Key() == key) {
          m_Data[index] = VM::Value::CreateObject<GraceKeyValuePair>(std::move(key), std::move(value));
          m_CellStates[index] = CellState::Occupied;
          return;
        }
        for (auto i = index + 1; ; ++i) {
          if (i == m_Capacity) {
            i = 0;
          }
          if (m_CellStates[i] == CellState::Occupied) {
            if (m_Data[i].GetObject()->GetAsKeyValuePair()->Key() == key) {
              m_Data[index] = VM::Value::CreateObject<GraceKeyValuePair>(std::move(key), std::move(value));
              m_CellStates[index] = CellState::Occupied;
              return;
            }
          } else {
            m_Data[i] = VM::Value::CreateObject<GraceKeyValuePair>(std::move(key), std::move(value));
            m_CellStates[i] = CellState::Occupied;
            m_Size++;
            return;
          }
        }
      }
      default:
        GRACE_UNREACHABLE();
        break;
    }
  }

  VM::Value GraceDictionary::Get(const VM::Value& key)
  {
    GRACE_LOCK_OBJECT_MUTEX();

    auto hash = m_Hasher(key);
    auto index = hash % m_Capacity;
    while (true) {
      if (index >= m_Capacity) {
        index = 0;
      }
      switch (m_CellStates[index]) {
        case CellState::NeverUsed:
          throw GraceException(
            GraceException::Type::KeyNotFound,
            fmt::format("Dict did not contain key {}", key)
          );
        case CellState::Occupied: {
          auto kvp = m_Data[index].GetObject()->GetAsKeyValuePair();
          if (key == kvp->Key()) {
            return kvp->Value();
          }
          index++;
          break;
        }
        case CellState::Tombstone:
          index++;
          break;
        default:
          GRACE_UNREACHABLE();
          break;
      }
    }

    GRACE_UNREACHABLE();
    return VM::Value();
  }

  bool GraceDictionary::ContainsKey(const VM::Value& key)
  {
    GRACE_LOCK_OBJECT_MUTEX();

    auto hash = m_Hasher(key);
    auto index = hash % m_Capacity;
    while (true) {
      if (index >= m_Capacity) {
        index = 0;
      }
      switch (m_CellStates[index]) {
        case CellState::NeverUsed:
          return false;
        case CellState::Occupied:
          if (key != m_Data[index].GetObject()->GetAsKeyValuePair()->Key()) {
            index++;
            break;
          }
          return true;
        case CellState::Tombstone:
          index++;
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
    GRACE_LOCK_OBJECT_MUTEX();

    auto hash = m_Hasher(key);
    auto index = hash % m_Capacity;
    while (true) {
      if (index >= m_Capacity) {
        index = 0;
      }
      switch (m_CellStates[index]) {
        case CellState::NeverUsed:
          return false;
        case CellState::Occupied:
          if (key != m_Data[index].GetObject()->GetAsKeyValuePair()->Key()) {
            index++;
            break;
          }
          m_Data[index] = VM::Value();
          m_CellStates[index] = CellState::Tombstone;
          return true;
        case CellState::Tombstone:
          index++;
          break;
        default:
          GRACE_UNREACHABLE();
          break;
      }
    }

    GRACE_UNREACHABLE();
    return false;
  }

  std::vector<VM::Value> GraceDictionary::ToVector()
  {
    GRACE_LOCK_OBJECT_MUTEX();

    std::vector<VM::Value> res;
    res.reserve(m_Size);
    for (const auto& value : m_Data) {
      if (value.GetType() == VM::Value::Type::Null) continue;
      res.push_back(value);
    }
    return res;
  }

  GRACE_NODISCARD std::vector<GraceObject*> GraceDictionary::GetObjectMembers()
  {
    GRACE_LOCK_OBJECT_MUTEX();

    std::vector<GraceObject*> res;
    for (const auto& el : m_Data) {
      if (el.GetType() == VM::Value::Type::Null) continue;
      res.push_back(el.GetObject());
    }

    return res;
  }

  GRACE_NODISCARD bool GraceDictionary::AnyMemberMatches(const GraceObject* match)
  {
    GRACE_LOCK_OBJECT_MUTEX();

    for (const auto& el : m_Data) {
      if (el.GetObject()  == match) {
        return true;
      }
    }

    return false;
  }

  void GraceDictionary::RemoveMember(GraceObject* object)
  {
    GRACE_LOCK_OBJECT_MUTEX();

    for (std::size_t i = 0; i < m_Data.size(); i++) {
      auto& el = m_Data[i];
      if (el.GetType() == VM::Value::Type::Null) continue;

      if (el.GetObject() == object) {
        el = VM::Value::NullValue();
        m_CellStates[i] = CellState::Tombstone;
      }
    }
  }

  void GraceDictionary::Rehash()
  {
    auto pairs = ToVector();

    GRACE_LOCK_OBJECT_MUTEX();

    std::fill(m_Data.begin(), m_Data.end(), VM::Value());
    std::fill(m_CellStates.begin(), m_CellStates.end(), CellState::NeverUsed);

    for (auto& pair : pairs) {
      auto kvp = pair.GetObject()->GetAsKeyValuePair();
      const auto& key = kvp->Key();
      auto hash = m_Hasher(key);
      auto index = hash % m_Capacity;

      auto state = m_CellStates[index];
      if (state == CellState::NeverUsed) {
        m_Data[index] = std::move(pair);
        m_CellStates[index] = CellState::Occupied;
      } else {
        // if its not NeverUsed it will be Occupied
        // iterate until we find a NeverUsed
        for (auto i = index + 1; ; ++i) {
          if (i == m_Capacity) {
            i = 0;
          }
          if (m_CellStates[i] == CellState::NeverUsed) {
            m_Data[i] = std::move(pair);
            m_CellStates[i] = CellState::Occupied;
            break;
          }
        }
      }
    }
  }
} // namespace Grace
