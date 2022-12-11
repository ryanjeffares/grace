/*
 *  The Grace Programming Language.
 *
 *  This file contains out of line definitions for the GraceSet class, the underlying class for Sets in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include "grace_set.hpp"
#include "grace_list.hpp"
#include "grace_dictionary.hpp"

namespace Grace
{
  GraceSet::GraceSet(std::vector<VM::Value>&& data)
  {
    for (auto& value : data) {
      Add(std::move(value));
    }
  }

  GraceSet::GraceSet(VM::Value&& value)
  {
    switch (value.GetType()) {
      case VM::Value::Type::Bool:
      case VM::Value::Type::Char:
      case VM::Value::Type::Double:
      case VM::Value::Type::Int:
      case VM::Value::Type::Null:
        Add(std::move(value));
        break;
      case VM::Value::Type::String:
        for (auto c : value.Get<std::string>()) {
          Add(VM::Value(c));
        }
        break;
      case VM::Value::Type::Object:
        switch (value.GetObject()->ObjectType()) {
          case GraceObjectType::List: {
            auto list = value.GetObject()->GetAsList();
            for (std::size_t i = 0; i < list->Length(); i++) {
              auto v = (*list)[i];
              Add(std::move(v));
            }
            break;
          }
          case GraceObjectType::Dictionary: {
            auto vec = value.GetObject()->GetAsDictionary()->ToVector();
            for (auto& v : vec) {
              Add(std::move(v));
            }
            break;
          }
          case GraceObjectType::Exception:
          case GraceObjectType::KeyValuePair:
          case GraceObjectType::Instance:
            Add(std::move(value));
            break;
          case GraceObjectType::Set: {
            auto set = value.GetObject()->GetAsSet();
            m_Size = set->m_Size;
            m_Capacity = set->m_Capacity;
            m_Data = set->m_Data;
            m_CellStates = set->m_CellStates;
            break;
          }
          case GraceObjectType::Iterator:
          default:
            GRACE_UNREACHABLE();
            break;
        }
        break;
    }
  }

  void GraceSet::Add(VM::Value&& value)
  {
    auto fullness = static_cast<float>(m_Size) / static_cast<float>(m_Capacity);
    if (fullness > s_MaxLoad) {
      GrowAndRehash();
    }

    auto index = m_Hasher(value) % m_Capacity;
    
    while (true) {
      switch (m_CellStates[index]) {
        case CellState::NeverUsed:
        case CellState::Tombstone:
          // we can insert it here
          m_Data[index] = std::move(value);
          m_CellStates[index] = CellState::Occupied;
          m_Size++;
          return;
        case CellState::Occupied: {
          if (m_Data[index] == value) {            
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
  }

  bool GraceSet::Contains(const VM::Value& value) const
  {
    auto index = m_Hasher(value) % m_Capacity;
    while (true) {
      switch (m_CellStates[index]) {
        case CellState::NeverUsed:
          return false;
        case CellState::Tombstone:
          index = (index + 1) % m_Capacity;
          break;
        case CellState::Occupied:
          if (m_Data[index] == value) {
            return true;
          }
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

  void GraceSet::DebugPrint() const
  {
    fmt::print("Set: {}\n", ToString());
  }

  void GraceSet::Print(bool err) const
  {
    auto stream = err ? stderr : stdout;
    fmt::print(stream, "{}", ToString());
  }

  void GraceSet::PrintLn(bool err) const
  {
    auto stream = err ? stderr : stdout;
    fmt::print(stream, "{}\n", ToString());
  }

  std::string GraceSet::ToString() const
  {
    std::string res = "{";
    std::size_t count = 0;
    for (std::size_t i = 0; i < m_Data.size(); i++) {
      const auto& el = m_Data[i];
      if (el.GetType() == VM::Value::Type::Null) continue;

      switch (el.GetType()) {
        case VM::Value::Type::Char:
          res.push_back('\'');
          res.push_back(el.Get<char>());
          res.push_back('\'');
          break;
        case VM::Value::Type::String:
          res.push_back('"');
          res.append(el.Get<std::string>());
          res.push_back('"');
          break;
        case VM::Value::Type::Object: {
          auto object = el.GetObject();
          auto type = object->ObjectType();
          if (type == GraceObjectType::Exception || type == GraceObjectType::Iterator || type == GraceObjectType::Instance) {
            res.append(object->ToString());
          } else {
            std::vector<GraceObject*> visisted;
            if (AnyMemberMatchesRecursive(this, object, visisted)) {
              switch (type) {
                case GraceObjectType::Dictionary:
                case GraceObjectType::Set:
                  res.append("{...}");
                  break;
                case GraceObjectType::List:
                  res.append("[...]");
                  break;
                case GraceObjectType::KeyValuePair:
                  res.append("(...)");
                  break;
                default:
                  GRACE_UNREACHABLE();
                  break;
              }
            } else {
              res.append(object->ToString());
            }
          }
          break;
        }
        default:
          res.append(el.AsString());
          break;
      }
      if (count++ < m_Size - 1) {
        res.append(", ");
      }
    }
    res.push_back('}');
    return res;
  }

  GRACE_NODISCARD bool GraceSet::AsBool() const
  {
    return m_Size > 0;
  }

  GraceSet::IteratorType GraceSet::Begin()
  {
    for (auto it = m_Data.begin(); it != m_Data.end(); it++){
      if (it->GetType() != VM::Value::Type::Null) {
        return it;
      }
    }

    return m_Data.end();
  }

  GraceSet::IteratorType GraceSet::End()
  {
    return m_Data.end();
  }

  void GraceSet::IncrementIterator(IteratorType& toIncrement)
  {
    GRACE_ASSERT(toIncrement != m_Data.end(), "Iterator already at end");
    do {
      toIncrement++;
    } while (toIncrement != m_Data.end() && toIncrement->GetType() == VM::Value::Type::Null);
  }

  std::vector<GraceObject*> GraceSet::GetObjectMembers() const
  {
    std::vector<GraceObject*> res;
    for (const auto& el : m_Data) {
      if (el.GetType() != VM::Value::Type::Object) continue;
      res.push_back(el.GetObject());
    }

    return res;
  }

  bool GraceSet::AnyMemberMatches(const GraceObject* match) const
  {
    return std::any_of(m_Data.begin(), m_Data.end(), [match] (const VM::Value& value) {
      return value.GetObject() == match;
    });
  }

  bool GraceSet::OnlyReferenceIsSelf() const
  {
    return static_cast<std::uint32_t>(std::count_if(m_Data.begin(), m_Data.end(), [this](const VM::Value& value) {
      return value.GetObject() == this;
    })) == m_RefCount;
  }

  void GraceSet::RemoveMember(GraceObject* object)
  {
    for (std::size_t i = 0; i < m_Data.size(); i++) {
      auto& el = m_Data[i];
      if (el.GetType() != VM::Value::Type::Object) continue;

      if (el.GetObject() == object) {
        el = VM::Value::NullValue();
        m_CellStates[i] = CellState::Tombstone;
      }
    }
  }

  void GraceSet::GrowAndRehash()
  {
    auto vec = ToVector();

    // grow arrays
    m_Capacity *= s_GrowFactor;
    m_Data.resize(m_Capacity, VM::Value());
    m_CellStates.resize(m_Capacity, CellState::NeverUsed);

    std::fill(m_Data.begin(), m_Data.end(), VM::Value());
    std::fill(m_CellStates.begin(), m_CellStates.end(), CellState::NeverUsed);

    for (auto& value : vec) {
      auto index = m_Hasher(value) % m_Capacity;

      while (true) {
        GRACE_ASSERT(m_CellStates[index] != CellState::Tombstone, "Shouldn't be any tombstones present during rehash");

        // if its never used, insert it
        if (m_CellStates[index] == CellState::NeverUsed) {
          m_CellStates[index] = CellState::Occupied;
          m_Data[index] = std::move(value);
          break;
        }

        // otherwise keep going
        index = (index + 1) % m_Capacity;
      }
    }

    InvalidateIterators();
  }
} // namespace Grace