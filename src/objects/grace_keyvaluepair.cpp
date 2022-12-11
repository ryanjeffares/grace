/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceKeyValuePair class, the object held by a GraceDictionary
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <fmt/core.h>

#include "grace_keyvaluepair.hpp"

namespace Grace
{
  GraceKeyValuePair::GraceKeyValuePair(VM::Value key, VM::Value value)
      : m_Key {std::move(key)}
      , m_Value {std::move(value)}
  {
  }

  GraceKeyValuePair::GraceKeyValuePair(std::pair<VM::Value, VM::Value>&& pair)
      : m_Key {std::move(pair.first)}
      , m_Value {std::move(pair.second)}
  {
  }

  void GraceKeyValuePair::DebugPrint() const
  {
    fmt::print("KeyValuePair: {}\n", ToString());
  }

  void GraceKeyValuePair::Print(bool err) const
  {
    auto stream = err ? stderr : stdout;
    fmt::print(stream, "{}", ToString());
  }

  void GraceKeyValuePair::PrintLn(bool err) const
  {
    auto stream = err ? stderr : stdout;
    fmt::print(stream, "{}\n", ToString());
  }

  std::string GraceKeyValuePair::ToString() const
  {
    std::string res = "(";
    if (m_Key.GetType() == VM::Value::Type::String) {
      res.push_back('"');
      res.append(m_Key.AsString());
      res.push_back('"');
    } else if (m_Key.GetType() == VM::Value::Type::Char) {
      res.push_back('\'');
      res.append(m_Key.AsString());
      res.push_back('\'');
    } else if (m_Key.GetType() == VM::Value::Type::Object) {
      auto object = m_Key.GetObject();
      auto type = object->ObjectType();
      if (type == GraceObjectType::Exception || type == GraceObjectType::Iterator || type == GraceObjectType::Instance) {
        res.append(object->ToString());
      } else {
        std::vector<GraceObject*> visited;
        if (AnyMemberMatchesRecursive(this, object, visited)) {
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
    } else {
      res.append(m_Key.AsString());
    }

    res.append(": ");

    if (m_Value.GetType() == VM::Value::Type::String) {
      res.push_back('"');
      res.append(m_Value.AsString());
      res.push_back('"');
    } else if (m_Value.GetType() == VM::Value::Type::Char) {
      res.push_back('\'');
      res.append(m_Value.AsString());
      res.push_back('\'');
    } else if (m_Value.GetType() == VM::Value::Type::Object) {
      auto object = m_Value.GetObject();
      auto type = object->ObjectType();
      if (type == GraceObjectType::Exception || type == GraceObjectType::Iterator || type == GraceObjectType::Instance) {
        res.append(object->ToString());
      } else {
        std::vector<GraceObject*> visited;
        if (AnyMemberMatchesRecursive(this, object, visited)) {
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
    } else {
      res.append(m_Value.AsString());
    }

    res.push_back(')');
    return res;
  }

  bool GraceKeyValuePair::AsBool() const
  {
    return m_Key.AsBool() && m_Value.AsBool();
  }

  GRACE_NODISCARD bool GraceKeyValuePair::AnyMemberMatches(const GraceObject* match) const
  {
    return m_Key.GetObject() == match || m_Value.GetObject() == match;
  }

  GRACE_NODISCARD std::vector<GraceObject*> GraceKeyValuePair::GetObjectMembers() const
  {
    std::vector<GraceObject*> res;
    if (auto k = m_Key.GetObject()) {
      res.push_back(k);
    }
    if (auto v = m_Value.GetObject()) {
      res.push_back(v);
    }
    return res;
  }

  void GraceKeyValuePair::RemoveMember(GraceObject* object)
  {
    if (object == m_Key.GetObject()) {
      m_Key = VM::Value::NullValue();
    } else if (object == m_Value.GetObject()) {
      m_Value = VM::Value::NullValue();
    }
  }

  // Possibly will always return false, since KVPs are immutable/only the value can be changed if it's in a Dict...
  GRACE_NODISCARD bool GraceKeyValuePair::OnlyReferenceIsSelf() const
  {
    return m_Key.GetObject() == this && m_Value.GetObject() == this;
  }
}// namespace Grace
