#include <vector>
#include <filesystem>

#include <fmt/core.h> 
#include <fmt/color.h>

#include "compiler.h"

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
  switch (args.size()) {
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
      std::filesystem::path inFile(args[1]);
      if (inFile.extension() != ".gr") {
        error("provided file was not a `.gr` file.");
        return 1;
      }
      Grace::Compiler::Compile(args[1], verbose);
      break;
  }
}
