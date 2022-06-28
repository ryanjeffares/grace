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

#include <chrono>
#include <cstdlib>
#include <iterator>
#include <stack>
#include <utility>

#ifdef GRACE_MSC
# include <stdlib.h>    // getenv_s
#endif

#include "grace.hpp"

#include "scanner.hpp"
#include "vm.hpp"
#include "objects/grace_exception.hpp"
#include "objects/grace_iterator.hpp"
#include "objects/grace_dictionary.hpp"
#include "objects/grace_list.hpp"
#include "objects/object_tracker.hpp"

using namespace Grace::VM;

static GRACE_INLINE std::pair<Value, Value> PopLastTwo(std::vector<Value>& stack)
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

bool VM::AddFunction(std::string&& name, std::size_t line, std::size_t arity, const std::string& fileName, bool exported)
{
  auto hash = static_cast<std::int64_t>(m_Hasher(name));
  auto [it, res] = m_FunctionList.try_emplace(hash, Function(std::move(name), hash, arity, line, fileName, exported));
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

InterpretResult VM::Start(bool verbose, const std::vector<std::string>& args)
{
  using namespace std::chrono;
  auto start = steady_clock::now();
  auto res = Run(verbose, args);
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

InterpretResult VM::Run(GRACE_MAYBE_UNUSED bool verbose, const std::vector<std::string>& clArgs)
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

  std::vector<Value> argsAsValues;
  for (const auto& a : clArgs) {
    argsAsValues.emplace_back(a);
  }
  localsList.push_back(Value::CreateObject<Grace::GraceList>(std::move(argsAsValues)));

  std::size_t opCurrent = 0, constantCurrent = 0;
  
  auto& mainFunc = m_FunctionList.at(funcNameHash);

  std::vector<std::pair<std::size_t, std::size_t>> opConstOffsets;
  opConstOffsets.reserve(32);
  opConstOffsets.emplace_back(mainFunc.m_OpIndexStart, mainFunc.m_ConstantIndexStart);

  std::stack<std::size_t> localsOffsets;
  localsOffsets.push(0);

  CallStack callStack;
  callStack.emplace_back(static_cast<std::int64_t>(m_Hasher("file")), funcNameHash, 1, mainFunc.m_FileName);

  // used to restore the "state" of the VM before entering a try block
  // if an exception is caught
  struct VMState
  {
    std::size_t stackSize{}, numLocals{}, callStackSize{}, opOffsetSize{}, localsOffsetsSize{}, heldIteratorIndexesSize{}, namespaceStackSize{};
    std::int64_t opIndexToJump{}, constIndexToJump{};
  };

  std::stack<VMState> vmStateStack;
  std::stack<std::size_t> heldIteratorIndexes;
  std::stack<std::vector<std::string>> namespaceLookupStack; // used to keep track of what namespace we look for a call in
  namespaceLookupStack.emplace();

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
          valueStack.emplace_back(c1 == c2);
          break;
        }
        case Ops::NotEqual: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.emplace_back(c1 != c2);
          break;
        }
        case Ops::Greater: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.emplace_back(c1 > c2);
          break;
        }
        case Ops::GreaterEqual: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.emplace_back(c1 >= c2);
          break;
        }
        case Ops::Less: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.emplace_back(c1 < c2);
          break;
        }
        case Ops::LessEqual: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.emplace_back(c1 <= c2);
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
        case Ops::BitwiseAnd: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 & c2);
          break;
        }
        case Ops::BitwiseNot: {
          auto c1 = Pop(valueStack);
          valueStack.push_back(~c1);
          break;
        }
        case Ops::BitwiseOr: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 | c2);
          break;
        }
        case Ops::BitwiseXOr: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 ^ c2);
          break;
        }
        case Ops::ShiftLeft: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 << c2);
          break;
        }
        case Ops::ShiftRight: {
          auto [c1, c2] = PopLastTwo(valueStack);
          valueStack.push_back(c1 >> c2);
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
          auto targetNumLocals = m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top();
          localsList.resize(targetNumLocals);
          break;
        }
        case Ops::Print:
          Pop(valueStack).Print(false);
          break;
        case Ops::PrintEmptyLine:
          fmt::print("\n");
          break;
        case Ops::PrintLn:
          Pop(valueStack).PrintLn(false);
          break;
        case Ops::PrintTab:
          fmt::print("\t");
          break;
        case Ops::EPrint:
          Pop(valueStack).Print(true);
          break;
        case Ops::EPrintEmptyLine:
          fmt::print(stderr, "\n");
          break;
        case Ops::EPrintLn:
          Pop(valueStack).PrintLn(true);
          break;
        case Ops::EPrintTab:
          fmt::print(stderr, "\t");
          break;
        case Ops::AppendNamespace:
          namespaceLookupStack.top().push_back(m_FullConstantList[constantCurrent++].Get<std::string>());
          break;
        case Ops::StartNewNamespace:
          namespaceLookupStack.emplace();
          break;
        case Ops::Call: {
          auto calleeNameHash = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          auto numArgsGiven = static_cast<std::size_t>(m_FullConstantList[constantCurrent++].Get<std::int64_t>());

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

          const auto& calleeFunc = it->second;

          if (std::get<3>(callStack.back()) != calleeFunc.m_FileName) {
            const auto& currentNamespace = namespaceLookupStack.top();
            if (!calleeFunc.CompareNamespace(currentNamespace)) {
              if (currentNamespace.empty()) {
                throw GraceException(
                  GraceException::Type::FunctionNotFound,
                  fmt::format("function `{}` does not exist in this namespace", calleeFunc.m_Name)
                );
              } else {
                throw GraceException(
                  GraceException::Type::FunctionNotFound,
                  fmt::format("function `{}` is not a member of namespace `{}`", calleeFunc.m_Name, fmt::join(currentNamespace, "::"))
                );
              }
            }

            if (!calleeFunc.m_Exported) {
              throw GraceException(
                GraceException::Type::FunctionNotExported,
                fmt::format("function `{}` is not marked as `export`", calleeFunc.m_Name)
              );
            }
          }

          namespaceLookupStack.pop();

          auto arity = calleeFunc.m_Arity;

          if (numArgsGiven != arity) {            
            throw GraceException(
              GraceException::Type::IncorrectArgCount, 
              fmt::format("Incorrect number of arguments given to function '{}', expected {} but got {}", calleeFunc.m_Name, arity, numArgsGiven)
            );
          }

          localsOffsets.push(localsList.size());
          localsList.resize(localsList.size() + arity);
          for (std::size_t i = 0; i < arity; i++) {
            localsList[arity - i - 1 + localsOffsets.top()] = Pop(valueStack);
          }

          callStack.emplace_back(funcNameHash, calleeNameHash, line, calleeFunc.m_FileName);
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
            auto value = Pop(valueStack);
            args[arity - i - 1] = std::move(value);
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
        case Ops::AssignIteratorBegin: {
          auto value = Pop(valueStack);
          auto object = value.GetObject();

          if (object == nullptr || !object->IsIterable()) {
            throw GraceException(
              GraceException::Type::InvalidType,
              fmt::format("{} is not iterable", value.GetTypeName())
            );
          }

          auto twoIterators = m_FullConstantList[constantCurrent++].Get<bool>();
          auto iteratorId = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          if (auto list = dynamic_cast<GraceList*>(object)) {
            auto listIterator = Value::CreateObject<GraceIterator>(list, GraceIterator::IterableType::List);
            heldIteratorIndexes.push(valueStack.size());
            valueStack.push_back(listIterator);
            auto listIteratorObject = dynamic_cast<GraceIterator*>(listIterator.GetObject());
            localsList[iteratorId + localsOffsets.top()] = listIteratorObject->IsAtEnd() ? Value() : listIteratorObject->Value();
            if (twoIterators) {
              auto secondIteratorId = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
              localsList[secondIteratorId + localsOffsets.top()] = std::int64_t(0);
            } else {
              constantCurrent++;
            }
          } else if (auto dict = dynamic_cast<GraceDictionary*>(object)) {
            auto dictIterator = Value::CreateObject<GraceIterator>(dict, GraceIterator::IterableType::Dictionary);
            heldIteratorIndexes.push(valueStack.size());
            valueStack.push_back(dictIterator);
            auto dictIteratorObject = dynamic_cast<GraceIterator*>(dictIterator.GetObject());
            if (twoIterators) {
              auto secondIteratorId = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
              if (dictIteratorObject->IsAtEnd()) {
                localsList[iteratorId + localsOffsets.top()] = nullptr;
                localsList[secondIteratorId + localsOffsets.top()] = nullptr;
              } else {
                auto kvpObject = dynamic_cast<GraceKeyValuePair*>(dictIteratorObject->Value().GetObject());
                localsList[iteratorId + localsOffsets.top()] = kvpObject->Key();
                localsList[secondIteratorId + localsOffsets.top()] = kvpObject->Value();
              }
            } else {
              localsList[iteratorId + localsOffsets.top()] = dictIteratorObject->IsAtEnd() ? Value() : dictIteratorObject->Value();
              constantCurrent++;
            }
          } else {
            GRACE_ASSERT(false, "Object did not dynamic_cast to a valid iterable type");
          }
          break;
        }
        case Ops::IncrementIterator: {
          auto twoIterators = m_FullConstantList[constantCurrent++].Get<bool>();
          auto iteratorVarId = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          auto iteratorIndex = heldIteratorIndexes.top();
          auto heldIterator = dynamic_cast<GraceIterator*>(valueStack[iteratorIndex].GetObject());
          auto iterableType = heldIterator->GetType();

          if (iterableType == GraceIterator::IterableType::List) {
            heldIterator->Increment();
            localsList[iteratorVarId + localsOffsets.top()] = heldIterator->IsAtEnd() ? Value() : heldIterator->Value();
            if (twoIterators) {
              auto secondIteratorId = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
              auto& local = localsList[secondIteratorId + localsOffsets.top()]; 
              auto value = local.Get<std::int64_t>();
              local = value + 1;
            } else {
              constantCurrent++;
            }
          } else if (iterableType == GraceIterator::IterableType::Dictionary) {
            heldIterator->Increment();
            if (twoIterators) {
              auto secondIteratorId = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
              if (!heldIterator->IsAtEnd()) {
                auto kvpObject = dynamic_cast<GraceKeyValuePair*>(heldIterator->Value().GetObject());
                localsList[iteratorVarId + localsOffsets.top()] = kvpObject->Key();
                localsList[secondIteratorId + localsOffsets.top()] = kvpObject->Value();
              } else {
                localsList[iteratorVarId + localsOffsets.top()] = nullptr;
                localsList[secondIteratorId + localsOffsets.top()] = nullptr;
              }
            } else {
              localsList[iteratorVarId + localsOffsets.top()] = heldIterator->IsAtEnd() ? Value() : heldIterator->Value();
              constantCurrent++;
            }
          } else {
            GRACE_UNREACHABLE();
          }
          break;
        }
        case Ops::CheckIteratorEnd: {
          auto heldIteratorIndex = heldIteratorIndexes.top();
          auto heldIterator = dynamic_cast<GraceIterator*>(valueStack[heldIteratorIndex].GetObject());

          GRACE_MAYBE_UNUSED auto iterableType = heldIterator->GetType();
          GRACE_ASSERT(iterableType == GraceIterator::IterableType::List || iterableType == GraceIterator::IterableType::Dictionary, "Iterator did not have a valid type set");

          valueStack.emplace_back(heldIterator->AsBool());
          break;
        }
        case Ops::DestroyHeldIterator: {
          auto index = heldIteratorIndexes.top();
          valueStack.erase(valueStack.begin() + index);
          heldIteratorIndexes.pop();
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
                  fmt::format("cannot cast `{}` as `int`", value.GetTypeName())
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
                  fmt::format("cannot cast `{}` as `float`", value.GetTypeName())
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
                  fmt::format("cannot cast `{}` as `char`", value.GetTypeName())
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
              case 7: {
                auto d = dynamic_cast<GraceDictionary*>(Pop(valueStack).GetObject());
                valueStack.emplace_back(d != nullptr);
                break;
              }
              default:
                GRACE_UNREACHABLE();
                break;
            }
          }
          break;
        }
        case Ops::IsObject:
          valueStack.emplace_back(Pop(valueStack).GetObject() != nullptr);
          break;
        case Ops::Typename: {
          valueStack.emplace_back(Pop(valueStack).GetTypeName());
          break;
        }
        case Ops::Dup: {
          auto numDups = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          auto value = valueStack.back();
          valueStack.reserve(numDups);
          valueStack.insert(valueStack.end(), numDups, value);
          break;
        }
        case Ops::CreateDictionary: {
          auto numItems = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          if (numItems == 0) {
            valueStack.push_back(Value::CreateObject<GraceDictionary>());
            break;
          }
          GraceDictionary dict;
          for (std::int64_t i = 0; i < numItems; i++) {
            auto [key, value] = PopLastTwo(valueStack);
            dict.Insert(std::move(key), std::move(value));
          }
          valueStack.push_back(Value::CreateObject<GraceDictionary>(std::move(dict)));
          break;
        }
        case Ops::CreateList: {
          auto numItems = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
          if (numItems == 0) {
            valueStack.push_back(Value::CreateObject<GraceList>());
            break;
          }
          std::vector<Value> result(numItems);
          for (auto i = 0; i < numItems; i++) {
            result[numItems - i - 1] = Pop(valueStack);
          }
          valueStack.push_back(Value::CreateObject<GraceList>(std::move(result)));
          break;
        }
        case Ops::CreateRangeList: {
          auto increment = Pop(valueStack);
          auto max = Pop(valueStack);
          auto min = Pop(valueStack);

          if (!min.IsNumber()) {
            throw GraceException(
              GraceException::Type::InvalidType,
              fmt::format("All values in range expression must be numbers, got: {}", min.GetTypeName())
            );
          }

          if (!max.IsNumber()) {
            throw GraceException(
              GraceException::Type::InvalidType,
              fmt::format("All values in range expression must be numbers, got: {}", max.GetTypeName())
            );
          }

          if (!increment.IsNumber()) {
            throw GraceException(
              GraceException::Type::InvalidType,
              fmt::format("All values in range expression must be numbers, got: {}", increment.GetTypeName())
            );
          }

          valueStack.push_back(Value::CreateObject<GraceList>(min, max, increment));
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
            heldIteratorIndexes.size(),
            namespaceLookupStack.size(),
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
          auto message = Pop(valueStack);
          throw GraceException(GraceException::Type::ThrownException, message.AsString());
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
        const auto& vmState = vmStateStack.top();

        // we need to "unwind" the call stack back to its state before we entered the try block...
        while (heldIteratorIndexes.size() != vmState.heldIteratorIndexesSize) {
          valueStack.erase(valueStack.begin() + heldIteratorIndexes.top());
          heldIteratorIndexes.pop();
        }

        while (localsOffsets.size() != vmState.localsOffsetsSize) {
          localsOffsets.pop();
        }

        valueStack.resize(vmState.stackSize);
        localsList.resize(vmState.numLocals);
        callStack.resize(vmState.callStackSize);
        opConstOffsets.resize(vmState.opOffsetSize);

        auto [opOffset, constOffset] = opConstOffsets.back();
        opCurrent = vmState.opIndexToJump + opOffset;
        constantCurrent = vmState.constIndexToJump + constOffset;

        funcNameHash = std::get<1>(callStack.back());

        valueStack.push_back(Value::CreateObject<GraceException>(ge));

        while (namespaceLookupStack.size() != vmState.namespaceStackSize) {
          namespaceLookupStack.pop();
        }
      } else {
        // exception unhandled, report the error and quit
        RuntimeError(ge, line, callStack);
#ifdef GRACE_DEBUG
        PRINT_LOCAL_MEMORY();
#endif
        valueStack.clear();
        localsList.clear();
        return InterpretResult::RuntimeError;
      }
    }
  }

