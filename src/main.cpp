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

#include "../include/fmt/core.h" 
#include "../include/fmt/color.h"

#include "grace.hpp"
#include "compiler.hpp"

static void Error(const std::string& message)
{
  fmt::print("Grace 0.0.1\n\n");
  fmt::print(fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print("{}\n", message);
}

static void Usage()
{
  fmt::print("Grace 0.0.1\n\nUsage: grace <file> [--verbose/-v]\n");
}

int main(int argc, const char* argv[])
{
  std::vector<std::string> args;
  for (auto i = 0; i < argc; i++) {
    args.push_back(argv[i]);
  }

  bool verbose = false;
  switch (argc) {
    case 1:
      Usage();
      return 1;
    case 3:
      if (args[2] == "-v" || args[2] == "--verbose") {
        verbose = true;
      } else {
        Usage();
        return 1;
      } 
    case 2:
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
      Grace::Compiler::Compile(inPath.filename(), inStream.str(), verbose);
      break;
  }
}

