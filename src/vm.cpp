#include "vm.h"
#include "vm_binary_op_handlers.h"

using namespace Grace::VM;

InterpretResult VM::Run()
{
  std::vector<Constant> constantsStack;

  while (m_OpCurrent < m_OpList.size()) {
    auto [op, line] = m_OpList[m_OpCurrent++];
    switch (op) {
      case Ops::Add: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (c1.m_Type == Constant::Type::Bool || c2.m_Type == Constant::Type::Bool) {
          RuntimeError("Cannot use type `bool` in binary expression", InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError; 
        }
        HandleAddition(c1, c2, constantsStack);
        break;
      }
      case Ops::Subtract: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (c1.m_Type == Constant::Type::Bool || c2.m_Type == Constant::Type::Bool) {
          RuntimeError("Cannot use type `bool` in binary expression", InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError; 
        }
        HandleSubtraction(c1, c2, constantsStack);
        break;
      }

      case Ops::Multiply: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (c1.m_Type == Constant::Type::Bool || c2.m_Type == Constant::Type::Bool) {
          RuntimeError("Cannot use type `bool` in binary expression", InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError; 
        }
        HandleMultiplication(c1, c2, constantsStack);
        break;
      }
      case Ops::Divide: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (c1.m_Type == Constant::Type::Bool || c2.m_Type == Constant::Type::Bool) {
          RuntimeError("Cannot use type `bool` in binary expression", InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError; 
        }
        HandleDivision(c1, c2, constantsStack);
        break;
      }
      case Ops::And: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (c1.m_Type != Constant::Type::Bool || c2.m_Type != Constant::Type::Bool) {
          RuntimeError("`and` can only be used with boolean operands", InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError; 
        }
        Constant c(Constant::Type::Bool);
        c.as.m_Bool = c1.as.m_Bool && c2.as.m_Bool;
        constantsStack.push_back(c);
        break;
      }
      case Ops::Or: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (c1.m_Type != Constant::Type::Bool || c2.m_Type != Constant::Type::Bool) {
          RuntimeError("`or` can only be used with boolean operands", InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError; 
        }
        Constant c(Constant::Type::Bool);
        c.as.m_Bool = c1.as.m_Bool || c2.as.m_Bool;
        constantsStack.push_back(c);
        break;
      }
      case Ops::Equal: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (!HandleEquality(c1, c2, constantsStack, true)) {
          RuntimeError(fmt::format("Invalid operands: cannot compare `{}` with `{}`", c1.m_Type, c2.m_Type), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::NotEqual: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (!HandleEquality(c1, c2, constantsStack, false)) {
          RuntimeError(fmt::format("Invalid operands: cannot compare `{}` with `{}`", c1.m_Type, c2.m_Type), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Greater: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (!HandleGreaterThan(c1, c2, constantsStack)) {
          RuntimeError(fmt::format("Invalid operands: cannot compare `{}` with `{}`", c1.m_Type, c2.m_Type), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::GreaterEqual: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (!HandleLessThan(c2, c1, constantsStack)) {
          RuntimeError(fmt::format("Invalid operands: cannot compare `{}` with `{}`", c1.m_Type, c2.m_Type), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Less: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (!HandleLessThan(c1, c2, constantsStack)) {
          RuntimeError(fmt::format("Invalid operands: cannot compare `{}` with `{}`", c1.m_Type, c2.m_Type), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::LessEqual: {
        auto c1 = constantsStack[constantsStack.size() - 2];
        auto c2 = constantsStack[constantsStack.size() - 1];
        constantsStack.pop_back();
        constantsStack.pop_back();
        if (!HandleGreaterThan(c2, c1, constantsStack)) {
          RuntimeError(fmt::format("Invalid operands: cannot compare `{}` with `{}`", c1.m_Type, c2.m_Type), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::LoadConstant:
        constantsStack.push_back(m_ConstantList[m_ConstantCurrent++]);
        break;
      case Ops::Pop:
        constantsStack.pop_back();
        break;
      case Ops::Print:
        constantsStack.back().Print();
        break;
    }
  }

  fmt::print("STACK:\n");
  for (const auto& c : constantsStack) {
    c.Print();
  }

  return InterpretResult::RuntimeOk;
}

void VM::RuntimeError(const std::string& message, InterpretError errorType, int line)
{
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print(stderr, "[line {}] {}: {}. Stopping execution\n", line, errorType, message);
}