exit:

  valueStack.clear();
  localsList.clear();

#ifdef GRACE_DEBUG 
  PRINT_LOCAL_MEMORY();
  ObjectTracker::Finalise();
#endif

  return InterpretResult::RuntimeOk;

#undef PRINT_LOCAL_MEMORY
}

void VM::RuntimeError(const GraceException& exception, std::size_t line, const CallStack& callStack)
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
        const auto& [caller, callee, ln, fileName] = callStack[i];
        const auto& callerFunc = m_FunctionList.at(caller);
        fmt::print(stderr, "in {}:{}:{}\n", fileName, callerFunc.m_Name, ln);
        fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(fileName, ln));
      }
    } else {
      fmt::print(stderr, "{} more calls before - set environment variable `GRACE_SHOW_FULL_CALLSTACK` to see full callstack\n", callStackSize - 15);
      for (auto i = callStackSize - 15; i < callStackSize; i++) {
        const auto& [caller, callee, ln, fileName] = callStack[i];
        const auto& callerFunc = m_FunctionList.at(caller);
        fmt::print(stderr, "in {}:{}:{}\n", fileName, callerFunc.m_Name, ln);
        fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(fileName, ln));
      }
    }
  } else {
    for (std::size_t i = 1; i < callStack.size(); i++) {
      const auto& [caller, callee, ln, fileName] = callStack[i];
      const auto& callerFunc = m_FunctionList.at(caller);
      fmt::print(stderr, "in {}:{}:{}\n", fileName, callerFunc.m_Name, ln);
      fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(fileName, ln));
    }
  }

  const auto& calleeFunc = m_FunctionList.at(std::get<1>(callStack.back()));
  fmt::print(stderr, "in {}:{}:{}\n", calleeFunc.m_FileName, calleeFunc.m_Name, line);
  fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(calleeFunc.m_FileName, line));

  fmt::print(stderr, "\n");
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print(stderr, "[line {}] {}. Stopping execution.\n", line, exception.ToString());
}
