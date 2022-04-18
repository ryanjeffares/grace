/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the VM class, which executes compiled Grace bytecode, as well as some static helper methods.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <stack>
#include <chrono>

#include "grace.hpp"

#include "compiler.hpp"
#include "vm.hpp"
#include "objects/object_tracker.hpp"

using namespace Grace::VM;

static GRACE_INLINE std::tuple<Value, Value> PopLastTwo(std::vector<Value>& stack)
{
  auto c1 = std::move(stack[stack.size() - 2]);
  auto c2 = std::move(stack[stack.size() - 1]);
  stack.pop_back();
  stack.pop_back();
  return {std::move(c1), std::move(c2)};
}

static GRACE_INLINE Value Pop(std::vector<Value>& stack)
{
  auto c = std::move(stack.back());
  stack.pop_back();
  return c;
}

static void PrintStack(const std::vector<Value>& stack, const std::string& funcName)
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
    fmt::print("\t");
    value.DebugPrint();
  }
}

bool VM::AddFunction(std::string&& name, int line, int arity)
{
  auto hash = static_cast<std::int64_t>(m_Hasher(name));
  auto [it, res] = m_FunctionList.try_emplace(hash, Function(std::move(name), hash, arity, line));
  if (res) {
    m_LastFunctionHash = hash;
    return true;
  }
  return false;
}

bool VM::CombineFunctions(bool verbose)
{
  auto mainHash = static_cast<std::int64_t>(m_Hasher("main"));
  auto it = m_FunctionList.find(mainHash);
  if (it == m_FunctionList.end()) {
    RuntimeError("Could not find `main` function", InterpretError::FunctionNotFound, 1, {});
    return false;
  }

  m_FullOpList = it->second.m_OpList;
  m_FullConstantList = it->second.m_ConstantList;
  for (auto& [name, func] : m_FunctionList) {
    if (name == mainHash) {
      continue;
    }

    auto& opList = func.m_OpList;
    func.m_OpIndexStart = m_FullOpList.size();
    m_FullOpList.reserve(opList.size());
    m_FullOpList.insert(m_FullOpList.end(), opList.begin(), opList.end());
    
    auto& constantList = func.m_ConstantList;
    func.m_ConstantIndexStart = m_FullConstantList.size();
    m_FullConstantList.reserve(constantList.size());
    m_FullConstantList.insert(m_FullConstantList.end(), constantList.begin(), constantList.end());
  }

#ifdef GRACE_DEBUG
  if (verbose) { 
    fmt::print("FULL OP LIST:\n");
    for (auto [op, line] : m_FullOpList) {
      fmt::print("{:>5} | {}\n", line, op);
    }
  }
#endif

  return true;
}

