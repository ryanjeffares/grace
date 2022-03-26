#include <cstdlib>

#include "grace.hpp"

#include "compiler.hpp"
#include "vm.hpp"
#include "vm_binop_handlers.hpp"

using namespace Grace::VM;

struct Result 
{
  Value m_C1, m_C2;

  Result(Value&& c1, Value&& c2)
    : m_C1(c1), m_C2(c2)
  {

  }
};

static inline Result PopLastTwo(std::vector<Value>& stack)
{
  auto c1 = std::move(stack[stack.size() - 2]);
  auto c2 = std::move(stack[stack.size() - 1]);
  stack.pop_back();
  stack.pop_back();
  return {std::move(c1), std::move(c2)};
}

static inline Value Pop(std::vector<Value>& stack)
{
  auto c = std::move(stack.back());
  stack.pop_back();
  return c;
}

static void PrintStack(std::vector<Value>& stack, const std::string& funcName)
{
  fmt::print("{} STACK:\n", funcName);
  for (const auto& c : stack) {
    fmt::print("\t");
    c.DebugPrint();
  }
}

static void PrintLocals(const std::vector<Value>& locals, const std::string& funcName)
{
  fmt::print("{} LOCALS:\n", funcName);
  for (const auto& value : locals) {
    value.DebugPrint();
  }
}

void VM::Start(bool verbose)
{
  auto mainHash = static_cast<std::int64_t>(m_Hasher("main"));
  auto fileHash = static_cast<std::int64_t>(m_Hasher("file"));
  CallStack callStack;
  if (m_FunctionList.find(mainHash) == m_FunctionList.end()) {
    RuntimeError("Could not find `main` function", InterpretError::FunctionNotFound, 1, {});
    return;
  }

  callStack.push_back(std::make_tuple(fileHash, mainHash, 1));
  Run(mainHash, 1, verbose, callStack, {}, 1);
}

