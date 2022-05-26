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

#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <stack>
#include <chrono>
#include <utility>

#include "grace.hpp"

#ifdef GRACE_MSC
# include <stdlib.h>    // getenv_s
#endif

#include "scanner.hpp"
#include "vm.hpp"
#include "objects/grace_list.hpp"
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

#ifdef GRACE_DEBUG
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
#endif

VM::VM()
{
  RegisterNatives();
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

bool VM::CombineFunctions(GRACE_MAYBE_UNUSED bool verbose)
{
  auto mainHash = static_cast<std::int64_t>(m_Hasher("main"));
  auto it = m_FunctionList.find(mainHash);
  if (it == m_FunctionList.end()) {
    fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
    fmt::print(stderr, "Could not find `main` function in file, execution cannot proceed.\n");
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

InterpretResult VM::Start(bool verbose)
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
  return res;
}

InterpretResult VM::Run(GRACE_MAYBE_UNUSED bool verbose)
{
#define PRINT_LOCAL_MEMORY()                                          \
  do {                                                                \
    if (verbose) {                                                    \
      PrintStack(valueStack, m_FunctionList.at(funcNameHash).m_Name); \
      PrintLocals(localsList, m_FunctionList.at(funcNameHash).m_Name);\
    }                                                                 \
  } while (false)                                                     \

  auto funcNameHash = static_cast<std::int64_t>(m_Hasher("main"));
  std::vector<Value> valueStack, localsList;
  valueStack.reserve(16);
  localsList.reserve(16);

  std::size_t opCurrent = 0, constantCurrent = 0;
  
  auto& mainFunc = m_FunctionList.at(funcNameHash);

  std::vector<std::pair<std::size_t, std::size_t>> opConstOffsets, constantOffsets;
  opConstOffsets.reserve(32);
  opConstOffsets.emplace_back(mainFunc.m_OpIndexStart, mainFunc.m_ConstantIndexStart);

  std::stack<std::size_t> localsOffsets;
  localsOffsets.push(0);

  CallStack callStack;
  callStack.emplace_back(static_cast<std::int64_t>(m_Hasher("file")), funcNameHash, 1);

  // used to restore the "state" of the VM before entering a try block
  // if an exception is caught
  struct VMState
  {
    std::size_t stackSize{}, numLocals{}, callStackSize{}, opOffsetSize{}, localsOffsets{};
    std::int64_t opIndexToJump{}, constIndexToJump{};
  };

  std::stack<VMState> vmStateStack;

  bool inTryBlock = false;

#ifdef GRACE_DEBUG
  if (verbose) {
    ObjectTracker::EnableVerbose();
  }
#endif

  while (true) {
    auto [op, line] = m_FullOpList[opCurrent++];

    try {

      switch (op) {
        case Ops::Add: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 + c2);
          break;
        }
        case Ops::Subtract: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 - c2);
          break;
        }
        case Ops::Multiply: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 * c2);
          break;
        }
        case Ops::Mod: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 % c2);
          break;
        }
        case Ops::Divide: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 / c2);
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
          valueStack.push_back(c1 == c2);
          break;
        }
        case Ops::NotEqual: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 != c2);
          break;
        }
        case Ops::Greater: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 > c2);
          break;
        }
        case Ops::GreaterEqual: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 >= c2);
          break;
        }
        case Ops::Less: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 < c2);
          break;
        }
        case Ops::LessEqual: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 <= c2);
          break;
        }
        case Ops::Pow: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1.Pow(c2));
          break;
        }
        case Ops::Negate: {
          auto c = Pop(valueStack);
          valueStack.push_back(-c);
          break;
        }
        case Ops::Not: {
          auto c = Pop(valueStack);
          valueStack.push_back(!c);
          break;
        }
        case Ops::LoadConstant:
          valueStack.push_back(m_FullConstantList[constantCurrent++]);
          break;
        case Ops::LoadLocal: {
          auto id = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          auto& value = localsList[id + localsOffsets.top()];
          valueStack.push_back(value);
          break;
        }
        case Ops::Pop:
          valueStack.pop_back();
          break;
        case Ops::PopLocal:
          localsList.pop_back();
          break;
        case Ops::PopLocals: {
          auto shouldPop = m_FullConstantList[constantCurrent++].Get<bool>();
          if (!shouldPop) {
            break;
          }
          auto targetNumLocals = m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top();
          localsList.resize(targetNumLocals);
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
          auto calleeNameHash = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          auto numArgsGiven = m_FullConstantList[constantCurrent++].Get<std::int64_t>();

          auto it = m_FunctionList.find(calleeNameHash);
          if (it == m_FunctionList.end()) {
            throw GraceException(
              GraceException::Type::FunctionNotFound, 
              fmt::format("cannot find function `{}`", m_FullConstantList[constantCurrent++].Get<std::string>())
            );
          }
          
          // increment this here to get past the string function name
          // we only put it in the constant list so we can report a nice error
          constantCurrent++;

          auto& calleeFunc = it->second;
          int arity = calleeFunc.m_Arity;          

          if (numArgsGiven != arity) {            
            throw GraceException(
              GraceException::Type::IncorrectArgCount, 
              fmt::format("Incorrect number of arguments given to function '{}', expected {} but got {}", calleeFunc.m_Name, arity, numArgsGiven)
            );
          }

          localsOffsets.push(localsList.size());
          localsList.resize(localsList.size() + arity);
          for (auto i = 0; i < arity; i++) {
            localsList[arity - i - 1 + localsOffsets.top()] = Pop(valueStack);
          }

          callStack.emplace_back(funcNameHash, calleeNameHash, line);
          valueStack.emplace_back(static_cast<std::int64_t>(opCurrent));
          valueStack.emplace_back(static_cast<std::int64_t>(constantCurrent));

          opCurrent = calleeFunc.m_OpIndexStart;
          constantCurrent = calleeFunc.m_ConstantIndexStart; 
          opConstOffsets.emplace_back(opCurrent, constantCurrent);
          
          funcNameHash = calleeNameHash;
          break;
        }
        case Ops::NativeCall: {
          auto calleeIndex = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          auto& calleeFunc = m_NativeFunctions[calleeIndex];
          auto arity = calleeFunc.GetArity();
          auto numArgsGiven = m_FullConstantList[constantCurrent++].Get<std::int64_t>();

          if (numArgsGiven != arity) {            
            throw GraceException(
              GraceException::Type::IncorrectArgCount, 
              fmt::format("Incorrect number of arguments given to function '{}', expected {} but got {}", calleeFunc.GetName(), arity, numArgsGiven)
            );
          }

          std::vector<Value> args(arity);
          for (std::uint32_t i = 0; i < arity; i++) {
            args[arity - i - 1] = Pop(valueStack);
          }

          auto res = calleeFunc(args);
          valueStack.push_back(std::move(res));
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
          auto [opOffset, constOffset] = opConstOffsets.back();
          opCurrent = opIdx + opOffset;
          constantCurrent = constIdx + constOffset;
          break;
        }
        case Ops::JumpIfFalse: {
          auto constIdx = m_FullConstantList[constantCurrent++].Get<std::int64_t>(); 
          auto opIdx = m_FullConstantList[constantCurrent++].Get<std::int64_t>(); 
          auto condition = Pop(valueStack);
          if (!condition.AsBool()) {
            auto [opOffset, constOffset] = opConstOffsets.back();
            opCurrent = opIdx + opOffset;
            constantCurrent = constIdx + constOffset;
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
          opConstOffsets.pop_back();
          constantOffsets.pop_back();

#ifdef GRACE_DEBUG
          PRINT_LOCAL_MEMORY();
#endif
          break;
        }
        case Ops::Cast: {
          auto value = Pop(valueStack);
          auto type = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          switch (type) {
            case 0: {
              std::int64_t result;
              auto [success, message] = value.AsInt(result);
              if (success) {
                valueStack.emplace_back(result);
              } else {                
                throw GraceException(
                  GraceException::Type::InvalidCast,
                  fmt::format("cannot cast `{}` as `int`", value.GetType())
                );
              }
              break;
            }
            case 1: {
              double result;
              auto [success, message] = value.AsDouble(result);
              if (success) {
                valueStack.emplace_back(result);
              } else {
                throw GraceException(
                  GraceException::Type::InvalidCast,
                  fmt::format("cannot cast `{}` as `float`", value.GetType())
                );                
              }
              break;
            }
            case 2:
              valueStack.emplace_back(value.AsBool());
              break;
            case 3:
              valueStack.emplace_back(value.AsString());
              break;
            case 4: {
              char result;
              auto [success, message] = value.AsChar(result);
              if (success) {
                valueStack.emplace_back(result);
              } else {
                throw GraceException(
                  GraceException::Type::InvalidCast,
                  fmt::format("cannot cast `{}` as `char`", value.GetType())
                );                
              }
              break;
            }
            case 5:
              valueStack.emplace_back(Value::CreateObject<GraceList>(value));
              break;
            default:
              GRACE_UNREACHABLE();
              break;
          }
          break;
        }
        case Ops::CheckType: {
          auto typeIdx = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          if (typeIdx < 6) {
            valueStack.emplace_back(typeIdx == static_cast<std::int64_t>(Pop(valueStack).GetType()));
          } else {
            switch (typeIdx) {
              case 6: {
                auto l = dynamic_cast<GraceList*>(Pop(valueStack).GetObject());
                valueStack.emplace_back(l != nullptr);
                break;
              }
              default:
                GRACE_UNREACHABLE();
                break;
            }
          }
          break;
        }
        case Ops::Dup: {
          auto numDups = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          auto value = valueStack.back();
          valueStack.reserve(numDups);
          valueStack.insert(valueStack.end(), numDups, value);
          break;
        }
        case Ops::CreateList: {
          auto numItems = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          std::vector<Value> result(numItems);
          for (auto i = 0; i < numItems; i++) {
            result[numItems - i - 1] = Pop(valueStack);
          }
          valueStack.push_back(Value::CreateObject<GraceList>(std::move(result)));
          break;
        }
        case Ops::CreateEmptyList: {
          valueStack.push_back(Value::CreateObject<GraceList>());
          break;
        }
        case Ops::CreateRepeatingList: {
          auto numItems = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          auto value = Pop(valueStack);
          valueStack.push_back(Value::CreateObject<GraceList>(value, numItems));
          break;
        }
        case Ops::Assert: {
          auto condition = Pop(valueStack);
          if (!condition.AsBool()) {
            RuntimeError(GraceException(
              GraceException::Type::AssertionFailed, "assertion failed"),
              line,
              callStack
            );
            goto exit;       
          }
          break;
        }
        case Ops::AssertWithMessage: {
          auto condition = Pop(valueStack);
          auto message = m_FullConstantList[constantCurrent++].Get<std::string>();
          if (!condition.AsBool()) {
            RuntimeError(GraceException(
              GraceException::Type::AssertionFailed, fmt::format("assertion failed: {}", message)),
              line,
              callStack
            );
            goto exit;
          }
          break;
        }
        case Ops::EnterTry: {
          inTryBlock = true;
          auto opIdx = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          auto constIdx = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          vmStateStack.push({
            valueStack.size(),
            localsList.size(),
            callStack.size(),
            opConstOffsets.size(),
            localsOffsets.size(),
            opIdx,
            constIdx
          });
          break;
        }
        case Ops::ExitTry: {
          auto targetNumLocals = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          localsList.resize(targetNumLocals);
          vmStateStack.pop();
          inTryBlock = !vmStateStack.empty();
          break;
        }
        case Ops::Throw: {
          auto message = m_FullConstantList[constantCurrent++].Get<std::string>();
          throw GraceException(GraceException::Type::ThrownException, std::move(message));
        }
        case Ops::Exit: {
          goto exit;
        }
        default:
          GRACE_UNREACHABLE();
          break;
      }      

    } catch(const GraceException& ge) {
      if (inTryBlock) {
        // jump to the catch block and put the exception on the stack to be assigned
        auto vmState = vmStateStack.top();

        // we need to "unwind" the call stack back to its state before we entered the try block...
        valueStack.resize(vmState.stackSize);
        localsList.resize(vmState.numLocals);
        callStack.resize(vmState.callStackSize);
        opConstOffsets.resize(vmState.opOffsetSize);

        while (localsOffsets.size() != vmState.localsOffsets) {
          localsOffsets.pop();
        }

        auto [opOffset, constOffset] = opConstOffsets.back();
        opCurrent = vmState.opIndexToJump + opOffset;
        constantCurrent = vmState.constIndexToJump + constOffset;

        funcNameHash = std::get<1>(callStack.back());

        valueStack.push_back(Value::CreateObject<GraceException>(ge));
      } else {
        // exception unhandled, report the error and quit
        RuntimeError(ge, line, callStack);
#ifdef GRACE_DEBUG
        PRINT_LOCAL_MEMORY();
#endif
        localsList.clear();
        return InterpretResult::RuntimeError;
      }
    }
  }

