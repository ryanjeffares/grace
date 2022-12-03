//
// Created by Ryan Jeffares on 03/12/2022.
//

#include "grace_set.hpp"
#include "grace_list.hpp"
#include "grace_dictionary.hpp"

static constexpr std::size_t s_InitialCapacity = 8;
static constexpr float s_GrowFactor = 0.75f;

namespace Grace
{
  GraceSet::GraceSet()
    : GraceIterable{s_InitialCapacity}
    , m_Size{0}
    , m_Capacity{s_InitialCapacity}
    , m_CellStates{m_Capacity, CellState::NeverUsed}
  {

  }

  GraceSet::GraceSet(std::vector<VM::Value>&& data)
    : GraceIterable{s_InitialCapacity}
    , m_Size{0}
    , m_Capacity{s_InitialCapacity}
    , m_CellStates{m_Capacity, CellState::NeverUsed}
  {
    for (auto& value : data) {
      Add(std::move(value));
    }
  }

  GraceSet::GraceSet(VM::Value&& value)
    : GraceIterable{s_InitialCapacity}
    , m_Size{0}
    , m_Capacity{s_InitialCapacity}
    , m_CellStates{s_InitialCapacity, CellState::NeverUsed}
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
    if (fullness > s_GrowFactor) {
      m_Capacity *= 2;
      m_Data.resize(m_Capacity);
      m_CellStates.resize(m_Capacity, CellState::NeverUsed);
      Rehash();
      InvalidateIterators();
    }

    auto index = m_Hasher(value) % m_Capacity;
    auto state = m_CellStates[index];
    switch (state) {
      case CellState::NeverUsed:
      case CellState::Tombstone:
        m_Data[index] = std::forward<VM::Value>(value);
        m_CellStates[index] = CellState::Occupied;
        m_Size++;
        break;
      case CellState::Occupied: {
        if (m_Data[index] == value) {
          return;
        } else {
          for (auto i = index + 1; ; ++i) {
            if (i == m_Capacity) {
              i = 0;
            }

            if (m_CellStates[i] == CellState::Occupied) {
              if (m_Data[i] == value) {
                return;
              }
            } else {
              m_Data[i] = std::forward<VM::Value>(value);
              m_CellStates[i] = CellState::Occupied;
              m_Size++;
              return;
            }
          }
        }
      }
      default:
        GRACE_UNREACHABLE();
        break;
    }
  }

  bool GraceSet::Contains(const VM::Value& value) const
  {
    auto index = m_Hasher(value) % m_Capacity;
    while (true) {
      if (index >= m_Capacity) {
        index = 0;
      }

      switch (m_CellStates[index]) {
        case CellState::NeverUsed:
          return false;
        case CellState::Occupied:
          if (m_Data[index] != value) {
            index++;
            break;
          } else {
            return true;
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

    for (std::size_t i = 0; i < m_Capacity; i++) {
      if (m_CellStates[i] != CellState::Occupied) continue;

      res.append(m_Data[i].AsString());
      if (count++ < m_Size - 1) {
        res.append(", ");
      }
    }

    res.append("}");
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

  void GraceSet::IncrementIterator(IteratorType& toIncrement) const
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

  std::vector<VM::Value> GraceSet::ToVector() const
  {
    std::vector<VM::Value> res;
    res.reserve(m_Size);

    for (auto& value : m_Data) {
      if (value.GetType() != VM::Value::Type::Null) {
        res.push_back(value);
      }
    }

    return res;
  }

  void GraceSet::Rehash()
  {
    auto vec = ToVector();

    std::fill(m_Data.begin(), m_Data.end(), VM::Value());
    std::fill(m_CellStates.begin(), m_CellStates.end(), CellState::NeverUsed);

    for (auto& value : vec) {
      auto index = m_Hasher(value) % m_Capacity;

      auto state = m_CellStates[index];
      if (state == CellState::NeverUsed) {
        m_Data[index] = std::move(value);
        m_CellStates[index] = CellState::Occupied;
      } else {
        for (auto i = index + 1; ; i++) {
          if (i >= m_Capacity) {
            i = 0;
          }

          if (m_CellStates[i] == CellState::NeverUsed) {
            m_Data[i] = std::move(value);
            m_CellStates[i] = CellState::Occupied;
            break;
          }
        }
      }
    }
  }
} // namespace Grace