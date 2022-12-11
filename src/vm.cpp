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
#include <filesystem>
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
#include "objects/grace_instance.hpp"
#include "objects/grace_iterator.hpp"
#include "objects/grace_dictionary.hpp"
#include "objects/grace_set.hpp"
#include "objects/grace_list.hpp"
#include "objects/grace_range.hpp"
#include "objects/object_tracker.hpp"

namespace Grace::VM
{
  std::unordered_map<std::int64_t, std::unordered_map<std::int64_t, std::shared_ptr<VM::Function>>> VM::m_FunctionLookup;
  std::unordered_map<std::size_t, std::vector<std::shared_ptr<VM::Function>>> VM::m_ExtensionMethodLookup;
  std::unordered_map<std::int64_t, std::string> VM::m_FileNameLookup;
  std::vector<Native::NativeFunction> VM::m_NativeFunctions;
  std::unordered_map<std::int64_t, std::unordered_map<std::int64_t, VM::Class>> VM::m_ClassLookup;
  std::vector<VM::OpLine> VM::m_FullOpList;
  std::vector<Value> VM::m_FullConstantList;
  std::int64_t VM::m_LastFileNameHash{};
  std::int64_t VM::m_LastFunctionHash{};
  std::hash<std::string> VM::m_Hasher;

  static std::pair<Value, Value> PopLastTwo(std::vector<Value>& stack)
  {
    auto c1 = std::move(stack[stack.size() - 2]);
    auto c2 = std::move(stack[stack.size() - 1]);
    stack.pop_back();
    stack.pop_back();
    return {std::move(c1), std::move(c2)};
  }

  static Value Pop(std::vector<Value>& stack)
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

  void VM::PrintOps()
  {
    for (const auto& [fileName, funcList] : m_FunctionLookup) {
      for (const auto& [name, func] : funcList) {
        fmt::print("<function `{}`> in file {}\n", func->name, m_FileNameLookup.at(fileName));
        for (const auto [op, line] : func->opList) {
          fmt::print("{:>5} | {}\n", line, op);
        }
      }
    }
  }

  bool VM::AddFunction(std::string&& name, std::size_t arity, const std::string& fileName, bool exported, bool extension, std::size_t objectNameHash)
  {
    auto funcNameHash = static_cast<std::int64_t>(m_Hasher(name));
    auto fileNameHash = static_cast<std::int64_t>(m_Hasher(fileName));
  
    if (m_FunctionLookup.find(fileNameHash) == m_FunctionLookup.end()) {
      m_FunctionLookup.insert({ fileNameHash, {} });
      m_FileNameLookup.insert({ fileNameHash, fileName });
    }

    auto func = std::make_shared<Function>(std::move(name), funcNameHash, arity, fileName, exported, extension);

    if (extension) {
      m_ExtensionMethodLookup[objectNameHash].push_back(func);
    }

    auto [it, res] = m_FunctionLookup.at(fileNameHash).try_emplace(funcNameHash, func);
    if (res) {
      m_LastFileNameHash = fileNameHash;
      m_LastFunctionHash = funcNameHash;
      return true;
    }
    return false;
  }

  bool VM::AddClass(std::string&& name, const std::vector<std::string>& members, const std::string& fileName, bool exported)
  {
    auto classNameHash = static_cast<std::int64_t>(m_Hasher(name));
    auto fileNameHash = static_cast<std::int64_t>(m_Hasher(fileName));

    if (m_ClassLookup.find(fileNameHash) == m_ClassLookup.end()) {
      m_ClassLookup.insert({ fileNameHash, {} });
      m_FileNameLookup.insert({ fileNameHash, fileName });
    }

    Class cls{ std::move(name), members, fileName, fileNameHash, exported };

    auto [it, res] = m_ClassLookup.at(fileNameHash).try_emplace(classNameHash, cls);
    if (res) {
      m_LastFileNameHash = fileNameHash;
      return true;
    }

    return false;
  }

