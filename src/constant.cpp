#include <type_traits>

#include "constant.hpp"

using namespace Grace::VM;

void Constant::PrintLn() const
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
    case Type::String:
      fmt::print("{}\n", *m_Data.m_Str);
      break;
  }
}

void Constant::Print() const
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
    case Type::String:
      fmt::print("{}", *m_Data.m_Str);
      break;
  }
}

std::string Constant::ToString() const
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
    case Type::String:
      return fmt::format("{}", *m_Data.m_Str);
    default:
      return "Invalid type";
  }
}