void VM::Start(bool verbose)
{
  using namespace std::chrono;
  auto start = steady_clock::now();
  auto res = Run(verbose);
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

InterpretResult VM::Run(bool verbose)
{
#define PRINT_LOCAL_MEMORY()                                          \
  do {                                                                \
    if (verbose) {                                                    \
      PrintStack(valueStack, m_FunctionList.at(funcNameHash).m_Name); \
      PrintLocals(localsList, m_FunctionList.at(funcNameHash).m_Name);\
    }                                                                 \
  } while (false)                                                     \

#define RETURN_ERR()                                                  \
  do {                                                                \
    localsList.clear();                                               \
    return InterpretResult::RuntimeError;                             \
  } while (false)                                                     \

#define RETURN_ASSERT_FAILED()                                        \
  do {                                                                \
    localsList.clear();                                               \
    return InterpretResult::RuntimeAssertionFailed;                   \
  } while (false)                                                     \

  auto funcNameHash = m_Hasher("main");
  std::vector<Value> valueStack, localsList;

  std::size_t opCurrent = 0, constantCurrent = 0;
  std::stack<std::size_t> localsOffsets;
  localsOffsets.push(0);

  CallStack callStack;
  callStack.emplace_back(static_cast<std::int64_t>(m_Hasher("file")), funcNameHash, 1);

  while (opCurrent < m_FullOpList.size()) {
    auto [op, line] = m_FullOpList[opCurrent++];

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
      case Ops::Mod: {
        auto [c1, c2] = PopLastTwo(valueStack);
        if (!HandleMod(c1, c2, valueStack)) {
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
        valueStack.emplace_back(c1.AsBool() && c2.AsBool());
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
        // hot path here
        valueStack.push_back(m_FullConstantList[constantCurrent++]);
        break;
      case Ops::LoadLocal: {
        auto id = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
        auto value = localsList[id + localsOffsets.top()];
        valueStack.push_back(value);
        break;
      }
      case Ops::Pop:
        valueStack.pop_back();
        break;
      case Ops::PopLocal:
        localsList.pop_back();
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
        auto calleeNameHash = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
        
        if (m_FunctionList.find(calleeNameHash) == m_FunctionList.end()) {
          RuntimeError(fmt::format("Could not find function '{}'", calleeNameHash), InterpretError::FunctionNotFound, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }

        auto& calleeFunc = m_FunctionList.at(calleeNameHash);
        int arity = calleeFunc.m_Arity;
        auto numArgsGiven = m_FullConstantList[constantCurrent++].Get<std::int64_t>();

        if (numArgsGiven != arity) {
          RuntimeError(fmt::format("Incorrect number of arguments given to function '{}', expected {} but got {}", calleeNameHash, arity, numArgsGiven), 
              InterpretError::IncorrectArgCount, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }

        callStack.emplace_back(funcNameHash, calleeNameHash, line);

        localsOffsets.push(localsList.size());
        localsList.resize(localsList.size() + arity);
        for (auto i = 0; i < arity; i++) {
          localsList[arity - i - 1 + localsOffsets.top()] = Pop(valueStack);
        }

        valueStack.emplace_back(static_cast<std::int64_t>(opCurrent));
        valueStack.emplace_back(static_cast<std::int64_t>(constantCurrent));

        opCurrent = calleeFunc.m_OpIndexStart;
        constantCurrent = calleeFunc.m_ConstantIndexStart; 
        
        funcNameHash = calleeNameHash;

        break;
      }
      case Ops::AssignLocal: {
        auto value = Pop(valueStack);
        localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] = std::move(value);
        break;
      }
      case Ops::DeclareLocal: {
        localsList.emplace_back();
        break;
      }
      case Ops::Jump: {
        auto constIdx = m_FullConstantList[constantCurrent++].Get<std::int64_t>(); 
        auto opIdx = m_FullConstantList[constantCurrent++].Get<std::int64_t>(); 
        opCurrent = opIdx + m_FunctionList.at(funcNameHash).m_OpIndexStart;
        constantCurrent = constIdx + m_FunctionList.at(funcNameHash).m_ConstantIndexStart;
        break;
      }
      case Ops::JumpIfFalse: {
        auto constIdx = m_FullConstantList[constantCurrent++].Get<std::int64_t>(); 
        auto opIdx = m_FullConstantList[constantCurrent++].Get<std::int64_t>(); 
        auto condition = Pop(valueStack);
        if (!condition.AsBool()) {
          opCurrent = opIdx + m_FunctionList.at(funcNameHash).m_OpIndexStart;
          constantCurrent = constIdx + m_FunctionList.at(funcNameHash).m_ConstantIndexStart;
        }
        break;
      }
      case Ops::Return: {
        auto returnValue = Pop(valueStack);

#ifdef GRACE_DEBUG
        PRINT_LOCAL_MEMORY();
#endif
        
        funcNameHash = std::get<0>(callStack.back());
        callStack.pop_back();

        constantCurrent = static_cast<std::size_t>(Pop(valueStack).Get<std::int64_t>());
        opCurrent = static_cast<std::size_t>(Pop(valueStack).Get<std::int64_t>());

        valueStack.emplace_back(std::move(returnValue));
        localsOffsets.pop();

#ifdef GRACE_DEBUG
        PRINT_LOCAL_MEMORY();
#endif
        break;
      }
      case Ops::CastAsInt: {
        auto value = Pop(valueStack);
        std::int64_t result;
        auto [success, message] = value.AsInt(result);
        if (success) {
          valueStack.emplace_back(result);
        } else {
          RuntimeError(message.value(), InterpretError::InvalidCast, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::CastAsFloat: {
        auto value = Pop(valueStack);
        double result;
        auto [success, message] = value.AsDouble(result);
        if (success) {
          valueStack.emplace_back(result);
        } else {
          RuntimeError(message.value(), InterpretError::InvalidCast, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::CastAsBool: {
        auto value = Pop(valueStack);
        valueStack.emplace_back(value.AsBool());
        break;
      }
      case Ops::CastAsString: {
        auto value = Pop(valueStack);
        valueStack.emplace_back(value.AsString());
        break;
      }
      case Ops::CastAsChar: {
        auto value = Pop(valueStack);
        char result;
        auto [success, message] = value.AsChar(result);
        if (success) {
          valueStack.emplace_back(result);
        } else {
          RuntimeError(message.value(), InterpretError::InvalidCast, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ERR();
        }
        break;
      }
      case Ops::CheckType: {
        auto typeIdx = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
        valueStack.emplace_back(typeIdx == static_cast<std::int64_t>(Pop(valueStack).GetType()));
        break;
      }
      case Ops::CreateList: {
        auto numItems = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
        auto offset = valueStack.size() - numItems;
        std::vector<Value> result;
        result.insert(result.end(), std::make_move_iterator(valueStack.begin() + offset), std::make_move_iterator(valueStack.end()));
        valueStack.erase(valueStack.begin() + offset, valueStack.end());
        valueStack.emplace_back(Value::CreateList(std::move(result)));
        break;
      }
      case Ops::CreateEmptyList: {
        valueStack.emplace_back(Value::CreateList());
        break;
      }
      case Ops::Assert: {
        auto condition = Pop(valueStack);
        if (!condition.AsBool()) {
          RuntimeError("Assertion failed", InterpretError::AssertionFailed, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ASSERT_FAILED();
        }
        break;
      }
      case Ops::AssertWithMessage: {
        auto condition = Pop(valueStack);
        auto message = m_FullConstantList[constantCurrent++].Get<std::string>();
        if (!condition.AsBool()) {
          RuntimeError(message, InterpretError::AssertionFailed, line, callStack);
#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          RETURN_ASSERT_FAILED();
        }
        break;
      }
      case Ops::Exit: {
        opCurrent = m_FullOpList.size();
        break;
      }
      default:
        GRACE_UNREACHABLE();
        break;
    }
  }

  localsList.clear();

#ifdef GRACE_DEBUG 
  PRINT_LOCAL_MEMORY();
  GRACE_ASSERT(valueStack.empty(), "Unhandled data on the stack");
  ObjectTracker::Finalise();
#endif

  return InterpretResult::RuntimeOk;

#undef PRINT_LOCAL_MEMORY
#undef RETURN_ERR
#undef RETURN_ASSERT_FAILED
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
        fmt::print(stderr, "line {}, in {}:\n", ln, m_FunctionList.at(caller).m_Name);
        fmt::print(stderr, "{:>4}\n", m_Compiler.GetCodeAtLine(ln));
      }
    } else {
      fmt::print(stderr, "{} more calls before - set environment variable `GRACE_SHOW_FULL_CALLSTACK` to see full callstack\n", callStackSize - 15);
      for (auto i = callStackSize - 15; i < callStackSize; i++) {
        const auto& [caller, callee, ln] = callStack[i];
        fmt::print(stderr, "line {}, in {}:\n", ln, m_FunctionList.at(caller).m_Name);
        fmt::print(stderr, "{:>4}\n", m_Compiler.GetCodeAtLine(ln));
      }
    }
  } else {
    for (auto i = 1; i < callStack.size(); i++) {
      const auto& [caller, callee, ln] = callStack[i];
      fmt::print(stderr, "line {}, in {}:\n", ln, m_FunctionList.at(caller).m_Name);
      fmt::print(stderr, "{:>4}\n", m_Compiler.GetCodeAtLine(ln));
    }
  }

  fmt::print(stderr, "line {}, in {}:\n", line, m_FunctionList.at(std::get<1>(callStack.back())).m_Name);
  fmt::print(stderr, "{:>4}\n", m_Compiler.GetCodeAtLine(line));

  fmt::print(stderr, "\n");
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print(stderr, "[line {}] {}: {}. Stopping execution.\n", line, errorType, message);
}
 
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
          stack.emplace_back(c1.Get<std::string>() + c2.AsString());
          return true;
        }
      }
    }
    default:
      return false;
  }
}

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
        if (c2.Get<std::int64_t>() > 0) {
          for (auto i = 0; i < c2.Get<std::int64_t>(); i++) {
            res += c1.Get<std::string>();
          }
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

bool VM::HandleMod(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() % c2.Get<std::int64_t>());
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(std::fmod(static_cast<double>(c1.Get<std::int64_t>()), c2.Get<double>()));
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(std::fmod(c1.Get<double>(), static_cast<double>(c2.Get<std::int64_t>())));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(std::fmod(c1.Get<double>(), c2.Get<double>()));
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
          stack.emplace_back(equal
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

