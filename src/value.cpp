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
#include "value.hpp"
#include "objects/grace_list.hpp"

namespace Grace::VM
{

  Value::Value()
    : m_Type(Type::Null)
  {
    m_Data.m_Null = nullptr;
  }

  Value::Value(const Value& other)
    : m_Type(other.m_Type)
  {
    if (other.m_Type == Type::String) {
      m_Data.m_Str = new std::string(*other.m_Data.m_Str);
    } else if (other.m_Type == Type::Object) {
      m_Data.m_Object = other.m_Data.m_Object;
      m_Data.m_Object->IncreaseRef();
    } else {
      m_Data = other.m_Data;
    }
  }

  Value::Value(Value&& other)
    : m_Type(other.m_Type)
  {
    m_Data = other.m_Data;

    if (other.m_Type == Type::Object || other.m_Type == Type::String) {
      other.m_Data.m_Null = nullptr;
      other.m_Type = Type::Null;
    }
  }

  Value::Value(GraceObject* object)
    : m_Type(Type::Object)
  {
    m_Data.m_Object = object;
    object->IncreaseRef();
  }

  Value::~Value()
  {
    if (m_Type == Type::String) {
      delete m_Data.m_Str;
    }
    if (m_Type == Type::Object) {
      if (m_Data.m_Object->DecreaseRef() == 0) {
        ObjectTracker::StopTrackingObject(m_Data.m_Object);
        delete m_Data.m_Object;
      }
    }
  }

