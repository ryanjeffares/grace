/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the Value class, which represents all values in Grace. 
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <type_traits>

#include "grace.hpp"
#include "objects/grace_list.hpp"
#include "value.hpp"

using namespace Grace::VM;

Value::Value()
{
  m_Type = Type::Null;
  m_Data.m_Null = nullptr;
}

Value::Value(const Value& other)
{
  m_Type = other.m_Type;
  if (other.m_Type == Type::String) {
    m_Data.m_Str = new std::string(*other.m_Data.m_Str);
  } else if (other.m_Type == Type::Object) {
    m_Data.m_Object = other.m_Data.m_Object;
    m_Data.m_Object->IncreaseRef();
  } else {
    m_Data = other.m_Data;
  }
}

Value::Value(Value&& other) GRACE_NOEXCEPT
{
  m_Type = other.m_Type;
  m_Data = other.m_Data;
  
  if (other.m_Type == Type::String || other.m_Type == Type::Object) {
    other.m_Data.m_Null = nullptr;
    other.m_Type = Type::Null;
  }
}

Value::~Value()
{
  if (m_Type == Type::String && m_Data.m_Str != nullptr) {
    delete m_Data.m_Str;
  } 
  if (m_Type == Type::Object) {
    if (m_Data.m_Object->DecreaseRef() == 0) {
#ifdef GRACE_DEBUG
      ObjectTracker::StopTracking(m_Data.m_Object); 
#endif
      delete m_Data.m_Object;
    }
  }
}

void Value::PrintLn() const
{
  switch (m_Type) {
    case Type::Bool:
      fmt::print("{}\n", m_Data.m_Bool);
      break;
    case Type::Char:
      fmt::print("{}\n", m_Data.m_Char);
      break;
    case Type::Double:
      fmt::print("{}\n", m_Data.m_Double);
      break;
    case Type::Int:
      fmt::print("{}\n", m_Data.m_Int);
      break;
    case Type::Null:
      fmt::print("null\n");
      break;
    case Type::Object:
      m_Data.m_Object->PrintLn();
      break;
    case Type::String:
      fmt::print("{}\n", *m_Data.m_Str);
      break;
  }
}

void Value::Print() const
{
  switch (m_Type) {
    case Type::Bool:
      fmt::print("{}", m_Data.m_Bool);
      break;
    case Type::Char:
      fmt::print("{}", m_Data.m_Char);
      break;
    case Type::Double:
      fmt::print("{}", m_Data.m_Double);
      break;
    case Type::Int:
      fmt::print("{}", m_Data.m_Int);
      break;
    case Type::Null:
      fmt::print("null");
      break;
    case Type::Object:
      m_Data.m_Object->Print();
      break;
    case Type::String:
      fmt::print("{}", *m_Data.m_Str);
      break;
  }
}

void Value::DebugPrint() const 
{
  switch (m_Type) {
    case Type::Bool:
      fmt::print("{}: {}\n", m_Type, m_Data.m_Bool);
      break;
    case Type::Char:
      fmt::print("{}: {}\n", m_Type, m_Data.m_Char);
      break;
    case Type::Double:
      fmt::print("{}: {}\n", m_Type, m_Data.m_Double);
      break;
    case Type::Int:
      fmt::print("{}: {}\n", m_Type, m_Data.m_Int);
      break;
    case Type::Null:
      fmt::print("{}: null\n", m_Type);
      break;
    case Type::Object:
      m_Data.m_Object->DebugPrint();
      break;
    case Type::String:
      fmt::print("{}: {}\n", m_Type, *m_Data.m_Str);
      break;
  }
}

std::string Value::AsString() const
{
  switch (m_Type) {
    case Type::Bool:
      return fmt::format("{}", m_Data.m_Bool);
    case Type::Char:
      return fmt::format("{}", m_Data.m_Char);
    case Type::Double:
      return fmt::format("{}", m_Data.m_Double);
    case Type::Int:
      return fmt::format("{}", m_Data.m_Int);
    case Type::Null:
      return "null";
    case Type::Object:
      return m_Data.m_Object->ToString();
    case Type::String:
      return fmt::format("{}", *m_Data.m_Str);
    default:
      GRACE_ASSERT(false, "Value::m_Type was not set");
      return "unknown";
  }
}

bool Value::AsBool() const
{
  switch (m_Type) {
    case Type::Bool:
      return m_Data.m_Bool;
    case Type::Char:
      return m_Data.m_Char > (char)0;
    case Type::Double:
      return m_Data.m_Double > 0.0;
    case Type::Int:
      return m_Data.m_Int > 0;
    case Type::Null:
      return false;
    case Type::String:
      return m_Data.m_Str->length() > 0;
    case Type::Object:
      return m_Data.m_Object->AsBool();
    default:
      GRACE_ASSERT(false, "Value::m_Type was not set");
      return false;
  }
}
 
