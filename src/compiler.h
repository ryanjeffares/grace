#pragma once

#include <chrono>
#include <string>
#include <fstream>
#include <sstream>
#include <optional>

#include <fmt/core.h>

#include "scanner.h"

namespace Grace
{
  namespace Compiler
  {

    void Compile(const std::string& filePath, bool verbose);

    class Compiler
    {
      public:

        Compiler(std::stringstream&& code);
        ~Compiler() = default;

        Compiler(const Compiler&) = delete;
        Compiler(Compiler&&) = delete;

        void Advance();
        void Compile();
        bool Match(Scanner::TokenType);

      private:

        bool Check(Scanner::TokenType) const;

      private:

        Scanner::Scanner m_Scanner;
        std::optional<Scanner::Token> m_Current, m_Previous;
    };
  }
}
