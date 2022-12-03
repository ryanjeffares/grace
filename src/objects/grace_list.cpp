/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceList class, the underlying C++ class for Lists in Grace
 *    
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <fmt/core.h>
#include <fmt/format.h>

#include "grace_list.hpp"
#include "grace_dictionary.hpp"
#include "object_tracker.hpp"

namespace Grace
{
  using namespace VM;

  GraceList::GraceList(std::vector<Value>&& items)
    : GraceIterable{std::move(items)}
  {

  }

  GraceList::GraceList(const GraceList& other)
    : GraceIterable{other.m_Data}
  {

  }

  GraceList::GraceList(const Value& value)
    : GraceIterable{0}
  {
    if (value.GetType() == Value::Type::String) {
      for (const auto c : value.Get<std::string>()) {
        m_Data.emplace_back(c);
      }
    } else if (value.GetObject() != nullptr) {
      if (auto dict = value.GetObject()->GetAsDictionary()) {
        m_Data = dict->ToVector();
      } else {
        m_Data.push_back(value);
      }
    } else {
      m_Data.push_back(value);
    }
  }

  GraceList::GraceList(const GraceList& other, std::int64_t multiple)
    : GraceIterable{0}
  {
    m_Data.reserve(other.m_Data.size() * multiple);
    for (std::size_t i = 0; i < static_cast<std::size_t>(multiple); i++) {
      m_Data.insert(m_Data.begin() + (i * other.m_Data.size()), other.m_Data.begin(), other.m_Data.end());
    }    
  }

  GraceList::GraceList(const Value& min, const Value& max, const Value& increment)
    : GraceIterable{0}
  {
    bool useDouble = min.GetType() == Value::Type::Double || max.GetType() == Value::Type::Double || increment.GetType() == Value::Type::Double;

    if (useDouble) {
      auto minVal = min.GetType() == Value::Type::Double ? min.Get<double>() : static_cast<double>(min.Get<std::int64_t>());
      auto maxVal = max.GetType() == Value::Type::Double ? max.Get<double>() : static_cast<double>(max.Get<std::int64_t>());
      auto incVal = increment.GetType() == Value::Type::Double ? increment.Get<double>() : static_cast<double>(increment.Get<std::int64_t>());
      auto capacity = maxVal / incVal;

      m_Data.reserve(static_cast<std::size_t>(capacity) + 1); // add 1 in case of rounding

      for (auto i = minVal; i < maxVal; i += incVal) {
        m_Data.emplace_back(i);
      }
    } else {
      auto minVal = min.Get<std::int64_t>();
      auto maxVal = max.Get<std::int64_t>();
      auto incVal = increment.Get<std::int64_t>();
      auto capacity = maxVal / incVal;

      m_Data.reserve(capacity);

      for (auto i = minVal; i < maxVal; i += incVal) {
        m_Data.emplace_back(i);
      }
    }
  }

  GraceList::~GraceList()
  {
    for (auto it : m_ActiveIterators) {
      it->Invalidate();
    }
  }

  void GraceList::Append(VM::Value&& value)
  {
    m_Data.push_back(std::forward<VM::Value>(value));
    InvalidateIterators();
  }

  void GraceList::Insert(VM::Value&& value, std::size_t index)
  {
    if (index >= m_Data.size()) {
      throw GraceException(
        GraceException::Type::IndexOutOfRange,
        fmt::format("The index is {} but the length is {}", index, m_Data.size())
      );
    }

    m_Data.insert(m_Data.begin() + index, std::forward<VM::Value>(value));
    InvalidateIterators();
  }

  void GraceList::Append(const std::vector<Value>& items)
  {
    m_Data.reserve(items.size());
    m_Data.insert(m_Data.end(), items.begin(), items.end());
    InvalidateIterators();
  }

  VM::Value GraceList::Remove(std::size_t index)
  {
    if (index >= m_Data.size()) {
      throw GraceException(
        GraceException::Type::IndexOutOfRange,
        fmt::format("The index is {} but the length is {}", index, m_Data.size())
      );
    }

    auto res = std::move(m_Data[index]);
    m_Data.erase(m_Data.begin() + index);
    InvalidateIterators();
    return res;
  }

  VM::Value GraceList::Pop()
  {
    InvalidateIterators();
    auto res = std::move(m_Data.back());
    m_Data.pop_back();
    return res;
  }

  void GraceList::Sort()
  {
    std::sort(m_Data.begin(), m_Data.end());
    InvalidateIterators();
  }

  void GraceList::SortDescending()
  {
    std::sort(m_Data.begin(), m_Data.end(), std::greater<Value>());
    InvalidateIterators();
  }

  void GraceList::DebugPrint() const
  {
    fmt::print("GraceList: {}\n", ToString());
  }

  void GraceList::Print(bool err) const
  {
    auto stream = err ? stderr : stdout;
    fmt::print(stream, "{}", ToString());
  }

  void GraceList::PrintLn(bool err) const
  {
    auto stream = err ? stderr : stdout;
    fmt::print(stream, "{}\n", ToString());
  }

  std::string GraceList::ToString() const
  {
    std::string res = "[";
    for (std::size_t i = 0; i < m_Data.size(); i++) {
      const auto& el = m_Data[i];
      switch (el.GetType()) {
        case Value::Type::Char:
          res.push_back('\'');
          res.push_back(el.Get<char>());
          res.push_back('\'');
          break;
        case Value::Type::String:
          res.push_back('"');
          res.append(el.Get<std::string>());
          res.push_back('"');
          break;
        case Value::Type::Object: {
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
      if (i < m_Data.size() - 1) {
        res.append(", ");
      }
    }
    res.push_back(']');
    return res;
  }

  bool GraceList::AsBool() const
  {
    return !m_Data.empty();
  }

  GRACE_NODISCARD bool Grace::GraceList::AnyMemberMatches(const GraceObject* match) const
  {
    return std::any_of(m_Data.begin(), m_Data.end(), [match] (const Value& value) {
      return value.GetObject() == match;
    });
  }

  GRACE_NODISCARD std::vector<GraceObject*> GraceList::GetObjectMembers() const
  {
    std::vector<GraceObject*> res;
    for (const auto& el : m_Data) {
      if (auto obj = el.GetObject()) {
        res.push_back(obj);
      }
    }

    return res;
  }

  void GraceList::RemoveMember(GraceObject* object)
  {
    // don't call the other Remove function or CleanCycles since this should ONLY be called while we are cleaning cycles
    auto it = std::find_if(m_Data.begin(), m_Data.end(), [object](const Value& value) {
      return object == value.GetObject();
    });

    if (it != m_Data.end()) {
      m_Data.erase(it);
    }
  }

  GRACE_NODISCARD bool GraceList::OnlyReferenceIsSelf() const
  {
    return static_cast<std::uint32_t>(std::count_if(m_Data.begin(), m_Data.end(), [this] (const Value& value) {
      return value.GetObject() == this;
    })) == m_RefCount;
  }
}