  bool VM::CombineFunctions(const std::string& mainFileName, GRACE_MAYBE_UNUSED bool verbose)
  {
    auto mainHash = static_cast<std::int64_t>(m_Hasher("main"));
    auto mainFileNameHash = static_cast<std::int64_t>(m_Hasher(mainFileName));

    auto it = m_FunctionLookup.at(mainFileNameHash).find(mainHash);
    if (it == m_FunctionLookup.at(mainFileNameHash).end()) {
      fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
      fmt::print(stderr, "Could not find `main` function in file, execution cannot proceed.\n");
      return false;
    }

    m_FullOpList = it->second->opList;
    m_FullConstantList = it->second->constantList;
    for (auto& [fileName, funcList] : m_FunctionLookup) {
      for (auto& [name, func] : funcList) {
        if (name == mainHash) {
          continue;
        }

        auto& opList = func->opList;
        func->opIndexStart = m_FullOpList.size();
        m_FullOpList.reserve(opList.size());
        m_FullOpList.insert(m_FullOpList.end(), opList.begin(), opList.end());

        auto& constantList = func->constantList;
        func->constantIndexStart = m_FullConstantList.size();
        m_FullConstantList.reserve(constantList.size());
        m_FullConstantList.insert(m_FullConstantList.end(), constantList.begin(), constantList.end());
      }
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

  InterpretResult VM::Start(const std::string& mainFileName, bool verbose, const std::vector<std::string>& args)
  {
    using namespace std::chrono;

    auto mainFileNameHash = static_cast<std::int64_t>(m_Hasher(mainFileName));
    auto start = steady_clock::now();
    auto res = Run(mainFileNameHash, verbose, args);
    auto end = steady_clock::now();
  
    if (verbose) {
      if (res == InterpretResult::RuntimeOk) {
        auto dur = duration_cast<microseconds>(end - start).count();
        if (dur > 1000) {
          fmt::print("Program finished successfully in {} ms.\n", duration_cast<milliseconds>(end - start).count());
        } else {
#ifdef GRACE_MSC
          fmt::print("Program finished successfully in {} \xE6s.\n", dur);
#else
          fmt::print("Program finished successfully in {} µs.\n", dur);
#endif
        }
      }
    }
    return res;
  }

  InterpretResult VM::Run(std::int64_t mainFileNameHash, GRACE_MAYBE_UNUSED bool verbose, const std::vector<std::string>& clArgs)
  {
  #define PRINT_LOCAL_MEMORY()                                                                          \
    do {                                                                                                \
      if (verbose) {                                                                                    \
        PrintStack(valueStack, m_FunctionLookup.at(fileNameStack.top().first).at(funcNameHash)->name);  \
        PrintLocals(localsList, m_FunctionLookup.at(fileNameStack.top().first).at(funcNameHash)->name); \
      }                                                                                                 \
    } while (false)                                                                                     \

    auto interpretResult = InterpretResult::RuntimeOk;

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
  
    auto& mainFunc = m_FunctionLookup.at(mainFileNameHash).at(funcNameHash);

    std::vector<std::pair<std::size_t, std::size_t>> opConstOffsets;
    opConstOffsets.reserve(32);
    opConstOffsets.emplace_back(mainFunc->opIndexStart, mainFunc->constantIndexStart);

    std::stack<std::size_t> localsOffsets;
    localsOffsets.push(0);  // [0] is args

    std::vector<CallStackEntry> callStack;
    callStack.push_back({ static_cast<std::int64_t>(m_Hasher("file")), funcNameHash, 1, mainFunc->fileName, mainFunc->fileName, mainFunc->fileNameHash, mainFunc->fileNameHash });

    std::stack<std::pair<std::int64_t, std::string>> fileNameStack;
    fileNameStack.push({ mainFileNameHash, mainFunc->fileName });

    // used to restore the "state" of the VM before entering a try block
    // if an exception is caught
    struct VMState
    {
      std::size_t stackSize{}, numLocals{}, callStackSize{}, opOffsetSize{}, localsOffsetsSize{}, 
        heldIteratorsSize{}, namespaceStackSize{}, fileNameStackSize{};
      std::int64_t opIndexToJump{}, constIndexToJump{};
    };

    std::stack<VMState> vmStateStack;
    std::stack<Value> heldIterators;
    std::stack<std::vector<std::pair<std::string, std::int64_t>>> namespaceLookupStack; // used to keep track of what namespace we look for a call in
    namespaceLookupStack.emplace();

    bool inTryBlock = false;

    ObjectTracker::SetVerbose(verbose);

#ifdef GRACE_PROFILE_OPS
    using namespace std::chrono;

    using TimeList = std::unordered_map<Ops, std::pair<std::int64_t, std::int64_t>>;
    TimeList opTimes;

    struct OpTimer
    {
      TimeList& timeList;
      Ops op;
      steady_clock::time_point start;

      OpTimer(TimeList& list, Ops op_) 
        : timeList{ list }
        , op { op_ }
        , start{ steady_clock::now() }
      {
      }

      ~OpTimer()
      {
        auto end = steady_clock::now();
        auto dur = duration_cast<nanoseconds>(end - start).count();
        if (timeList.find(op) == timeList.end()) {
          timeList[op] = { 1, dur };
        } else {
          auto [count, time] = timeList[op];
          timeList[op] = { count + 1, time + dur };
        }
      }
    };
#endif

    while (true) {
      auto [op, line] = m_FullOpList[opCurrent++];

#ifdef GRACE_PROFILE_OPS
      OpTimer p{ opTimes, op };
#endif
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
          case Ops::AppendNamespace: {
            auto text = m_FullConstantList[constantCurrent++].Get<std::string>();
            auto hash = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
            namespaceLookupStack.top().emplace_back(std::move(text), hash);
            break;
          }
          case Ops::StartNewNamespace:
            namespaceLookupStack.emplace();
            break;
          case Ops::Call: {
            auto calleeNameHash = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
            auto numArgsGiven = static_cast<std::size_t>(m_FullConstantList[constantCurrent++].Get<std::int64_t>());

            Function* calleeFunc;

            // need to verify the callee exists in the namespace prepended to the call
            const auto& namespaceToSearch = namespaceLookupStack.top();
            if (namespaceToSearch.empty()) {
              // calling a function in the same file
              auto& funcList = m_FunctionLookup.at(fileNameStack.top().first);
              auto funcIt = funcList.find(calleeNameHash);
              if (funcIt == funcList.end()) {
                throw GraceException(
                  GraceException::Type::FunctionNotFound,
                  fmt::format(
                    "cannot find function `{}` in the current namespace",
                    m_FullConstantList[constantCurrent++].Get<std::string>()
                  )
                );
              }

              calleeFunc = funcIt->second.get();
            } else {
              // build out the file path to search...
              std::stringstream path;
              for (std::size_t i = 0; i < namespaceToSearch.size(); ++i) {
                path << namespaceToSearch[i].first;
                if (i < namespaceToSearch.size() - 1) {
                  path << '/';
                } else {
                  path << ".gr";
                }
              }

              auto funcListIt = m_FunctionLookup.find(static_cast<std::int64_t>(m_Hasher(path.str())));
              if (funcListIt == m_FunctionLookup.end()) {
                std::string toDisplay;
                for (std::size_t i = 0; i < namespaceToSearch.size(); ++i) {
                  toDisplay.append(namespaceToSearch[i].first);
                  if (i < namespaceToSearch.size() - 1) {
                    toDisplay.append("::");
                  }
                }
                throw GraceException(
                  GraceException::Type::NamespaceNotFound,
                  fmt::format("namespace `{}` has not been imported", toDisplay)
                );
              }

              auto funcIt = funcListIt->second.find(calleeNameHash);
              if (funcIt == funcListIt->second.end() || !funcIt->second->exported) {
                std::string toDisplay;
                for (std::size_t i = 0; i < namespaceToSearch.size(); ++i) {
                  toDisplay.append(namespaceToSearch[i].first);
                  if (i < namespaceToSearch.size() - 1) {
                    toDisplay.append("::");
                  }
                }
                throw GraceException(
                  GraceException::Type::FunctionNotFound,
                  fmt::format(
                    "function `{}` is not a member of namespace `{}` or has not been marked `export`", 
                    m_FullConstantList[constantCurrent++].Get<std::string>(),
                    toDisplay
                  )
                );
              }

              calleeFunc = funcIt->second.get();
            }          
          
            // increment this here to get past the string function name
            // we only put it in the constant list so we can report a nice error
            constantCurrent++;         

            if (namespaceLookupStack.size() > 1) {
              // keep the size at 1 for the file that was ran
              namespaceLookupStack.pop();
            }

            auto arity = calleeFunc->arity;

            if (numArgsGiven != arity) {            
              throw GraceException(
                GraceException::Type::IncorrectArgCount, 
                fmt::format("Incorrect number of arguments given to function '{}', expected {} but got {}", calleeFunc->name, arity, numArgsGiven)
              );
            }

            localsOffsets.push(localsList.size());
            localsList.resize(localsList.size() + arity);
            for (std::size_t i = 0; i < arity; i++) {
              localsList[arity - i - 1 + localsOffsets.top()] = Pop(valueStack);
            }

            callStack.push_back({ funcNameHash, calleeNameHash, line, fileNameStack.top().second, calleeFunc->fileName, fileNameStack.top().first, calleeFunc->fileNameHash });
            
            valueStack.emplace_back(static_cast<std::int64_t>(opCurrent));
            valueStack.emplace_back(static_cast<std::int64_t>(constantCurrent));
            valueStack.emplace_back(static_cast<std::int64_t>(heldIterators.size()));

            fileNameStack.push({ calleeFunc->fileNameHash, calleeFunc->fileName });

            opCurrent = calleeFunc->opIndexStart;
            constantCurrent = calleeFunc->constantIndexStart; 
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
          case Ops::MemberCall: {
            auto& calleeFuncName = m_FullConstantList[constantCurrent++].Get<std::string>();
            auto calleeNameHash = m_FullConstantList[constantCurrent++].Get<std::int64_t>();          
            auto numArgs = static_cast<std::size_t>(m_FullConstantList[constantCurrent++].Get<std::int64_t>());

            std::vector<Value> argsGiven(numArgs);
            for (std::size_t i = 0; i < numArgs; i++) {
              argsGiven[numArgs - i - 1] = Pop(valueStack);
            }

            auto callerObject = Pop(valueStack);
            auto typeNameHash = m_Hasher(callerObject.GetTypeName());
          
            auto funcListIt = m_ExtensionMethodLookup.find(typeNameHash);
            if (funcListIt == m_ExtensionMethodLookup.end()) {
              throw GraceException(GraceException::Type::FunctionNotFound, fmt::format("Member function `{}` for type `{}` not found, you might be missing an import", calleeFuncName, callerObject.GetTypeName()));
            }

            auto funcIt = std::find_if(funcListIt->second.begin(), funcListIt->second.end(), [&calleeFuncName](const std::shared_ptr<Function>& func) {
                return func->name == calleeFuncName;
            });
            if (funcIt == funcListIt->second.end()) {
              throw GraceException(GraceException::Type::FunctionNotFound, fmt::format("Member function `{}` for type `{}` not found, you might be missing an import", calleeFuncName, callerObject.GetTypeName()));
            }

            auto calleeFunc = funcIt->get();
            auto arity = calleeFunc->arity;

            // first arg for the function will be the value we popped from the stack above...
            if (arity != numArgs + 1) {
              throw GraceException(
                GraceException::Type::IncorrectArgCount,
                fmt::format("Incorrect number of arguments given to function '{}', expected {} but got {}", calleeFunc->name, arity, numArgs)
              );
            }

            localsOffsets.push(localsList.size());
            localsList.resize(localsList.size() + arity);
            for (std::size_t i = 0; i < arity - 1; i++) {
              localsList[arity - i - 1 + localsOffsets.top()] = std::move(argsGiven.back());
              argsGiven.pop_back();
            }
            localsList[localsOffsets.top()] = std::move(callerObject);

            callStack.push_back({ funcNameHash, calleeNameHash, line, fileNameStack.top().second, calleeFunc->fileName, fileNameStack.top().first, calleeFunc->fileNameHash });
            
            valueStack.emplace_back(static_cast<std::int64_t>(opCurrent));
            valueStack.emplace_back(static_cast<std::int64_t>(constantCurrent));
            valueStack.emplace_back(static_cast<std::int64_t>(heldIterators.size()));
            
            fileNameStack.push({ calleeFunc->fileNameHash, calleeFunc->fileName });

            opCurrent = calleeFunc->opIndexStart;
            constantCurrent = calleeFunc->constantIndexStart;
            opConstOffsets.emplace_back(opCurrent, constantCurrent);

            funcNameHash = calleeNameHash;
            break;
          }
          case Ops::AssignMember: {
            auto value = Pop(valueStack);
            auto parentValue = Pop(valueStack);
            auto parentObject = parentValue.GetObject();
            
            if (parentObject == nullptr) {
              throw GraceException(
                GraceException::Type::InvalidType,
                fmt::format("`{}` has no members", parentValue.GetTypeName())
              );
            }

            auto instance = parentObject->GetAsInstance();
            if (instance == nullptr) {
              throw GraceException(
                GraceException::Type::InvalidType,
                fmt::format("`{}` has no members", parentValue.GetTypeName())
              );
            }

            auto& memberName = m_FullConstantList[constantCurrent++].Get<std::string>();
            instance->AssignMember(memberName, std::move(value));
            break;
          }
          case Ops::LoadMember: {
            auto parentValue = Pop(valueStack);
            auto parentObject = parentValue.GetObject();
            if (parentObject == nullptr) {
              throw GraceException(
                GraceException::Type::InvalidType,
                fmt::format("`{}` has no members", parentValue.GetTypeName())
              );
            }

            auto instance = parentObject->GetAsInstance();
            if (instance == nullptr) {
              throw GraceException(
                GraceException::Type::InvalidType,
                fmt::format("`{}` has no members", parentValue.GetTypeName())
              );
            }

            auto& memberName = m_FullConstantList[constantCurrent++].Get<std::string>();
            valueStack.push_back(instance->LoadMember(memberName));
            break;
          }
          case Ops::AssignLocal: {
            auto value = Pop(valueStack);
            localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] = std::move(value);
            break;
          }
          case Ops::AddAssign: {
            auto value = Pop(valueStack);
            localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] += value;
            break;
          }
          case Ops::DivideAssign: {
            auto value = Pop(valueStack);
            localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] /= value;
            break;
          }
          case Ops::MultiplyAssign: {
            auto value = Pop(valueStack);
            localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] *= value;
            break;
          }
          case Ops::SubtractAssign: {
            auto value = Pop(valueStack);
            localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] -= value;
            break;
          }
          case Ops::BitwiseAndAssign: {
            auto value = Pop(valueStack);
            localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] &= value;
            break;
          }
          case Ops::BitwiseOrAssign: {
            auto value = Pop(valueStack);
            localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] |= value;
            break;
          }
          case Ops::BitwiseXOrAssign: {
            auto value = Pop(valueStack);
            localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] ^= value;
            break;
          }
          case Ops::ModAssign: {
            auto value = Pop(valueStack);
            localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] %= value;
            break;
          }
          case Ops::ShiftLeftAssign: {
            auto value = Pop(valueStack);
            localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] <<= value;
            break;
          }
          case Ops::ShiftRightAssign: {
            auto value = Pop(valueStack);
            localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()] >>= value;
            break;
          }
          case Ops::PowAssign: {
            auto value = Pop(valueStack);
            auto& local = localsList[m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top()];
            local = local.Pow(value);
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

            if (auto list = object->GetAsList()) {
              auto listIterator = Value::CreateObject<GraceIterator>(list, GraceIterator::IterableType::List);
              auto listIteratorObject = listIterator.GetObject()->GetAsIterator();
              heldIterators.push(listIterator);
              
              localsList[iteratorId + localsOffsets.top()] = listIteratorObject->IsAtEnd() ? Value() : listIteratorObject->Value();
            
              if (twoIterators) {
                auto secondIteratorId = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
                localsList[secondIteratorId + localsOffsets.top()] = std::int64_t(0);
              } else {
                constantCurrent++;
              }
            } else if (auto dict = object->GetAsDictionary()) {
              auto dictIterator = Value::CreateObject<GraceIterator>(dict, GraceIterator::IterableType::Dictionary);
              auto dictIteratorObject = dictIterator.GetObject()->GetAsIterator();
              heldIterators.push(dictIterator);


              if (twoIterators) {
                auto secondIteratorId = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
                if (dictIteratorObject->IsAtEnd()) {
                  localsList[iteratorId + localsOffsets.top()] = nullptr;
                  localsList[secondIteratorId + localsOffsets.top()] = nullptr;
                } else {
                  auto kvpObject = dictIteratorObject->Value().GetObject()->GetAsKeyValuePair();
                  localsList[iteratorId + localsOffsets.top()] = kvpObject->Key();
                  localsList[secondIteratorId + localsOffsets.top()] = kvpObject->Value();
                }
              } else {
                localsList[iteratorId + localsOffsets.top()] = dictIteratorObject->IsAtEnd() ? Value()
                                                                                             : dictIteratorObject->Value();
                constantCurrent++;
              }
            } else if (auto set = object->GetAsSet()) {
              auto setIterator = Value::CreateObject<GraceIterator>(set, GraceIterator::IterableType::Set);
              auto setIteratorObject = setIterator.GetObject()->GetAsIterator();
              heldIterators.push(setIterator);

              localsList[iteratorId + localsOffsets.top()] = setIteratorObject->IsAtEnd() ? Value() : setIteratorObject->Value();
              constantCurrent++;

              if (twoIterators) {
                throw GraceException(
                  GraceException::Type::InvalidCollectionOperation,
                  "`Set` does not support multiple iterators"
                );
              }
            } else if (auto range = object->GetAsRange()) {
              auto rangeIterator = Value::CreateObject<GraceIterator>(range, GraceIterator::IterableType::Range);
              auto rangeIteratorObject = rangeIterator.GetObject()->GetAsIterator();
              heldIterators.push(rangeIterator);

              localsList[iteratorId + localsOffsets.top()] = rangeIteratorObject->IsAtEnd() ? Value() : rangeIteratorObject->Value();
              constantCurrent++;

              if (twoIterators) {
                throw GraceException(
                  GraceException::Type::InvalidCollectionOperation,
                  "`Range` does not support multiple iterators"
                );
              }
            } else {
              // unreachable (?) due to IsIterable() check
              GRACE_ASSERT(false, "Object did not dynamic_cast to a valid iterable type");
            }
            break;
          }
          case Ops::IncrementIterator: {
            auto twoIterators = m_FullConstantList[constantCurrent++].Get<bool>();
            auto iteratorVarId = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
            auto heldIterator = heldIterators.top().GetObject()->GetAsIterator();
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
                  auto kvpObject = heldIterator->Value().GetObject()->GetAsKeyValuePair();
                  localsList[iteratorVarId + localsOffsets.top()] = kvpObject->Key();
                  localsList[secondIteratorId + localsOffsets.top()] = kvpObject->Value();
                } else {
                  localsList[iteratorVarId + localsOffsets.top()] = nullptr;
                  localsList[secondIteratorId + localsOffsets.top()] = nullptr;
                }
              } else {
                localsList[iteratorVarId + localsOffsets.top()] = heldIterator->IsAtEnd() ? Value()
                                                                                          : heldIterator->Value();
                constantCurrent++;
              }
            } else if (iterableType == GraceIterator::IterableType::Set || iterableType == GraceIterator::IterableType::Range){
              heldIterator->Increment();
              localsList[iteratorVarId + localsOffsets.top()] = heldIterator->IsAtEnd() ? Value() : heldIterator->Value();
              constantCurrent++;
            } else {
              GRACE_UNREACHABLE();
            }
            break;
          }
          case Ops::CheckIteratorEnd: {
            auto heldIterator = heldIterators.top().GetObject()->GetAsIterator();
            valueStack.emplace_back(heldIterator->AsBool());
            break;
          }
          case Ops::DestroyHeldIterator: {
            heldIterators.pop();
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

            funcNameHash = callStack.back().callerHash;
            callStack.pop_back();
            fileNameStack.pop();

            auto heldIteratorsSize = static_cast<std::size_t>(Pop(valueStack).Get<std::int64_t>());
            while (heldIterators.size() != heldIteratorsSize) {
              heldIterators.pop();
            }

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
              case 0: { // int
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
              case 1: { // float
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
              case 2: // bool
                valueStack.emplace_back(value.AsBool());
                break;
              case 3: // string
                valueStack.emplace_back(value.AsString());
                break;
              case 4: { // char
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
              case 5: // exception
                valueStack.emplace_back(Value::CreateObject<GraceException>(value.AsString()));
                break;
              case 6: { // kvp
                auto key = Pop(valueStack);
                valueStack.emplace_back(Value::CreateObject<GraceKeyValuePair>(std::move(key), std::move(value)));
                break;
              }
              case 7: { // range
                GRACE_NOT_IMPLEMENTED();
                break;
              }
              default:
                GRACE_UNREACHABLE();
                break;
            }
            break;
          }
          case Ops::CheckType: {
            auto value = Pop(valueStack);
            auto typeIdx = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
            if (typeIdx < 6) {
              valueStack.emplace_back(typeIdx == static_cast<std::int64_t>(value.GetType()));
            } else if (typeIdx < 11) {
              auto object = value.GetObject();
              valueStack.emplace_back(typeIdx - 6 == static_cast<std::int64_t>(object->ObjectType()));
            } else {
              auto object = value.GetObject();
              auto& typeName = m_FullConstantList[constantCurrent++].Get<std::string>();
              valueStack.emplace_back(typeName == object->ObjectName());
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
          case Ops::CreateInstance: {
            auto numMembers = static_cast<std::size_t>(m_FullConstantList[constantCurrent++].Get<std::int64_t>());

            std::vector<GraceInstance::Member> memberList(numMembers);
            auto localsStartIndex = localsList.size() - numMembers;
            for (auto i = localsStartIndex; i < localsStartIndex + numMembers; i++) {
              memberList[i - localsStartIndex] = { m_FullConstantList[constantCurrent++].Get<std::string>(), localsList[i] };
            }

            auto classNameHash = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
            auto classFileNameHash = m_FullConstantList[constantCurrent++].Get<std::int64_t>();

            // i don't think we need to check these since the call to the constructor would have already failed...
            auto& classType = m_ClassLookup.at(classFileNameHash).at(classNameHash);
            auto className = classType.name;
            valueStack.push_back(Value::CreateObject<GraceInstance>(std::move(className), std::move(memberList)));
            break;
          }
          case Ops::CreateDictionary: {
            auto numItems = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
            if (numItems == 0) {
              valueStack.push_back(Value::CreateObject<GraceDictionary>());
              break;
            }
            
            auto value = Value::CreateObject<GraceDictionary>();
            auto dict = value.GetObject()->GetAsDictionary();

            for (std::int64_t i = 0; i < numItems; i++) {
              auto [key, val] = PopLastTwo(valueStack);
              dict->Insert(std::move(key), std::move(val));
            }
            valueStack.push_back(std::move(value));
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
          case Ops::CreateListFromCast: {
            auto numItems = m_FullConstantList[constantCurrent++].Get<std::int64_t>();

            if (numItems == 0) {
              valueStack.push_back(Value::CreateObject<GraceList>());
            } else if (numItems == 1) {
              auto value = Pop(valueStack);
              if (value.GetType() == Value::Type::String) {
                valueStack.push_back(GraceList::FromString(value.Get<std::string>()));
              } else if (value.GetType() == Value::Type::Object && value.GetObject()->GetAsDictionary() != nullptr) {
                valueStack.push_back(GraceList::FromDict(value.GetObject()->GetAsDictionary()));
              } else {
                std::vector<Value> items{ value };
                valueStack.push_back(Value::CreateObject<GraceList>(std::move(items)));
              }
            } else {
              std::vector<Value> items(numItems);
              for (auto i = 0; i < numItems; i++) {
                items[numItems - i - 1] = Pop(valueStack);
              }

              valueStack.push_back(Value::CreateObject<GraceList>(std::move(items)));
            }
            break;
          }
          case Ops::CreateRange: {
            auto increment = Pop(valueStack);
            auto max = Pop(valueStack);
            auto min = Pop(valueStack);
            valueStack.push_back(Value::CreateObject<GraceRange>(std::move(min), std::move(max), std::move(increment)));
            break;
          }
          case Ops::CreateSet: {
            auto numItems = m_FullConstantList[constantCurrent++].Get<std::int64_t>();
            if (numItems == 0) {
              valueStack.push_back(Value::CreateObject<GraceSet>());
              break;
            }

            if (numItems == 1) {
              valueStack.push_back(Value::CreateObject<GraceSet>(Pop(valueStack)));
              break;
            }

            std::vector<Value> result(numItems);
            for (auto i = 0; i < numItems; i++) {
              result[numItems - i - 1] = Pop(valueStack);
            }

            valueStack.push_back(Value::CreateObject<GraceSet>(std::move(result)));
            break;
          }
          case Ops::AssignSubscript: {
            auto newValue = Pop(valueStack);
            auto subscript = Pop(valueStack);
            auto container = Pop(valueStack);

            if (container.GetType() == Value::Type::Object) {
              auto object = container.GetObject();
              
              switch (object->ObjectType()) {
                case GraceObjectType::List: {
                  if (subscript.GetType() != Value::Type::Int) {
                    throw GraceException(GraceException::Type::InvalidType, fmt::format("Expected `Int` for subscript index but got `{}`", subscript.GetTypeName()));
                  }
                  auto index = static_cast<std::size_t>(subscript.Get<std::int64_t>());
                  (*object->GetAsList())[index] = newValue;
                  break;
                }
                case GraceObjectType::Dictionary:
                  object->GetAsDictionary()->Update(subscript, std::move(newValue));
                  break;
                default:
                  GRACE_UNREACHABLE();
                  break;
              }
            } else {
              throw GraceException(GraceException::Type::InvalidType, fmt::format("`{}` cannot be indexed", container.GetTypeName()));
            }
            break;
          }
          case Ops::GetSubscript: {
            auto [container, subscript] = PopLastTwo(valueStack);
            
            auto valueType = container.GetType();
            if (valueType == Value::Type::String) {
              const auto& s = container.Get<std::string>();
              if (subscript.GetType() != Value::Type::Int) {
                throw GraceException(GraceException::Type::InvalidType, fmt::format("Expected `Int` for subscript index but got `{}`", subscript.GetTypeName()));
              }
              auto index = static_cast<std::size_t>(subscript.Get<std::int64_t>());
              if (index >= s.length()) {
                throw GraceException(GraceException::Type::IndexOutOfRange, fmt::format("Given index is {} but the length of the `String` is {}", index, s.length()));
              }
              valueStack.emplace_back(s[index]);
            } else if (valueType == Value::Type::Object) {
              auto object = container.GetObject();
              
              switch (object->ObjectType()) {
                case GraceObjectType::List: {
                  if (subscript.GetType() != Value::Type::Int) {
                    throw GraceException(GraceException::Type::InvalidType, fmt::format("Expected `Int` for subscript index but got `{}`", subscript.GetTypeName()));
                  }
                  auto i = static_cast<std::size_t>(subscript.Get<std::int64_t>());
                  valueStack.push_back((*object->GetAsList())[i]);
                  break;
                }
                case GraceObjectType::Dictionary:
                  valueStack.push_back(object->GetAsDictionary()->Get(subscript));                
                  break;
                default:
                  GRACE_UNREACHABLE();
                  break;
              }
            } else {
              throw GraceException(GraceException::Type::InvalidType, fmt::format("`{}` cannot be indexed", container.GetTypeName()));
            }
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
            auto& message = m_FullConstantList[constantCurrent++].Get<std::string>();
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
              heldIterators.size(),
              namespaceLookupStack.size(),
              fileNameStack.size(),
              opIdx,
              constIdx
            });
            break;
          }
          case Ops::ExitTry: {
            auto targetNumLocals = m_FullConstantList[constantCurrent++].Get<std::int64_t>() + localsOffsets.top();
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
          while (heldIterators.size() != vmState.heldIteratorsSize) {
            heldIterators.pop();
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

          funcNameHash = callStack.back().calleeHash;

          valueStack.push_back(Value::CreateObject<GraceException>(ge.GetType(), ge.Message()));

          while (namespaceLookupStack.size() != vmState.namespaceStackSize) {
            namespaceLookupStack.pop();
          }

          while (fileNameStack.size() != vmState.fileNameStackSize) {
            fileNameStack.pop();
          }

        } else {
          // exception unhandled, report the error, clean up and quit
          RuntimeError(ge, line, callStack);          
          interpretResult = InterpretResult::RuntimeError;

          valueStack.clear();
          localsList.clear();
          while (!heldIterators.empty()) {
            heldIterators.pop();
          }

          ObjectTracker::Finalise();
          return interpretResult;
        }
      }
    }

  exit:

    valueStack.clear();
    localsList.clear();

#ifdef GRACE_DEBUG 
    PRINT_LOCAL_MEMORY();
#endif

    ObjectTracker::Finalise();

#ifdef GRACE_PROFILE_OPS
    using Pair = std::pair<Ops, std::int64_t>;
    std::vector<Pair> timesVes;
    for (const auto& [op, stats] : opTimes) {
      auto& [count, time] = stats;
      auto average = time / count;
      timesVes.emplace_back(op, average);
    }

    std::sort(timesVes.begin(), timesVes.end(), [](const Pair& a, const Pair& b) {
      return a.second < b.second;
    });
    fmt::print("OP TIMES:\n");
    for (const auto& [op, time] : timesVes) {
      fmt::print("\t{}: {} ns/op average\n", op, time);
    }
#endif

    return interpretResult;

#undef PRINT_LOCAL_MEMORY
  }

  void VM::RuntimeError(const GraceException& exception, std::size_t line, const std::vector<CallStackEntry>& callStack)
  {
    fmt::print(stderr, "\nCall stack (most recent call last):\n");

    auto callStackSize = callStack.size();
    if (callStackSize > 15) {
  #ifdef GRACE_MSC
      // std::getenv() produces a warning on Windows, therefore error with /W4 /WX
      std::size_t size;
      getenv_s(&size, NULL, 0, "GRACE_SHOW_FULL_CALLSTACK");
      if (size != 0) {
  #else
      if (std::getenv("GRACE_SHOW_FULL_CALLSTACK") != NULL) {
  #endif
        for (std::size_t i = 1; i < callStack.size(); i++) {
          const auto& [caller, callee, ln, fileName, calleeFileName, fileNameHash, calleeFileNameHash] = callStack[i];
          const auto& callerFunc = m_FunctionLookup.at(fileNameHash).at(caller);
          fmt::print(stderr, "in {}:{}:{}\n", fileName, callerFunc->name, ln);
          auto absolute = std::filesystem::absolute(fileName).string();
          fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(absolute, ln));
        }
      } else {
        fmt::print(stderr, "{} more calls before - set environment variable `GRACE_SHOW_FULL_CALLSTACK` to see full callstack\n", callStackSize - 15);
        for (auto i = callStackSize - 15; i < callStackSize; i++) {
          const auto& [caller, callee, ln, fileName, calleeFileName, fileNameHash, calleeFileNameHash] = callStack[i];
          const auto& callerFunc = m_FunctionLookup.at(fileNameHash).at(caller);
          auto absolute = std::filesystem::absolute(fileName).string();
          fmt::print(stderr, "in {}:{}:{}\n", fileName, callerFunc->name, ln);
          fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(absolute, ln));
        }
      }
    } else {
      for (std::size_t i = 1; i < callStack.size(); i++) {
        const auto& [caller, callee, ln, fileName, calleeFileName, fileNameHash, calleeFileNameHash] = callStack[i];
        const auto& callerFunc = m_FunctionLookup.at(fileNameHash).at(caller);
        auto absolute = std::filesystem::absolute(fileName).string();
        fmt::print(stderr, "in {}:{}:{}\n", fileName, callerFunc->name, ln);
        fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(absolute, ln));
      }
    }

    const auto& calleeFunc = m_FunctionLookup.at(callStack.back().calleeFileNameHash).at(callStack.back().calleeHash);
    fmt::print(stderr, "in {}:{}:{}\n", calleeFunc->fileName, calleeFunc->name, line);
    auto absolute = std::filesystem::absolute(calleeFunc->fileName).string();
    fmt::print(stderr, "{:>4}\n", Scanner::GetCodeAtLine(absolute, line));

    fmt::print(stderr, "\n");
    fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
    fmt::print(stderr, "[line {}] {}. Stopping execution.\n", line, exception.ToString());
  }
}
