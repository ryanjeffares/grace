/*
 *  The Grace Programming Language.
 *
 *  This file contains the declarations of exported functions when Grace is built as a dynamic library.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_DLLMAIN_HPP
#define GRACE_DLLMAIN_HPP

#include "compiler.hpp"
#include "grace.hpp"

#ifdef GRACE_MSC
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

extern "C"
{
  EXPORT Grace::VM::InterpretResult RunFile(const char* filePath, int interpreterArgc, const char* interpreterArgv[], int graceArgc, const char* graceArgv[]);
}

#endif // ifndef GRACE_DLLMAIN_HPP