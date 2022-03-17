#include "grace.hpp"

#include "vm.hpp"
#include "vm_binop_handlers.hpp"

using namespace Grace::VM;

struct Result 
{
  Value m_C1, m_C2;

  Result(Value&& c1, Value&& c2)
    : m_C1(std::move(c1)), m_C2(std::move(c2))
  {

  }
};

static inline Result PopLastTwo(std::vector<Value>& stack)
{
  auto c1 = std::move(stack[stack.size() - 2]);
  auto c2 = std::move(stack[stack.size() - 1]);
  stack.pop_back();
  stack.pop_back();
  return Result(std::move(c1), std::move(c2));
}

InterpretResult VM::Run(const String& funcName, int startLine)
{
  if (m_FunctionList.find(funcName) == m_FunctionList.end()) {
    RuntimeError(fmt::format("Could not find function {}", funcName), InterpretError::FunctionNotFound, startLine);
    return InterpretResult::RuntimeError;
  }

  std::vector<Value> valueStack;
  auto& function = m_FunctionList.at(funcName);
  auto& opList = function.m_OpList;
  auto& constantList = function.m_ConstantList;
  std::size_t opCurrent = 0, constantCurrent = 0;

  while (opCurrent < opList.size()) {
    auto [op, line] = opList[opCurrent++];
    switch (op) {
      case Ops::Add: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleAddition(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot add `{}` to `{}`", c2.GetType(), c1.GetType()), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Subtract: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleSubtraction(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot subtract `{}` from `{}`", c2.GetType(), c1.GetType()), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Multiply: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleMultiplication(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot multiply `{}` by `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Divide: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleDivision(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot divide `{}` by `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::And: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (c1.GetType() != Value::Type::Bool || c2.GetType() != Value::Type::Bool) {
          RuntimeError("`and` can only be used with boolean operands", InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError; 
        }
        valueStack.emplace_back(c1.Get<bool>() == c2.Get<bool>());
        break;
      }
      case Ops::Or: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (c1.GetType() != Value::Type::Bool || c2.GetType() != Value::Type::Bool) {
          RuntimeError("`or` can only be used with boolean operands", InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError; 
        }
        valueStack.emplace_back(c1.Get<bool>() || c2.Get<bool>());
        break;
      }
      case Ops::Equal: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleEquality(c1, c2, valueStack, true)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::NotEqual: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleEquality(c1, c2, valueStack, false)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Greater: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleGreaterThan(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::GreaterEqual: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleGreaterEqual(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Less: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleLessThan(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::LessEqual: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleLessEqual(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::LoadConstant:
        valueStack.push_back(constantList[constantCurrent++]);
        break;
      case Ops::Pop:
        valueStack.pop_back();
        break;
      case Ops::Pow: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandlePower(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot power `{}` with `{}`", c1.GetType(), c2.GetType()),
            InterpretError::InvalidOperand, line);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Print:
        valueStack.back().Print();
        break;
      case Ops::PrintEmptyLine:
        fmt::print("\n");
        break;
      case Ops::PrintLn:
        valueStack.back().PrintLn();
        break;
      case Ops::PrintTab:
        fmt::print("\t");
        break;
      case Ops::Call: {
        auto funcName = valueStack.back().Get<std::string>();
        valueStack.pop_back();
        auto res = Run(funcName, line);
        if (res != InterpretResult::RuntimeOk) {
          return res;
        }
        break;
      }
      default:
        GRACE_UNREACHABLE();
        break;
    }
  }

#ifdef GRACE_DEBUG
  fmt::print("STACK:\n");
  for (const auto& c : valueStack) {
    fmt::print("\t");
    c.PrintLn();
  }
#endif

  return InterpretResult::RuntimeOk;
}

bool VM::AddFunction(const String& name)
{
  auto [it, res] = m_FunctionList.try_emplace(name, Function(name));
  m_LastFunction = name;
  return res;
}

void VM::RuntimeError(const std::string& message, InterpretError errorType, int line)
{
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print(stderr, "[line {}] {}: {}. Stopping execution.\n", line, errorType, message);
}
