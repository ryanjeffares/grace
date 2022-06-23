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
      auto kvp = dynamic_cast<GraceKeyValuePair*>(m_Data[i].GetObject());
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

  GraceDictionary::Iterator GraceDictionary::Begin()
  {
    for (auto it = m_Data.begin(); it != m_Data.end(); ++it) {
      if (it->GetType() != VM::Value::Type::Null) {
        return it;
      }
    }
    return m_Data.end();
  }

  GraceDictionary::Iterator GraceDictionary::End()
  {
    return m_Data.end();
  }

  void GraceDictionary::IncrementIterator(Iterator& toIncrement) const
  {
    do {
      toIncrement++;
    } while (toIncrement->GetType() == VM::Value::Type::Null && toIncrement != m_Data.end());
  }

  bool GraceDictionary::Insert(VM::Value&& key, VM::Value&& value)
  {
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
        return true;
      case CellState::Occupied: {
        if (dynamic_cast<GraceKeyValuePair*>(m_Data[index].GetObject())->Key() == key) {
          return false;
        }
        for (auto i = index + 1; ; ++i) {
          if (i == m_Capacity) {
            i = 0;
          }
          if (m_CellStates[i] == CellState::Occupied) {
            if (dynamic_cast<GraceKeyValuePair*>(m_Data[i].GetObject())->Key() == key) {
              return false;
            }
            continue;
          }
          m_Data[i] = VM::Value::CreateObject<GraceKeyValuePair>(std::move(key), std::move(value));
          m_CellStates[i] = CellState::Occupied;
          m_Size++;
          return true;
        }
      }
      default:
        GRACE_UNREACHABLE();
        return false;
    }
  }

  VM::Value GraceDictionary::Get(const VM::Value& key)
  {
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
        case CellState::Occupied:
          return m_Data[index];
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

  std::vector<VM::Value> GraceDictionary::ToVector() const
  {
    std::vector<VM::Value> res;
    res.reserve(m_Size);
    for (const auto& value : m_Data) {
      if (value.GetType() == VM::Value::Type::Null) continue;
      res.push_back(value);
    }
    return res;
  }

  void GraceDictionary::Rehash()
  {
    auto pairs = ToVector();
    std::fill(m_Data.begin(), m_Data.end(), VM::Value());
    std::fill(m_CellStates.begin(), m_CellStates.end(), CellState::NeverUsed);

    for (auto& pair : pairs) {
      auto kvp = dynamic_cast<GraceKeyValuePair*>(pair.GetObject());
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
