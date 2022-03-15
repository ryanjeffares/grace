#include "vm.h"
#include "vm_binary_op_handlers.h"

using namespace Grace::VM;

InterpretResult VM::Run()
{
  std::vector<Constant> stack;

  while (m_OpCurrent < m_OpList.size()) {
    switch (m_OpList[m_OpCurrent++]) {
      case Ops::Add: {
        auto c1 = stack[stack.size() - 2];
        auto c2 = stack[stack.size() - 1];
        stack.pop_back();
        stack.pop_back();
        if (c1.m_Type == Constant::Type::Bool || c2.m_Type == Constant::Type::Bool) {
          RuntimeError("Cannot use type `bool` in binary expression", InterpretError::InvalidOperand);
          return InterpretResult::RuntimeError; 
        }
        HandleAddition(c1, c2, stack);
        break;
      }
      case Ops::Divide: {
        auto c1 = stack[stack.size() - 2];
        auto c2 = stack[stack.size() - 1];
        stack.pop_back();
        stack.pop_back();
        if (c1.m_Type == Constant::Type::Bool || c2.m_Type == Constant::Type::Bool) {
          RuntimeError("Cannot use type `bool` in binary expression", InterpretError::InvalidOperand);
          return InterpretResult::RuntimeError; 
        }
        HandleDivision(c1, c2, stack);
        break;
      }
      case Ops::LoadBool:
      case Ops::LoadFloat:
      case Ops::LoadInteger:
        stack.push_back(m_ConstantList[m_ConstantCurrent++]);
        break;
      case Ops::Multiply: {
        auto c1 = stack[stack.size() - 2];
        auto c2 = stack[stack.size() - 1];
        stack.pop_back();
        stack.pop_back();
        if (c1.m_Type == Constant::Type::Bool || c2.m_Type == Constant::Type::Bool) {
          RuntimeError("Cannot use type `bool` in binary expression", InterpretError::InvalidOperand);
          return InterpretResult::RuntimeError; 
        }
        HandleMultiplication(c1, c2, stack);
        break;
      }
      case Ops::Pop:
        stack.pop_back();
        break;
      case Ops::Print:
        stack.back().Print();
        break;
      case Ops::Subtract: {
        auto c1 = stack[stack.size() - 2];
        auto c2 = stack[stack.size() - 1];
        stack.pop_back();
        stack.pop_back();
        if (c1.m_Type == Constant::Type::Bool || c2.m_Type == Constant::Type::Bool) {
          RuntimeError("Cannot use type `bool` in binary expression", InterpretError::InvalidOperand);
          return InterpretResult::RuntimeError; 
        }
        HandleSubtraction(c1, c2, stack);
        break;
      }
    }
  }

  fmt::print("STACK:\n");
  for (const auto& c : stack) {
    c.Print();
  }
  return InterpretResult::RuntimeOk;
}

void VM::RuntimeError(const std::string& message, InterpretError errorType)
{
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print(stderr, "{}. Stopping execution\n", errorType);
}
