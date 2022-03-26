#include <cstdlib>
#include <stack>

#include "grace.hpp"

#include "compiler.hpp"
#include "vm.hpp"

using namespace Grace::VM;

static inline std::tuple<Value, Value> PopLastTwo(std::vector<Value>& stack)
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
  if (m_FunctionList.find(mainHash) == m_FunctionList.end()) {
    RuntimeError("Could not find `main` function", InterpretError::FunctionNotFound, 1, {});
    return;
  }

  using namespace std::chrono;
  auto start = steady_clock::now();
  auto res = Run(mainHash, 1, verbose);
  auto end = steady_clock::now();
  if (verbose) {
    if (res == InterpretResult::RuntimeOk) {
      auto dur = duration_cast<microseconds>(end - start).count();
      if (dur > 1000) {
        fmt::print("Program finished successfully in {} ms.\n", duration_cast<milliseconds>(end - start).count());
      } else {
        fmt::print("Program finished successfully in {} μs.\n", dur);
      }
    }
  }
}

InterpretResult VM::Run(std::int64_t funcNameHash, int startLine, bool verbose)
{
#define PRINT_LOCAL_MEMORY()                                          \
  do {                                                                \
    if (verbose) {                                                    \
      PrintStack(*valueStack, m_FunctionNames.at(funcNameHash));      \
      PrintLocals(*localsList, m_FunctionNames.at(funcNameHash));     \
    }                                                                 \
  } while (false)                                                     \

#define RETURN_ERR()                                                  \
  do {                                                                \
    localsList->clear();                                              \
    return InterpretResult::RuntimeError;                             \
  } while (false)                                                     \

#define RETURN_NULL()                                                 \
  do {                                                                \
    localsList->clear();                                              \
    return InterpretResult::RuntimeOk;                                \
  } while (false)                                                     \

#define RETURN_VALUE(value)                                           \
  do {                                                                \
    localsList.clear();                                               \
    return InterpretResult::RuntimeOk;                                \
  } while (false)                                                     \

  // Function, op index, constant index
  struct FunctionInfo 
  {
    Function& m_Function;
    std::size_t m_OpIndex, m_ConstantIndex;
    std::vector<Value> m_Locals;

    FunctionInfo(Function& func, std::size_t opIdx, std::size_t constIdx)
      : m_Function(func), m_OpIndex(opIdx), m_ConstantIndex(constIdx)
    {
    }
  };

  std::vector<FunctionInfo> funcInfoStack;
  funcInfoStack.reserve(64);
  funcInfoStack.emplace_back(m_FunctionList.at(funcNameHash), 0, 0);

  auto opCurrent = &funcInfoStack.back().m_OpIndex;
  auto constantCurrent = &funcInfoStack.back().m_ConstantIndex;
  auto opList = &funcInfoStack.back().m_Function.m_OpList;
  auto constantList = &funcInfoStack.back().m_Function.m_ConstantList;
  auto valueStack = &funcInfoStack.back().m_Function.m_ValueStack;
  auto localsList = &funcInfoStack.back().m_Locals;

  CallStack callStack;
  callStack.emplace_back(
      static_cast<std::int64_t>(m_Hasher("file")),
      static_cast<std::int64_t>(m_Hasher("main")),
      1
  );

  while (*opCurrent < opList->size()) {
    auto [op, line] = opList->at(*opCurrent);
    (*opCurrent)++;

    switch (op) {
      case Ops::Add: {
        auto [c1, c2] = PopLastTwo(*valueStack);
        if (!HandleAddition(c1, c2, *valueStack)) {
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
        auto [c1, c2] = PopLastTwo(*valueStack);
        if (!HandleSubtraction(c1, c2, *valueStack)) {
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
        auto [c1, c2] = PopLastTwo(*valueStack);
        if (!HandleMultiplication(c1, c2, *valueStack)) {
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
        auto [c1, c2] = PopLastTwo(*valueStack);
        if (!HandleDivision(c1, c2, *valueStack)) {
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
        auto [c1, c2] = PopLastTwo(*valueStack);
        valueStack->emplace_back(c1.AsBool() == c2.AsBool());
        break;
      }
      case Ops::Or: {
        auto [c1, c2] = PopLastTwo(*valueStack);
        valueStack->emplace_back(c1.AsBool() || c2.AsBool());
        break;
      }
      case Ops::Equal: {
        auto [c1, c2] = PopLastTwo(*valueStack);
        HandleEquality(c1, c2, *valueStack, true);
        break;
      }
      case Ops::NotEqual: {
        auto [c1, c2] = PopLastTwo(*valueStack);
        HandleEquality(c1, c2, *valueStack, false);
        break;
      }
      case Ops::Greater: {
        auto [c1, c2] = PopLastTwo(*valueStack);
        if (!HandleGreaterThan(c1, c2, *valueStack)) {
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
        auto [c1, c2] = PopLastTwo(*valueStack);
        if (!HandleGreaterEqual(c1, c2, *valueStack)) {
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
        auto [c1, c2] = PopLastTwo(*valueStack);
        if (!HandleLessThan(c1, c2, *valueStack)) {
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
        auto [c1, c2] = PopLastTwo(*valueStack);
        if (!HandleLessEqual(c1, c2, *valueStack)) {
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
        auto [c1, c2] = PopLastTwo(*valueStack);
        if (!HandlePower(c1, c2, *valueStack)) {
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
        auto c = Pop(*valueStack);
        if (!HandleNegate(c, *valueStack)) {
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
        auto c = Pop(*valueStack);
        valueStack->emplace_back(!(c.AsBool()));
        break;
      }
      case Ops::LoadConstant:
        // hot path here
        valueStack->push_back(std::move(constantList->at(*constantCurrent)));
        (*constantCurrent)++;
        break;
      case Ops::LoadLocal: {
        auto id = valueStack->back().Get<std::int64_t>();
        valueStack->pop_back();
        auto value = localsList->at(id);
        valueStack->push_back(value);
        break;
      }
      case Ops::Pop:
        valueStack->pop_back();
        break;
      case Ops::Print:
        valueStack->back().Print();
        break;
      case Ops::PrintEmptyLine:
        fmt::print("\n");
        break;
      case Ops::PrintLn:
        valueStack->back().PrintLn();
        break;
      case Ops::PrintTab:
        fmt::print("\t");
        break;
      case Ops::Call: {
        auto calleeNameHash = valueStack->back().Get<std::int64_t>();
        valueStack->pop_back();
        
        if (m_FunctionList.find(calleeNameHash) == m_FunctionList.end()) {
          RuntimeError(fmt::format("Could not find function '{}'", calleeNameHash), InterpretError::FunctionNotFound, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }

        auto& calleeFunc = m_FunctionList.at(calleeNameHash);
        int arity = calleeFunc.m_Arity;
        auto numArgsGiven = valueStack->back().Get<std::int64_t>();
        valueStack->pop_back();

        if (numArgsGiven != arity) {
          RuntimeError(fmt::format("Incorrect number of arguments given to function '{}', expected {} but got {}", calleeNameHash, arity, numArgsGiven), 
              InterpretError::IncorrectArgCount, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }

        callStack.emplace_back(funcNameHash, calleeNameHash, line);
        funcInfoStack.emplace_back(calleeFunc, 0, 0);

        // reassign pointers
        opCurrent = &funcInfoStack.back().m_OpIndex;
        constantCurrent = &funcInfoStack.back().m_ConstantIndex;
        opList = &funcInfoStack.back().m_Function.m_OpList;
        constantList = &funcInfoStack.back().m_Function.m_ConstantList;
        localsList = &funcInfoStack.back().m_Locals;
        
        for (auto i = 0; i < arity; i++) {
          localsList->push_back(std::move(valueStack->back()));
          valueStack->pop_back();
        }

        valueStack = &funcInfoStack.back().m_Function.m_ValueStack;
        funcNameHash = calleeNameHash;

        break;
      }
      case Ops::AssignLocal: {
        auto value = std::move(valueStack->back());
        valueStack->pop_back();
        localsList->at(valueStack->back().Get<std::int64_t>()) = value;
        valueStack->pop_back();
        break;
      }
      case Ops::DeclareLocal: {
        localsList->emplace_back();
        break;
      }
      case Ops::JumpIfFalse: {
        auto [constIdx, opIdx] = PopLastTwo(*valueStack);
        auto condition = Pop(*valueStack);
        if (!condition.AsBool()) {
          *opCurrent = opIdx.Get<std::int64_t>();
          *constantCurrent = constIdx.Get<std::int64_t>();
        }
        break;
      }
      case Ops::Return: {
        auto returnValue = std::move(valueStack->back());
        valueStack->pop_back();

#ifdef GRACE_DEBUG
        PRINT_LOCAL_MEMORY();
#endif
        
        // hot path here
        funcInfoStack.pop_back();
        callStack.pop_back();

        // reassign pointers
        opCurrent = &funcInfoStack.back().m_OpIndex;
        constantCurrent = &funcInfoStack.back().m_ConstantIndex;
        opList = &funcInfoStack.back().m_Function.m_OpList;
        constantList = &funcInfoStack.back().m_Function.m_ConstantList;
        valueStack = &funcInfoStack.back().m_Function.m_ValueStack;
        localsList = &funcInfoStack.back().m_Locals;

        funcNameHash = funcInfoStack.back().m_Function.m_NameHash;
        valueStack->push_back(std::move(returnValue));

#ifdef GRACE_DEBUG
        PRINT_LOCAL_MEMORY();
#endif
        break;
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

[[nodiscard]] 
bool VM::HandleAddition(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() + c2.Get<std::int64_t>());
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) + c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() + static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() + c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Char) {
        std::string res;
        res.push_back(c1.Get<char>());
        res.push_back(c2.Get<char>());
        stack.emplace_back(res);
        return true;
      }
      return false;
    }
    case Value::Type::String: {
      switch (c2.GetType()) {
        case Value::Type::String: {
          stack.emplace_back(c1.Get<std::string>() + c2.Get<std::string>());
          return true;
        }
        case Value::Type::Char: {
          stack.emplace_back(c1.Get<std::string>() + c2.Get<char>());
          return true;
        }
        default: {
          stack.emplace_back(c1.Get<std::string>() + c2.ToString());
          return true;
        }
      }
    }
    default:
      return false;
  }
}

[[nodiscard]]
bool VM::HandleSubtraction(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() - c2.Get<std::int64_t>());
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) - c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() - static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() - c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    default:
      return false;
  }
}

[[nodiscard]]
bool VM::HandleDivision(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() / c2.Get<std::int64_t>());
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) / c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() / static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() / c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    default:
      return false;
  }
}

[[nodiscard]]
bool VM::HandleMultiplication(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() * c2.Get<std::int64_t>());
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) * c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() * static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() * c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Int) {
        stack.emplace_back(std::string(c2.Get<std::int64_t>(), c1.Get<char>()));
        return true;
      }
      return false;
    }
    case Value::Type::String: {
      if (c2.GetType() == Value::Type::Int) {
        std::string res;
        for (auto i = 0; i < c2.Get<std::int64_t>(); i++) {
          res += c1.Get<std::string>();
        }
        stack.emplace_back(res);
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

void VM::HandleEquality(const Value& c1, const Value& c2, std::vector<Value>& stack, bool equal)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Double: {
          stack.emplace_back(equal 
              ? static_cast<double>(c1.Get<std::int64_t>()) == c2.Get<double>()
              : static_cast<double>(c1.Get<std::int64_t>()) != c2.Get<double>());
          break;
        }
        case Value::Type::Int: {
          stack.emplace_back(equal 
              ? c1.Get<std::int64_t>() == c2.Get<std::int64_t>()
              : c1.Get<std::int64_t>() != c2.Get<std::int64_t>());
          break;
        }
        default: {
          stack.emplace_back(false);
          break;
        }
      }
      break;
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(equal 
              ? c1.Get<double>() == static_cast<double>(c2.Get<std::int64_t>())
              : c1.Get<double>() != static_cast<double>(c2.Get<std::int64_t>()));
          break;
        }
        case Value::Type::Double: {
          stack.emplace_back(equal 
              ? c1.Get<double>() == c2.Get<double>()
              : c1.Get<double>() != c2.Get<double>());
          break;
        }
        default: {
          stack.emplace_back(false);
          break;
        }
      }
      break;
    }
    case Value::Type::Bool: {
      if (c2.GetType() == Value::Type::Bool) {
        stack.emplace_back(equal 
            ? c1.Get<bool>() == c2.Get<bool>()
            : c1.Get<bool>() != c2.Get<bool>());
      } else {
        stack.emplace_back(false);
      }
      break;
    }
    case Value::Type::Char: {
      switch (c2.GetType()) {
        case Value::Type::String: {
          stack.emplace_back(c2.Get<std::string>().length() == 1 
              && equal 
              ? c1.Get<char>() == c2.Get<std::string>()[0]
              : c1.Get<char>() != c2.Get<std::string>()[0]);
          break;
        }
        case Value::Type::Char: {
          stack.emplace_back(c1.Get<std::string>().length() == 1 
              && equal
              ? c1.Get<char>() == c2.Get<char>()
              : c1.Get<char>() != c2.Get<char>());
          break;
        }
        default: {
          stack.emplace_back(false);
          break;
        }
      }
      break;
    }
    case Value::Type::String: {
      switch (c2.GetType()) {
        case Value::Type::String: {
          stack.emplace_back(equal 
              ? c1.Get<std::string>() == c2.Get<std::string>()
              : c1.Get<std::string>() != c2.Get<std::string>());
          break;
        }
        case Value::Type::Char: {
          stack.emplace_back(c1.Get<std::string>().length() == 1 
              && equal
              ? c1.Get<std::string>()[0] == c2.Get<char>()
              : c1.Get<std::string>()[0] != c2.Get<char>());
          break;
        }
        default:
          break;
      }
      break;
    }
    case Value::Type::Null: {
      switch (c2.GetType()) {
        case Value::Type::Null: {
          stack.emplace_back(true);
          break;
        }
        default:
          stack.emplace_back(false);
          break;
      }
      break;
    }
    default:
      stack.emplace_back(false);
      break;
  }
}

