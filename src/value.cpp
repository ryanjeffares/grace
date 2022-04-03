#include <type_traits>

#include "value.hpp"

using namespace Grace::VM;

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
    case Type::String:
      fmt::print("{}: {}\n", m_Type, *m_Data.m_Str);
      break;
  }
}

[[nodiscard]]
std::string Value::AsString() const
{
  switch (m_Type)
  {
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
    case Type::String:
      return fmt::format("{}", *m_Data.m_Str);
    default:
      return "Invalid type";
  }
}

[[nodiscard]]
bool Value::AsBool() const
{
  switch (m_Type)
  {
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
    default:
      return false;
  }
}

[[nodiscard]] 
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
  }
}

[[nodiscard]] 
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
        return {false, fmt::format("Could not convert '{}' to int: {}", *m_Data.m_Str, e.what())};
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
  } 
}

[[nodiscard]] 
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
  } 
}