  Value Value::operator+(const Value& other) const
  {
    switch (m_Type) {
      case Type::Int: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(m_Data.m_Int + other.m_Data.m_Int);
          }
          case Type::Double: {
            return Value(static_cast<double>(m_Data.m_Int) + other.m_Data.m_Double);
          }
          default: break;
        }
        break;
      }
      case Type::Double: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(m_Data.m_Double + static_cast<double>(other.m_Data.m_Int));
          }
          case Type::Double: {
            return Value(m_Data.m_Double + other.m_Data.m_Double);
          }
          default: break;
        }
        break;
      }
      case Type::Char: {
        if (other.m_Type == Type::Char) {
          std::string res;
          res.push_back(m_Data.m_Char);
          res.push_back(other.m_Data.m_Char);
          return Value(res);
        }
        break;
      }
      case Type::String: {
        switch (other.m_Type) {
          case Type::String: {
            return Value(Get<std::string>() + other.Get<std::string>());
          }
          case Type::Char: {
            return Value(Get<std::string>() + other.m_Data.m_Char);
          }
          default: {
            return Value(Get<std::string>() + other.AsString());
          }
        }
        break;
      }
      default: break;
    }
    throw GraceException(
      GraceException::Type::InvalidOperand,
      fmt::format("Cannot add {} to {}", other.GetTypeName(), GetTypeName())
    );
  }

  Value Value::operator-(const Value& other) const
  {
    switch (m_Type) {
      case Type::Int: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(m_Data.m_Int - other.m_Data.m_Int);
          }
          case Type::Double: {
            return Value(static_cast<double>(m_Data.m_Int) - other.m_Data.m_Double);
          }
          default: break;
        }
        break;
      }
      case Type::Double: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(m_Data.m_Double - static_cast<double>(other.m_Data.m_Int));
          }
          case Type::Double: {
            return Value(m_Data.m_Double - other.m_Data.m_Double);
          }
          default: break;
        }
        break;
      }
      default: break;
    }
    throw GraceException(
      GraceException::Type::InvalidOperand,
      fmt::format("Cannot subtract {} from {}", other.GetTypeName(), GetTypeName())
    );
  }

  Value Value::operator/(const Value& other) const
  {
    switch (m_Type) {
      case Type::Int: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(m_Data.m_Int / other.m_Data.m_Int);
          }
          case Type::Double: {
            return Value(static_cast<double>(m_Data.m_Int) / other.m_Data.m_Double);
          }
          default: break;
        }
        break;
      }
      case Type::Double: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(m_Data.m_Double / static_cast<double>(other.m_Data.m_Int));
          }
          case Type::Double: {
            return Value(m_Data.m_Double / other.m_Data.m_Double);
          }
          default: break;
        }
        break;
      }
      default: break;
    }
    throw GraceException(
      GraceException::Type::InvalidOperand,
      fmt::format("Cannot divide {} by {}", GetTypeName(), other.GetTypeName())
    );
  }

  Value Value::operator*(const Value& other) const
  {
    switch (m_Type) {
      case Type::Int: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(m_Data.m_Int * other.m_Data.m_Int);
          }
          case Type::Double: {
            return Value(static_cast<double>(m_Data.m_Int) * other.m_Data.m_Double);
          }
          default: break;
        }
        break;
      }
      case Type::Double: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(m_Data.m_Double * static_cast<double>(other.m_Data.m_Int));
          }
          case Type::Double: {
            return Value(m_Data.m_Double * other.m_Data.m_Double);
          }
          default: break;
        }
        break;
      }
      case Type::Char: {
        if (other.m_Type == Type::Int) {
          return Value(std::string(other.m_Data.m_Int, m_Data.m_Char));
        }
        break;
      }
      case Type::String: {
        if (other.m_Type == Type::Int) {
          std::string res;
          if (other.m_Data.m_Int > 0) {
            for (auto i = 0; i < other.m_Data.m_Int; i++) {
              res += Get<std::string>();
            }
          }
          return Value(res);
        }
        break;
      }
      case Type::Object: {
        if (auto list = GetObject()->GetAsList()) {
          if (other.m_Type == Type::Int) {
            return Value(CreateObject<GraceList>(*list, other.m_Data.m_Int));
          }
        }
        break;
      }
      default: break;
    }
    throw GraceException(
      GraceException::Type::InvalidOperand,
      fmt::format("Cannot multiple {} by {}", GetTypeName(), other.GetTypeName())
    );
  }

  Value Value::operator%(const Value& other) const
  {
    switch (m_Type) {
      case Type::Int: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(m_Data.m_Int % other.m_Data.m_Int);
          }
          case Type::Double: {
            return Value(std::fmod(static_cast<double>(m_Data.m_Int), other.m_Data.m_Double));
          }
          default: break;
        }
        break;
      }
      case Type::Double: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(std::fmod(m_Data.m_Double, static_cast<double>(other.m_Data.m_Int)));
          }
          case Type::Double: {
            return Value(std::fmod(m_Data.m_Double, other.m_Data.m_Double));
          }
          default: break;
        }
        break;
      }
      default: break;
    }
    throw GraceException(
      GraceException::Type::InvalidOperand,
      fmt::format("Cannot mod {} by {}", GetTypeName(), other.GetTypeName())
    );
  }

  Value Value::operator<<(const Value& other) const
  {
    if (m_Type != Type::Int || other.m_Type != Type::Int) {
      throw GraceException(
        GraceException::Type::InvalidOperand,
        fmt::format("Cannot shift {} by {}", GetTypeName(), other.GetTypeName())
      );
    }
    return Value(m_Data.m_Int << other.m_Data.m_Int);
  }

  Value Value::operator>>(const Value& other) const
  {
    if (m_Type != Type::Int || other.m_Type != Type::Int) {
      throw GraceException(
        GraceException::Type::InvalidOperand,
        fmt::format("Cannot shift {} by {}", GetTypeName(), other.GetTypeName())
      );
    }
    return Value(m_Data.m_Int >> other.m_Data.m_Int);
  }

  Value Value::operator|(const Value& other) const
  {
    if (m_Type != Type::Int || other.m_Type != Type::Int) {
      throw GraceException(
        GraceException::Type::InvalidOperand,
        fmt::format("Cannot bitwise or {} by {}", GetTypeName(), other.GetTypeName())
      );
    }
    return Value(m_Data.m_Int | other.m_Data.m_Int);
  }

  Value Value::operator^(const Value& other) const
  {
    if (m_Type != Type::Int || other.m_Type != Type::Int) {
      throw GraceException(
        GraceException::Type::InvalidOperand,
        fmt::format("Cannot bitwise xor {} by {}", GetTypeName(), other.GetTypeName())
      );
    }
    return Value(m_Data.m_Int ^ other.m_Data.m_Int);
  }

  Value Value::operator&(const Value& other) const
  {
    if (m_Type != Type::Int || other.m_Type != Type::Int) {
      throw GraceException(
        GraceException::Type::InvalidOperand,
        fmt::format("Cannot bitwise and {} by {}", GetTypeName(), other.GetTypeName())
      );
    }
    return Value(m_Data.m_Int & other.m_Data.m_Int);
  }

  bool Value::operator==(const Value& other) const
  {
    switch (m_Type) {
      case Type::Int: {
        switch (other.m_Type) {
          case Type::Double: {
            return static_cast<double>(m_Data.m_Int) == other.m_Data.m_Double;
          }
          case Type::Int: {
            return m_Data.m_Int == other.m_Data.m_Int;
          }
          default: break;
        }
        break;
      }
      case Type::Double: {
        switch (other.m_Type) {
          case Type::Int: {
            return m_Data.m_Double == static_cast<double>(other.m_Data.m_Int);
          }
          case Type::Double: {
            return m_Data.m_Double == other.m_Data.m_Double;
          }
          default: break;
        }
        break;
      }
      case Type::Bool: {
        if (other.m_Type == Type::Bool) {
          return m_Data.m_Bool == other.m_Data.m_Bool;
        }
        break;
      }
      case Type::Char: {
        switch (other.m_Type) {
          case Type::String: {
            return other.Get<std::string>().length() == 1 && m_Data.m_Char == other.Get<std::string>()[0];
          }
          case Type::Char: {
            return m_Data.m_Char == other.m_Data.m_Char;
          }
          default: break;
        }
        break;
      }
      case Type::String: {
        switch (other.m_Type) {
          case Type::String: {
            return Get<std::string>() == other.Get<std::string>();
          }
          case Type::Char: {
            return Get<std::string>().length() == 1 && Get<std::string>()[0] == other.m_Data.m_Char;
          }
          default: break;
        }
        break;
      }
      case Type::Null: {
        if (other.m_Type == Type::Null) {
          return true;
        }
        break;
      }
      case Type::Object: {
        return m_Data.m_Object == other.m_Data.m_Object;
      }
      default: break;
    }
    return false;
  }

  bool Value::operator!=(const Value& other) const
  {
    return !(*this == other);
  }

  bool Value::operator<(const Value& other) const
  {
    switch (m_Type) {
      case Type::Int: {
        switch (other.m_Type) {
          case Type::Double: {
            return static_cast<double>(m_Data.m_Int) < other.m_Data.m_Double;
          }
          case Type::Int: {
            return m_Data.m_Int < other.m_Data.m_Int;
          }
          default: break;
        }
        break;
      }
      case Type::Double: {
        switch (other.m_Type) {
          case Type::Int: {
            return m_Data.m_Double < static_cast<double>(other.m_Data.m_Int);
          }
          case Type::Double: {
            return m_Data.m_Double < other.m_Data.m_Double;
          }
          default: break;
        }
        break;
      }
      case Type::Char: {
        if (other.m_Type == Type::Char) {
          return m_Data.m_Char < other.m_Data.m_Char;
        }
        break;
      }
      default: break;
    }
    throw GraceException(
      GraceException::Type::InvalidOperand,
      fmt::format("Cannot compare {} with {}", GetTypeName(), other.GetTypeName())
    );
  }

  bool Value::operator<=(const Value& other) const
  {
    switch (m_Type) {
      case Type::Int: {
        switch (other.m_Type) {
          case Type::Double: {
            return static_cast<double>(m_Data.m_Int) <= other.m_Data.m_Double;
          }
          case Type::Int: {
            return m_Data.m_Int <= other.m_Data.m_Int;
          }
          default: break;
        }
        break;
      }
      case Type::Double: {
        switch (other.m_Type) {
          case Type::Int: {
            return m_Data.m_Double <= static_cast<double>(other.m_Data.m_Int);
          }
          case Type::Double: {
            return m_Data.m_Double <= other.m_Data.m_Double;
          }
          default: break;
        }
        break;
      }
      case Type::Char: {
        if (other.m_Type == Type::Char) {
          return m_Data.m_Char <= other.m_Data.m_Char;
        }
        break;
      }
      default: break;
    }
    throw GraceException(
      GraceException::Type::InvalidOperand,
      fmt::format("Cannot compare {} with {}", GetTypeName(), other.GetTypeName())
    );
  }

  bool Value::operator>(const Value& other) const
  {
    return !(*this <= other);
  }

  bool Value::operator>=(const Value& other) const
  {
    return !(*this < other);
  }

  Value Value::operator!() const
  {
    return Value(!AsBool());
  }

  Value Value::operator-() const
  {
    switch (m_Type) {
      case Type::Int: {
        return Value(-(m_Data.m_Int));
      }
      case Type::Double: {
        return Value(-(m_Data.m_Double));
      }
      default: break;
    }
    throw GraceException(
      GraceException::Type::InvalidType,
      fmt::format("Cannot negate type {}", m_Type)
    );
  }

  Value Value::operator~() const
  {
    if (m_Type != Type::Int) {
      throw GraceException(
        GraceException::Type::InvalidOperand,
        fmt::format("Cannot bitwise not {}", GetTypeName())
      );
    }
    return Value(~(m_Data.m_Int));
  }

  Value Value::Pow(const Value& other) const
  {
    switch (m_Type) {
      case Type::Int: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(static_cast<std::int64_t>(std::pow(m_Data.m_Int, other.m_Data.m_Int)));
          }
          case Type::Double: {
            return Value(std::pow(static_cast<double>(m_Data.m_Int), other.m_Data.m_Double));
          }
          default: break;
        }
        break;
      }
      case Type::Double: {
        switch (other.m_Type) {
          case Type::Int: {
            return Value(std::pow(m_Data.m_Double, static_cast<double>(other.m_Data.m_Int)));
          }
          case Type::Double: {
            return Value(std::pow(m_Data.m_Double, other.m_Data.m_Double));
          }
          default: break;
        }
        break;
      }
      default: break;
    }
    throw GraceException(
      GraceException::Type::InvalidOperand,
      fmt::format("Cannot exponentiate {} with {}", GetTypeName(), other.GetTypeName())
    );
  }

  void Value::PrintLn(bool err) const
  {
    auto stream = err ? stderr : stdout;
    switch (m_Type) {
      case Type::Bool:
        fmt::print(stream, "{}\n", m_Data.m_Bool);
        break;
      case Type::Char:
        fmt::print(stream, "{}\n", m_Data.m_Char);
        break;
      case Type::Double:
        fmt::print(stream, "{}\n", m_Data.m_Double);
        break;
      case Type::Int:
        fmt::print(stream, "{}\n", m_Data.m_Int);
        break;
      case Type::Null:
        fmt::print(stream, "null\n");
        break;
      case Type::Object:
        m_Data.m_Object->PrintLn(err);
        break;
      case Type::String:
        fmt::print(stream, "{}\n", *m_Data.m_Str);
        break;
    }
  }

  void Value::Print(bool err) const
  {
    auto stream = err ? stderr : stdout;
    switch (m_Type) {
      case Type::Bool:
        fmt::print(stream, "{}", m_Data.m_Bool);
        break;
      case Type::Char:
        fmt::print(stream, "{}", m_Data.m_Char);
        break;
      case Type::Double:
        fmt::print(stream, "{}", m_Data.m_Double);
        break;
      case Type::Int:
        fmt::print(stream, "{}", m_Data.m_Int);
        break;
      case Type::Null:
        fmt::print(stream, "null");
        break;
      case Type::Object:
        m_Data.m_Object->Print(err);
        break;
      case Type::String:
        fmt::print(stream, "{}", *m_Data.m_Str);
        break;
      default:
        GRACE_ASSERT(false, "Value::m_Type was not set");
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
      default:
        GRACE_ASSERT(false, "Value::m_Type was not set");
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

  static bool CompareStringsIgnoreCase(const std::string& first, const std::string& second)
  {
    auto size = first.size();
    if (size != second.length()) {
      return false;
    }

    for (std::size_t i = 0; i < size; i++) {
      if (std::tolower(first[i]) != std::tolower(second[i])) {
        return false;
      }
    }

    return true;
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
      case Type::String: {
        if (CompareStringsIgnoreCase(*m_Data.m_Str, "true")) {
          return true;
        }
        if (CompareStringsIgnoreCase(*m_Data.m_Str, "false")) {
          return false;
        }
        return m_Data.m_Str->length() > 0;
      }
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
        return { true, {} };
      }
      case Type::Double: {
        result = static_cast<std::int64_t>(m_Data.m_Double);
        return { true, {} };
      }
      case Type::Bool: {
        result = m_Data.m_Bool ? 1 : 0;
        return { true, {} };
      }
      case Type::String: {
        try {
          result = std::stoll(*m_Data.m_Str);
          return { true, {} };
        }
        catch (const std::invalid_argument& e) {
          return { false, fmt::format("Could not convert '{}' to int: {}", *m_Data.m_Str, e.what()) };
        }
        catch (const std::out_of_range&) {
          return { false, "Int represented by string was out of range" };
        }
      }
      case Type::Char: {
        result = static_cast<std::int64_t>(static_cast<unsigned char>(m_Data.m_Char));
        return { true, {} };
      }
      case Type::Null: {
        return { false, "Cannot convert `null` to int" };
      }
      default:
        return { false, "Cannot convert object to int" };
    }
  }

  std::tuple<bool, std::optional<std::string>> Value::AsDouble(double& result) const
  {
    switch (m_Type) {
      case Type::Int: {
        result = static_cast<double>(m_Data.m_Int);
        return { true, {} };
      }
      case Type::Double: {
        result = m_Data.m_Double;
        return { true, {} };
      }
      case Type::Bool: {
        result = m_Data.m_Bool ? 1.0 : 0.0;
        return { true, {} };
      }
      case Type::String: {
        try {
          result = std::stod(*m_Data.m_Str);
          return { true, {} };
        }
        catch (const std::invalid_argument& e) {
          return { false, fmt::format("Could not convert '{}' to float: {}", *m_Data.m_Str, e.what()) };
        }
        catch (const std::out_of_range&) {
          return { false, "Float represented by string was out of range" };
        }
      }
      case Type::Char: {
        result = static_cast<double>(m_Data.m_Char);
        return { true, {} };
      }
      case Type::Null: {
        return { false, "Cannot convert `null` to float" };
      }
      default:
        return { false, "Cannot convert object to float" };
    }
  }

  std::tuple<bool, std::optional<std::string>> Value::AsChar(char& result) const
  {
    switch (m_Type) {
      case Type::Int: {
        result = static_cast<char>(m_Data.m_Int);
        return { true, {} };
      }
      case Type::Double: {
        result = static_cast<char>(m_Data.m_Double);
        return { true, {} };
      }
      case Type::Bool: {
        result = static_cast<char>(m_Data.m_Bool);
        return { true, {} };
      }
      case Type::String: {
        if (m_Data.m_Str->length() == 1) {
          result = (*m_Data.m_Str)[0];
          return { true,{} };
        }
        else {
          return { false, fmt::format("Cannot convert {} to `char`, string must be 1 character long to convert to char", *m_Data.m_Str) };
        }
      }
      case Type::Char: {
        result = m_Data.m_Char;
        return { true, {} };
      }
      case Type::Null: {
        return { false, "Cannot convert `null` to char" };
      }
      default:
        return { false, "Cannot convert object to char" };
    }
  }

  GRACE_NODISCARD std::string Value::GetTypeName() const
  {
    if (m_Type == Type::Object) {
      return std::string(m_Data.m_Object->ObjectName());
    }
    return fmt::format("{}", m_Type);
  }
} // namespace Grace::VM