exit:

  localsList.clear();

#ifdef GRACE_DEBUG 
  PRINT_LOCAL_MEMORY();
  ObjectTracker::Finalise();
#endif

  return InterpretResult::RuntimeOk;

#undef PRINT_LOCAL_MEMORY
}

void VM::RuntimeError(const GraceException& exception, int line, const CallStack& callStack)
{
  fmt::print(stderr, "\n");
  
  fmt::print(stderr, "Call stack (most recent call last):\n");

  auto callStackSize = callStack.size();
  if (callStackSize > 15) {
#ifdef GRACE_MSC
    // std::getenv() produces a warning on Windows, therefore error with /W4 /WX
    std::size_t size;
    getenv_s(&size, NULL, 0, "GRACE_SHOW_FULL_CALLSTACK");
    if (size != 0) {
#else
    if (auto showFull = std::getenv("GRACE_SHOW_FULL_CALLSTACK")) {
#endif
      for (std::size_t i = 1; i < callStack.size(); i++) {
        const auto& [caller, callee, ln] = callStack[i];
        fmt::print(stderr, "line {}, in {}:\n", ln, m_FunctionList.at(caller).m_Name);
        fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(ln));
      }
    } else {
      fmt::print(stderr, "{} more calls before - set environment variable `GRACE_SHOW_FULL_CALLSTACK` to see full callstack\n", callStackSize - 15);
      for (auto i = callStackSize - 15; i < callStackSize; i++) {
        const auto& [caller, callee, ln] = callStack[i];
        fmt::print(stderr, "line {}, in {}:\n", ln, m_FunctionList.at(caller).m_Name);
        fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(ln));
      }
    }
  } else {
    for (std::size_t i = 1; i < callStack.size(); i++) {
      const auto& [caller, callee, ln] = callStack[i];
      fmt::print(stderr, "line {}, in {}:\n", ln, m_FunctionList.at(caller).m_Name);
      fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(ln));
    }
  }

  fmt::print(stderr, "line {}, in {}:\n", line, m_FunctionList.at(std::get<1>(callStack.back())).m_Name);
  fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(line));

  fmt::print(stderr, "\n");
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print(stderr, "[line {}] {}. Stopping execution.\n", line, exception.ToString());
}