[[nodiscard]]
bool VM::HandleLessThan(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) < c2.Get<double>());
          return true;
        }
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() < c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() < static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() < c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Char) {
        stack.emplace_back(c1.Get<char>() < c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
bool VM::HandleLessEqual(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) <= c2.Get<double>());
          return true;
        }
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() <= c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() <= static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() <= c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Char) {
        stack.emplace_back(c1.Get<char>() <= c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
bool VM::HandleGreaterThan(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) > c2.Get<double>());
          return true;
        }
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() > c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() > static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() > c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Char) {
        stack.emplace_back(c1.Get<char>() > c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
bool VM::HandleGreaterEqual(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) >= c2.Get<double>());
          return true;
        }
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() >= c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() >= static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() >= c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Char) {
        stack.emplace_back(c1.Get<char>() >= c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
bool VM::HandlePower(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(std::pow(c1.Get<std::int64_t>(), c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(std::pow(static_cast<double>(c1.Get<std::int64_t>()), c2.Get<double>()));
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(std::pow(c1.Get<double>(), static_cast<double>(c2.Get<std::int64_t>())));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(std::pow(c1.Get<double>(), c2.Get<double>()));
          return true;
        }
        default:
          return false;
      }
    }
    default:
      return false;
  }
}

[[nodiscard]]
bool VM::HandleNegate(const Value& c, std::vector<Value>& stack)
{
  switch (c.GetType()) {
    case Value::Type::Int: {
      auto val = c.Get<std::int64_t>();
      stack.emplace_back(-val);
      return true;
    }
    case Value::Type::Double: {
      auto val = c.Get<double>();
      stack.emplace_back(-val);
      return true;
    }
    default:
      return false;
  }
}

