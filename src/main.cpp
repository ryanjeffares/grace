#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "../include/fmt/core.h" 
#include "../include/fmt/color.h"

#include "grace.hpp"
#include "compiler.hpp"

static void error(const std::string& message)
{
  fmt::print("Grace 0.0.1\n\n");
  fmt::print(fmt::fg(fmt::color::red) | fmt::emphasis::bold, "ERROR: ");
  fmt::print("{}\n", message);
}

static void usage()
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
      usage();
      return 1;
    case 3:
      if (args[2] == "-v" || args[2] == "--verbose") {
        verbose = true;
      } else {
        usage();
        return 1;
      } 
    case 2:
      std::filesystem::path inPath(args[1]);
      if (inPath.extension() != ".gr") {
        error(fmt::format("provided file `{}` was not a `.gr` file.", inPath.string()));
        return 1;
      }
      if (!std::filesystem::exists(inPath)) {
        error(fmt::format("provided file `{}` does not exist", inPath.string()));
        return 1; 
      }

      try {
        std::ifstream inFile(args[1]);
        std::stringstream inStream;
        inStream << inFile.rdbuf();
        Grace::Compiler::Compile(std::move(inPath.filename()), std::move(inStream.str()), verbose);
      } catch (const std::exception& e) {
        error(e.what());
        return 1;
      }
      break;
  }
}

