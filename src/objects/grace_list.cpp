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
    : m_Data(std::move(items))
  {

  }

  GraceList::GraceList(const Value& value)
  {
    if (value.GetType() == Value::Type::String) {
      for (const auto c : value.Get<std::string>()) {
        m_Data.emplace_back(c);
      }
    } else if (auto dict = dynamic_cast<GraceDictionary*>(value.GetObject())) {
      m_Data = dict->ToVector();
    } else {
      m_Data.push_back(value);
    }
  }

  GraceList::GraceList(const GraceList& other, std::int64_t multiple)
  {
    m_Data.reserve(other.m_Data.size() * multiple);
    for (std::size_t i = 0; i < static_cast<std::size_t>(multiple); i++) {
      m_Data.insert(m_Data.begin() + (i * other.m_Data.size()), other.m_Data.begin(), other.m_Data.end());
    }    
  }

  GraceList::GraceList(const Value& min, const Value& max, const Value& increment)
  {
    auto useDouble = false;
    if (min.GetType() == Value::Type::Double || max.GetType() == Value::Type::Double
      || increment.GetType() == Value::Type::Double) {
      useDouble = true;
    }

    if (useDouble) {
      auto minVal = min.GetType() == Value::Type::Double ? min.Get<double>() : static_cast<double>(min.Get<std::int64_t>());
      auto maxVal = max.GetType() == Value::Type::Double ? max.Get<double>() : static_cast<double>(max.Get<std::int64_t>());
      auto incVal = increment.GetType() == Value::Type::Double ? increment.Get<double>() : static_cast<double>(increment.Get<std::int64_t>());

      for (auto i = minVal; i < maxVal; i += incVal) {
        m_Data.emplace_back(i);
      }
    }
    else {
      for (auto i = min.Get<std::int64_t>(); i < max.Get<std::int64_t>(); i += increment.Get<std::int64_t>()) {
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

  void GraceList::Append(const std::vector<Value>& items)
  {
    GRACE_LOCK_OBJECT_MUTEX();
    m_Data.reserve(items.size());
    m_Data.insert(m_Data.end(), items.begin(), items.end());
    InvalidateIterators();
  }

  void GraceList::Remove(std::size_t index)
  {
    GRACE_LOCK_OBJECT_MUTEX();
    m_Data.erase(m_Data.begin() + index);
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

  GRACE_NODISCARD bool Grace::GraceList::AnyMemberMatches(const GraceObject* match)
  {
    GRACE_LOCK_OBJECT_MUTEX();

    for (const auto& el : m_Data) {
      if (el.GetObject() == match) {
        return true;
      }
    }

    return false;
  }

  GRACE_NODISCARD std::vector<GraceObject*> GraceList::GetObjectMembers()
  {
    GRACE_LOCK_OBJECT_MUTEX();

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
    GRACE_LOCK_OBJECT_MUTEX();

    // don't call the other Remove function or CleanCycles since this should ONLY be called while we are cleaning cycles
    auto it = std::find_if(m_Data.begin(), m_Data.end(),
      [object](const Value& value) {
        return object == value.GetObject();
      });

    if (it != m_Data.end()) {
      m_Data.erase(it);
    }
  }
}