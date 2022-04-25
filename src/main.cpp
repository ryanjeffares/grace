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

#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <fmt/core.h>
#include <fmt/color.h>

#include "grace.hpp"
#include "compiler.hpp"

static void Error(const std::string& message)
{
  fmt::print(stderr, "Grace {}.{}.{}\n\n", GRACE_MAJOR_VERSION, GRACE_MINOR_VERSION, GRACE_PATCH_NUMBER);
  fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print(stderr, "{}\n", message);
}

static void Usage()
{
  fmt::print("Grace {}.{}.{}\n\n", GRACE_MAJOR_VERSION, GRACE_MINOR_VERSION, GRACE_PATCH_NUMBER);
  fmt::print("USAGE:\n");
  fmt::print("    grace <filename> [OPTIONS]\n\n");
  fmt::print("OPTIONS:\n");
  fmt::print("    -V, --version                 Print version info and exit\n");
  fmt::print("    -v, --verbose                 Enable verbose mode - print compilation and run times, print compiler warnings\n");
  fmt::print("    -we, --warnings-error         Show compiler warnings, warnings result in errors\n");
}

int main(int argc, const char* argv[])
{
  if (argc < 2) {
    Usage();
    return 1;
  }

  std::vector<std::string> args;
  for (auto i = 0; i < argc; i++) {
    args.push_back(argv[i]);
  }

  bool verbose = false;
  bool warningsError = false;

  for (auto i = 2; i < argc; i++) {
    if (args[i] == "--version" || args[i] == "-V") {
      fmt::print("Grace {}.{}.{}\n", GRACE_MAJOR_VERSION, GRACE_MINOR_VERSION, GRACE_PATCH_NUMBER);
      return 0;
    } else if (args[i] == "--verbose" || args[i] == "-v") {
      verbose = true;
    } else if (args[i] == "--warnings-error" || args[i] == "-we") {
      warningsError = true;
    } else {
      Error(fmt::format("Unrecognised argument {}\n", args[i]));
      Usage();
      return 1;
    }
  }

  std::filesystem::path inPath(args[1]);
  if (inPath.extension() != ".gr") {
    Error(fmt::format("provided file `{}` was not a `.gr` file.", inPath.string()));
    return 1;
  }
  if (!std::filesystem::exists(inPath)) {
    Error(fmt::format("provided file `{}` does not exist", inPath.string()));
    return 1; 
  }

  std::stringstream inStream;
  try {
    std::ifstream inFile(args[1]);
    inStream << inFile.rdbuf();
  } catch (const std::exception& e) {
    Error(e.what());
    return 1;
  }

  Grace::Compiler::Compile(inPath.filename(), inStream.str(), verbose, warningsError);
}

