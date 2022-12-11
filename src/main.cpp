/*
 *  The Grace Programming Language.
 *
 *  This file contains the main entry point and argument parsing for the Grace interpreter. 
 *  
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <filesystem>
#include <fstream>
#include <vector>

#include <fmt/core.h>
#include <fmt/color.h>

#include "grace.hpp"
#include "compiler.hpp"

static void Error(const std::string& message)
{
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print(stderr, "{}\n", message);
}

static void Usage()
{
  fmt::print("Grace {}.{}.{}\n\n", GRACE_MAJOR_VERSION, GRACE_MINOR_VERSION, GRACE_PATCH_NUMBER);
  fmt::print("USAGE:\n");
  fmt::print("  grace [options] file [grace_options]\n\n");
  fmt::print("OPTIONS:\n");
  fmt::print("  -h, --help                    Print help info and exit\n");
  fmt::print("  -V, --version                 Print version info and exit\n");
  fmt::print("  -v, --verbose                 Enable verbose mode - print compilation and run times, print compiler warnings\n");
  fmt::print("  -we, --warnings-error         Show compiler warnings, warnings result in errors\n");
}

int main(int argc, const char* argv[])
{  
  if (argc < 2) {
    Usage();
    return 1;
  }

  std::vector<std::string> args;
  args.reserve(static_cast<std::size_t>(argc));
  for (auto i = 0; i < argc; i++) {
    args.emplace_back(argv[i]);
  }

  std::filesystem::path filePath;
  bool verbose = false;
  bool warningsError = false;

  std::vector<std::string> graceMainArgs;
  auto appendToGraceArgs = false;
  for (auto i = 1; i < argc; i++) {
    if (args[i] == "--version" || args[i] == "-V") {
      if (appendToGraceArgs) {
        graceMainArgs.push_back(args[i]);
      } else {
        fmt::print("Grace {}.{}.{}\n", GRACE_MAJOR_VERSION, GRACE_MINOR_VERSION, GRACE_PATCH_NUMBER);
        return 0;
      }
    } else if (args[i] == "--help" || args[i] == "-h") {
      if (appendToGraceArgs) {
        graceMainArgs.push_back(args[i]);
      } else {
        Usage();
        return 0;
      }
    } else if (args[i] == "--verbose" || args[i] == "-v") {
      if (appendToGraceArgs) {
        graceMainArgs.push_back(args[i]);
      } else {
        verbose = true;
      }
    } else if (args[i] == "--warnings-error" || args[i] == "-we") {
      if (appendToGraceArgs) {
        graceMainArgs.push_back(args[i]);
      } else {
        warningsError = true;
      }
    } else if (args[i].ends_with(".gr")) {
      // first .gr file will be used as the file to run
      // any other command line flags for the interpreter, e.g. -v, should be given before the file
      // any args after the first .gr file will be given to the main function in the grace script
      if (appendToGraceArgs) {
        graceMainArgs.push_back(args[i]);
      } else {
        filePath = args[i];
      }
      appendToGraceArgs = true;
    } else {
      if (appendToGraceArgs) {
        graceMainArgs.push_back(args[i]);
      } else {
        Error(fmt::format("Unrecognised argument '{}'\n", args[i]));
        Usage();
        return 1;
      }
    }
  }

  if (filePath.empty()) {
    Error("no '.gr' file given");
    return 1;
  }
  
  if (!std::filesystem::exists(filePath)) {
    Error(fmt::format("provided file '{}' does not exist", filePath.string()));
    return 1;
  }

  return static_cast<int>(Grace::Compiler::Compile(filePath.string(), verbose, warningsError, graceMainArgs));
}