std::tuple<bool, std::optional<std::string>> Value::AsInt(std::int64_t& result) const
{
  switch (m_Type) {
    case Type::Int: {
      result = m_Data.m_Int;
      return {true, {}};
    }
    case Type::Double: {
      result = static_cast<std::int64_t>(m_Data.m_Double);
      return {true, {}};
    }
    case Type::Bool: {
      result = m_Data.m_Bool ? 1 : 0;
      return {true, {}};
    }
    case Type::String: {
      try {
        result = std::stoll(*m_Data.m_Str);
        return {true, {}};
      } catch (const std::invalid_argument& e) {
        return {false, fmt::format("Could not convert '{}' to int: {}", *m_Data.m_Str, e.what())};
      } catch (const std::out_of_range&) {
        return {false, "Int represented by string was out of range"};
      }
    }
    case Type::Char: {
      result = static_cast<std::int64_t>(m_Data.m_Char);
      return {true, {}};
    }
    case Type::Null: {
      return {false, "Cannot convert `null` to int"};
    }
    default:
      return {false, "Cannot convert object to int"};
  }
}
 
std::tuple<bool, std::optional<std::string>> Value::AsDouble(double& result) const
{
  switch (m_Type) {
    case Type::Int: {
      result = static_cast<double>(m_Data.m_Int);
      return {true, {}};
    }
    case Type::Double: {
      result = m_Data.m_Double;
      return {true, {}};
    }
    case Type::Bool: {
      result = m_Data.m_Bool ? 1.0 : 0.0;
      return {true, {}};
    }
    case Type::String: {
      try {
        result = std::stod(*m_Data.m_Str);
        return {true, {}};
      } catch (const std::invalid_argument& e) {
        return {false, fmt::format("Could not convert '{}' to float: {}", *m_Data.m_Str, e.what())};
      } catch (const std::out_of_range&) {
        return {false, "Float represented by string was out of range"};
      }
    }
    case Type::Char: {
      result = static_cast<double>(m_Data.m_Char);
      return {true, {}};
    }
    case Type::Null: {
      return {false, "Cannot convert `null` to float"};
    }
    default:
      return {false, "Cannot convert object to float"};
  } 
}
 
std::tuple<bool, std::optional<std::string>> Value::AsChar(char& result) const
{
  switch (m_Type) {
    case Type::Int: {
      result = static_cast<char>(m_Data.m_Int);
      return {true, {}};
    }
    case Type::Double: {
      result = static_cast<char>(m_Data.m_Double);
      return {true, {}};
    }
    case Type::Bool: {
      result = static_cast<char>(m_Data.m_Bool);
      return {true, {}};
    }
    case Type::String: {
      if (m_Data.m_Str->length() == 1) {
        result = (*m_Data.m_Str)[0];
        return {true,{}};
      } else {
        return {false, fmt::format("Cannot convert {} to `char`, string must be 1 character long to convert to char", *m_Data.m_Str)};
      } 
    }
    case Type::Char: {
      result = m_Data.m_Char;
      return {true, {}};
    }
    case Type::Null: {
      return {false, "Cannot convert `null` to char"};
    }
    default:
      return {false, "Cannot convert object to char"};
  } 
}

Value Value::CreateList()
{
  Value value;
  value.m_Type = Type::Object;
  value.m_Data.m_Object = new GraceList();
  value.m_Data.m_Object->IncreaseRef();
#ifdef GRACE_DEBUG
  ObjectTracker::TrackObject(value.m_Data.m_Object); 
#endif
  return value;
}

Value Value::CreateList(std::vector<Value>&& items)
{
  Value value;
  value.m_Type = Type::Object;
  value.m_Data.m_Object = new GraceList(std::move(items));
  value.m_Data.m_Object->IncreaseRef();
#ifdef GRACE_DEBUG
  ObjectTracker::TrackObject(value.m_Data.m_Object); 
#endif
  return value;
}

Value Value::CreateList(const GraceList& list, std::int64_t multiple)
{
  Value value;
  value.m_Type = Type::Object;
  value.m_Data.m_Object = new GraceList(list, multiple);
  value.m_Data.m_Object->IncreaseRef();
#ifdef GRACE_DEBUG
  ObjectTracker::TrackObject(value.m_Data.m_Object); 
#endif
  return value;
}

Value Value::CreateList(const Value& value)
{
  Value res;
  res.m_Type = Type::Object;
  res.m_Data.m_Object = new GraceList(value);
  res.m_Data.m_Object->IncreaseRef();
#ifdef GRACE_DEBUG
  ObjectTracker::TrackObject(res.m_Data.m_Object);
#endif
  return res;
}
