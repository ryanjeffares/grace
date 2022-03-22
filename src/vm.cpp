#include "grace.hpp"

#include "compiler.hpp"
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

static void PrintStack(std::vector<Value>& stack, const Grace::String& funcName)
{
  fmt::print("{} STACK:\n", funcName);
  for (const auto& c : stack) {
    fmt::print("\t");
    c.DebugPrint();
  }
}

static void PrintLocals(const std::unordered_map<Grace::String, Value>& locals, const Grace::String& funcName)
{
  fmt::print("{} LOCALS:\n", funcName);
  for (const auto& [name, value] : locals) {
    fmt::print("\t");
    value.DebugPrint();
  }
}

void VM::Start(bool verbose)
{
  CallStack callStack;
  if (m_FunctionList.find("main") == m_FunctionList.end()) {
    RuntimeError("Could not find `main` function", InterpretError::FunctionNotFound, 1, {});
    return;
  }

  Run("main", 1, verbose, callStack);
}

InterpretResult VM::Run(const String& funcName, int startLine, bool verbose, CallStack& callStack)
{
  if (m_FunctionList.find(funcName) == m_FunctionList.end()) {
    RuntimeError(fmt::format("Could not find function {}", funcName), InterpretError::FunctionNotFound, startLine, {});
    return InterpretResult::RuntimeError;
  }

  std::vector<Value> valueStack;
  auto& function = m_FunctionList.at(funcName);
  auto& opList = function.m_OpList;
  auto& constantList = function.m_ConstantList;
  auto& localsList = function.m_Locals;
  std::size_t opCurrent = 0, constantCurrent = 0;

  while (opCurrent < opList.size()) {
    auto [op, line] = opList[opCurrent++];
    switch (op) {
      case Ops::Add: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleAddition(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot add `{}` to `{}`", c2.GetType(), c1.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Subtract: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleSubtraction(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot subtract `{}` from `{}`", c2.GetType(), c1.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Multiply: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleMultiplication(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot multiply `{}` by `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Divide: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleDivision(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot divide `{}` by `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::And: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (c1.GetType() != Value::Type::Bool || c2.GetType() != Value::Type::Bool) {
          RuntimeError("`and` can only be used with boolean operands", InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError; 
        }
        valueStack.emplace_back(c1.Get<bool>() == c2.Get<bool>());
        break;
      }
      case Ops::Or: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (c1.GetType() != Value::Type::Bool || c2.GetType() != Value::Type::Bool) {
          RuntimeError("`or` can only be used with boolean operands", InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError; 
        }
        valueStack.emplace_back(c1.Get<bool>() || c2.Get<bool>());
        break;
      }
      case Ops::Equal: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleEquality(c1, c2, valueStack, true)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::NotEqual: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleEquality(c1, c2, valueStack, false)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Greater: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleGreaterThan(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::GreaterEqual: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleGreaterEqual(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Less: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleLessThan(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::LessEqual: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleLessEqual(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::Pow: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandlePower(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot power `{}` with `{}`", c1.GetType(), c2.GetType()),
            InterpretError::InvalidOperand, line, callStack);
          return InterpretResult::RuntimeError;
        }
        break;
      }
      case Ops::LoadConstant:
        valueStack.push_back(constantList[constantCurrent++]);
        break;
      case Ops::LoadLocal: {
        auto name = valueStack.back().Get<std::string>();
        valueStack.pop_back();
        auto value = localsList[name];
        valueStack.push_back(value);
        break;
      }
      case Ops::Pop:
        valueStack.pop_back();
        break;
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
        auto calleeName = valueStack.back().Get<std::string>();
        valueStack.pop_back();
        callStack.push_back(std::make_tuple(funcName, calleeName, line));
        auto res = Run(calleeName, line, verbose, callStack);
        if (res != InterpretResult::RuntimeOk) {
          return res;
        }
        callStack.pop_back();
        break;
      }
      case Ops::AssignLocal: {
        auto value = valueStack.back();
        valueStack.pop_back();
        localsList[valueStack.back().Get<std::string>()] = value;
        valueStack.pop_back();
        break;
      }
      case Ops::DeclareLocal: {
        auto name = valueStack.back().Get<std::string>();
        localsList.insert(std::make_pair(name, Value()));
        break;
      }
      default:
        GRACE_UNREACHABLE();
        break;
    }
  }

#ifdef GRACE_DEBUG
  if (verbose) {
    PrintStack(valueStack, funcName);
    PrintLocals(localsList, funcName);
  }
#endif

  // clearing the locals here because that memory is no longer necessary
  // not clearning the value stack, because that shouldnt have anything on it 
  // so printing above while still in debug will show any problems
  localsList.clear();
  return InterpretResult::RuntimeOk;
}

bool VM::AddFunction(const String& name, int line)
{
  auto [it, res] = m_FunctionList.try_emplace(name, Function(name, line));
  m_LastFunction = name;
  return res;
}

void VM::RuntimeError(const std::string& message, InterpretError errorType, int line, const CallStack& callStack)
{
  fmt::print(stderr, "\n");
  fmt::print(stderr, "Call stack:\n");
  
  for (const auto& [caller, callee, ln] : callStack){
    fmt::print(stderr, "\tline {}, in {}\n", ln, caller);
    fmt::print(stderr, "\t\t{}\n", m_Compiler.GetCodeAtLine(ln));
  }

  fmt::print(stderr, "\tline {}, in {}\n", line, std::get<1>(callStack.back()));
  fmt::print(stderr, "\t\t{}\n", m_Compiler.GetCodeAtLine(line));

  fmt::print(stderr, "\n");
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print(stderr, "[line {}] {}: {}. Stopping execution.\n", line, errorType, message);
}
