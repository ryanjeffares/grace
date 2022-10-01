/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions of exported functions when Grace is built as a dynamic library, as well as the DllMain entry point for Windows.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include "dllmain.hpp"

#ifdef GRACE_MSC

BOOL APIENTRY DllMain(GRACE_MAYBE_UNUSED HMODULE hModule, DWORD ul_reason_for_call, GRACE_MAYBE_UNUSED LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}

#endif

extern "C"
{
  EXPORT Grace::VM::InterpretResult RunFile(const char* filePath, int interpreterArgc, const char* interpreterArgv[], int graceArgc, const char* graceArgv[])
  {
    std::vector<std::string> interpreterArgs, graceArgs;
    
    for (auto i = 0; i < interpreterArgc; i++) {
      interpreterArgs.push_back(interpreterArgv[i]);
    }

    for (auto i = 0; i < graceArgc; i++) {
      graceArgs.push_back(graceArgv[i]);
    }

    bool verbose = false, warningsError = false;
    for (const auto& arg : interpreterArgs) {
      if (arg == "--verbose" || arg == "-v") {
        verbose = true;
      }
      if (arg == "--warnings-error" || arg == "-we") {
        warningsError = true;
      }
    }

    return Grace::Compiler::Compile(filePath, verbose, warningsError, graceArgs);
  }
}