std::pair<InterpretResult, Value> VM::Run(std::int64_t funcNameHash, 
    int startLine, 
    bool verbose, 
    CallStack& callStack, 
    const std::vector<Value>& args, 
    int depth
  )
{
#define PRINT_LOCAL_MEMORY()                                          \
  do {                                                                \
    if (verbose) {                                                    \
      PrintStack(valueStack, m_FunctionNames.at(funcNameHash));       \
      PrintLocals(localsList, m_FunctionNames.at(funcNameHash));      \
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

  auto& function = m_FunctionList.at(funcNameHash);
  auto& opList = function.m_OpList;
  auto& constantList = function.m_ConstantList;
  // locals list needs to be copied
  auto localsList = function.m_Locals;
  
  std::size_t opCurrent = 0, constantCurrent = 0;

  for (auto&& arg : args) {
    localsList.push_back(arg);
  }
  
  if (depth > s_CallstackLimit/* / 10*/) {
    RuntimeError(fmt::format("Maximum callstack depth {} exceeded", s_CallstackLimit), InterpretError::CallstackDepthExceeded, startLine, callStack);
    RETURN_ERR();
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
      case Ops::Negate: {
        auto c = Pop(valueStack);
        if (!HandleNegate(c, valueStack)) {
          RuntimeError(fmt::format("Cannot negate `{}`", c.GetType()), 
              InterpretError::InvalidType, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::Not: {
        auto c = Pop(valueStack);
        valueStack.emplace_back(!(c.AsBool()));
        break;
      }
      case Ops::LoadConstant:
        valueStack.push_back(constantList[constantCurrent++]);
        break;
      case Ops::LoadLocal: {
        auto id = valueStack.back().Get<std::int64_t>();
        valueStack.pop_back();
        auto value = localsList[id];
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
        auto calleeNameHash = valueStack.back().Get<std::int64_t>();
        valueStack.pop_back();
        
        if (m_FunctionList.find(calleeNameHash) == m_FunctionList.end()) {
          RuntimeError(fmt::format("Could not find function '{}'", calleeNameHash), InterpretError::FunctionNotFound, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }

        auto& calleeFunc = m_FunctionList.at(calleeNameHash);
        int arity = calleeFunc.m_Arity;
        auto numArgsGiven = valueStack.back().Get<std::int64_t>();
        valueStack.pop_back();

        if (numArgsGiven != arity) {
          RuntimeError(fmt::format("Incorrect number of arguments given to function '{}', expected {} but got {}", 
                calleeNameHash, arity, numArgsGiven), 
              InterpretError::IncorrectArgCount, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }

        // top of the stack will be the last function argument 
        std::vector<Value> callArgs(arity);
        for (auto i = 0; i < arity; i++) {
//          callArgs.push_back(std::move(valueStack.back()));
          callArgs[arity - 1 - i] = std::move(valueStack.back());
          valueStack.pop_back();
        }
        
        callStack.push_back(std::make_tuple(funcNameHash, calleeNameHash, line));
        auto [res, value] = Run(calleeNameHash, line, verbose, callStack, callArgs, depth + 1);
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
        localsList[valueStack.back().Get<std::int64_t>()] = value;
        valueStack.pop_back();
        break;
      }
      case Ops::DeclareLocal: {
        localsList.emplace_back();
        break;
      }
      case Ops::JumpIfFalse: {
        auto [constIdx, opIdx] = PopLastTwo(valueStack); 
        auto condition = Pop(valueStack);
        if (!condition.AsBool()) {
          opCurrent = opIdx.Get<std::int64_t>();
          constantCurrent = constIdx.Get<std::int64_t>();
        }
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
    }
  }

#ifdef GRACE_DEBUG 
  PRINT_LOCAL_MEMORY();
#endif

  RETURN_NULL();

#undef PRINT_LOCAL_MEMORY
#undef RETURN_ERR
#undef RETURN_NULL
#undef RETURN_VALUE
}

bool VM::AddFunction(const std::string& name, int line, std::vector<std::string>&& parameters)
{
  auto hash = static_cast<std::int64_t>(m_Hasher(name));
  auto [it, res] = m_FunctionList.try_emplace(hash, Function(hash, std::move(parameters), line));
  if (res) {
    m_LastFunctionHash = hash;
    m_FunctionNames.emplace(hash, name);
    return true;
  }
  return false;
}

void VM::RuntimeError(const std::string& message, InterpretError errorType, int line, const CallStack& callStack)
{
  fmt::print(stderr, "\n");
  
  fmt::print(stderr, "Call stack:\n");

  auto callStackSize = callStack.size();
  if (callStackSize > 15) {
    if (auto showFull = std::getenv("GRACE_SHOW_FULL_CALLSTACK")) {
      for (auto i = 1; i < callStack.size(); i++) {
        const auto& [caller, callee, ln] = callStack[i];
        fmt::print(stderr, "\tline {}, in {}:\n", ln, m_FunctionNames.at(caller));
        fmt::print(stderr, "\t    {}\n", m_Compiler.GetCodeAtLine(ln));
      }
    } else {
      fmt::print(stderr, "\t{} more calls before - set environment variable `GRACE_SHOW_FULL_CALLSTACK` to see full callstack\n", callStackSize - 15);
      for (auto i = callStackSize - 15; i < callStackSize; i++) {
        const auto& [caller, callee, ln] = callStack[i];
        fmt::print(stderr, "\tline {}, in {}:\n", ln, m_FunctionNames.at(caller));
        fmt::print(stderr, "\t    {}\n", m_Compiler.GetCodeAtLine(ln));
      }
    }
  } else {
    for (auto i = 1; i < callStack.size(); i++) {
      const auto& [caller, callee, ln] = callStack[i];
      fmt::print(stderr, "\tline {}, in {}:\n", ln, m_FunctionNames.at(caller));
      fmt::print(stderr, "\t    {}\n", m_Compiler.GetCodeAtLine(ln));
    }
  }

  fmt::print(stderr, "\tline {}, in {}:\n", line, m_FunctionNames.at(std::get<1>(callStack.back())));
  fmt::print(stderr, "\t    {}\n", m_Compiler.GetCodeAtLine(line));

  fmt::print(stderr, "\n");
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print(stderr, "[line {}] {}: {}. Stopping execution.\n", line, errorType, message);
}
