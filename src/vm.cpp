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
    fmt::print("\t{}: ", name);
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

  callStack.push_back(std::make_tuple("file", "main", 1));
  Run("main", 1, verbose, callStack, {});
}

std::pair<InterpretResult, Value> VM::Run(const String& funcName, int startLine, bool verbose, CallStack& callStack, const std::vector<Value>& args)
{
#define PRINT_LOCAL_MEMORY()                                          \
  do {                                                                \
    if (verbose) {                                                    \
      PrintStack(valueStack, funcName);                               \
      PrintLocals(localsList, funcName);                              \
    }                                                                 \
  } while (false)                                                     \

#define RETURN_ERR()                                                  \
  do {                                                                \
    localsList.clear();                                               \
    return std::make_pair(InterpretResult::RuntimeError, Value());    \
  } while (false)                                                     \

#define RETURN_NULL()                                                 \
  do {                                                                \
    localsList.clear();                                               \
    return std::make_pair(InterpretResult::RuntimeOk, Value());       \
  } while (false)                                                     \

#define RETURN_VALUE(value)                                           \
  do {                                                                \
    localsList.clear();                                               \
    return std::make_pair(InterpretResult::RuntimeOk, value);         \
  } while (false)                                                     \
    
  std::vector<Value> valueStack;
  auto& function = m_FunctionList.at(funcName);
  auto& opList = function.m_OpList;
  auto& constantList = function.m_ConstantList;
  auto& localsList = function.m_Locals;
  std::size_t opCurrent = 0, constantCurrent = 0;

  for (auto i = 0; i < args.size(); i++) {
    localsList.insert(std::make_pair(function.m_Parameters[i], std::move(args[i])));
  }

  while (opCurrent < opList.size()) {
    auto [op, line] = opList[opCurrent++];
    switch (op) {
      case Ops::Add: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleAddition(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot add `{}` to `{}`", c2.GetType(), c1.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::Subtract: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleSubtraction(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot subtract `{}` from `{}`", c2.GetType(), c1.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::Multiply: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleMultiplication(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot multiply `{}` by `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::Divide: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleDivision(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot divide `{}` by `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::And: {
        auto [c1, c2] = PopLastTwo(valueStack);
        valueStack.emplace_back(c1.AsBool() == c2.AsBool());
        break;
      }
      case Ops::Or: {
        auto [c1, c2] = PopLastTwo(valueStack);
        valueStack.emplace_back(c1.AsBool() || c2.AsBool());
        break;
      }
      case Ops::Equal: {
        auto [c1, c2] = PopLastTwo(valueStack);
        HandleEquality(c1, c2, valueStack, true);
        break;
      }
      case Ops::NotEqual: {
        auto [c1, c2] = PopLastTwo(valueStack);
        HandleEquality(c1, c2, valueStack, false);
        break;
      }
      case Ops::Greater: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleGreaterThan(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::GreaterEqual: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleGreaterEqual(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::Less: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleLessThan(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::LessEqual: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleLessEqual(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot compare `{}` with `{}`", c1.GetType(), c2.GetType()), 
              InterpretError::InvalidOperand, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::Pow: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandlePower(c1, c2, valueStack)) {
          RuntimeError(fmt::format("cannot power `{}` with `{}`", c1.GetType(), c2.GetType()),
            InterpretError::InvalidOperand, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
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
        auto calleeName = std::move(valueStack.back().Get<std::string>());
        valueStack.pop_back();
        
        if (m_FunctionList.find(calleeName) == m_FunctionList.end()) {
          RuntimeError(fmt::format("Could not find function '{}'", calleeName), InterpretError::FunctionNotFound, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }

        auto& calleeFunc = m_FunctionList.at(calleeName);
        int arity = calleeFunc.m_Arity;
        auto numArgsGiven = valueStack.back().Get<std::int64_t>();
        valueStack.pop_back();

        if (numArgsGiven != arity) {
          RuntimeError(fmt::format("Incorrect number of arguments given to function '{}', expected {} but got {}", 
                calleeName, arity, numArgsGiven), 
              InterpretError::IncorrectArgCount, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }

        // top of the stack will be the last function argument 
        std::vector<Value> callArgs;
        for (auto i = 0; i < arity; i++) {
          callArgs.push_back(std::move(valueStack.back()));
          valueStack.pop_back();
        }
        
        callStack.push_back(std::make_tuple(funcName, calleeName, line));
        auto [res, value] = Run(calleeName, line, verbose, callStack, callArgs);
        if (res != InterpretResult::RuntimeOk) {
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        valueStack.push_back(std::move(value));
        callStack.pop_back();
        break;
      }
      case Ops::AssignLocal: {
        auto value = std::move(valueStack.back());
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
      case Ops::Return: {
        auto returnValue = std::move(valueStack.back());
        valueStack.pop_back();
#ifdef GRACE_DEBUG
        PRINT_LOCAL_MEMORY();
#endif
        RETURN_VALUE(returnValue);
      }
      default:
        GRACE_UNREACHABLE();
        break;
    }
  }

  RETURN_NULL();

#undef PRINT_LOCAL_MEMORY
#undef RETURN_ERR
#undef RETURN_NULL
#undef RETURN_VALUE
}

bool VM::AddFunction(const String& name, int line, std::vector<std::string>&& parameters)
{
  auto [it, res] = m_FunctionList.try_emplace(name, Function(name, std::move(parameters), line));
  m_LastFunction = name;
  return res;
}

void VM::RuntimeError(const std::string& message, InterpretError errorType, int line, const CallStack& callStack)
{
  fmt::print(stderr, "\n");
  
  fmt::print(stderr, "Call stack:\n");

  for (auto i = 1; i < callStack.size(); i++) {
    const auto& [caller, callee, ln] = callStack[i];
    fmt::print(stderr, "\tline {}, in {}:\n", ln, caller);
    fmt::print(stderr, "\t    {}\n", m_Compiler.GetCodeAtLine(ln));
  }

  fmt::print(stderr, "\tline {}, in {}:\n", line, std::get<1>(callStack.back()));
  fmt::print(stderr, "\t    {}\n", m_Compiler.GetCodeAtLine(line));

  fmt::print(stderr, "\n");
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print(stderr, "[line {}] {}: {}. Stopping execution.\n", line, errorType, message);
}