namespace std
{
  static hash<std::int64_t> s_IntHash{};
  static hash<double> s_DoubleHash{};
  static hash<char> s_CharHash{};
  static hash<bool> s_BoolHash{};
  static hash<std::string> s_StringHash{};
  static hash<Grace::GraceObject*> s_ObjectHash{};

  size_t hash<Grace::VM::Value>::operator()(const Grace::VM::Value& value) const
  {
    switch (value.GetType()) {
      case Grace::VM::Value::Type::Bool:
        return s_BoolHash(value.Get<bool>());
      case Grace::VM::Value::Type::Char:
        return s_CharHash(value.Get<char>());
      case Grace::VM::Value::Type::Double:
        return s_DoubleHash(value.Get<double>());
      case Grace::VM::Value::Type::Int:
        return s_IntHash(value.Get<std::int64_t>());
      case Grace::VM::Value::Type::Null:
        throw Grace::GraceException(
          Grace::GraceException::Type::InvalidType,
          "Cannot hash null value"
        );
      case Grace::VM::Value::Type::Object:
        return s_ObjectHash(value.GetObject());
      case Grace::VM::Value::Type::String:
        return s_StringHash(value.Get<std::string>());
      default:
        GRACE_UNREACHABLE();
        return 0;
    }
  }
}
