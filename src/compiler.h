#pragma once

#include <chrono>
#include <string>
#include <fstream>
#include <functional>
#include <sstream>
#include <optional>
#include <unordered_map>

#include <fmt/core.h>
#include <fmt/color.h>

#include "scanner.h"

namespace Grace
{
  namespace Compiler
  {

    /*
     *  Starts the compilation process.
     *
     *  @param code      The code to be compiled.
     *  @param verbose   Verbose mode (display compilation time).
     */
    void Compile(std::string&& code, bool verbose);

    namespace Detail
    {
      /*
       *  The main Compiler class.   
       */
      class Compiler
      {
        public:

          /*
           *  Constructs a new compiler instance, taking the code to be compiled as a 
           *  forward reference to be given to the Scanner.
           *
           *  @param code   The code to be compiled.
           */
          explicit Compiler(std::string&& code);
          ~Compiler() = default;

          Compiler(const Compiler&) = delete;
          Compiler(Compiler&&) = delete;

          /*
           *  Advances the scanner by 1 token and updates the Compiler's previous and current.
           *  `ErrorAtCurrent` will be called if the new current is an error token.
           */
          void Advance();

          /*
           *  Compiles a 'declaration' according to the grammar.
           */
          void Declaration();

          /*
           *  Returns `false` if the current token's type is not the expected.
           *  Otherwise, calls `Advance()` and returns true.
           *
           *  @param expected   The token to be matched against.
           */
          bool Match(Scanner::TokenType expected);

        private:

          enum class Precedence 
          {
            None,
            Assignment,
            Or,
            And,
            Equality,
            Comparison,
            Term,
            Factor,
            Unary,
            Call,
            Primary,
          };

          struct ParseRule
          {
            std::function<void(Compiler*, bool)> m_Prefix, m_Infix;
            Precedence m_Precedence;
          };
          
          /*
           *  Returns true if the current token matches the given type.
           *  No side effects, does not advance the scanner or compiler.
           *
           *  @param expected   The token to be checked
           */
          bool Check(Scanner::TokenType expected) const;

          /*
           *  Advances the compiler if the current token matches the expected.
           *  Calls `ErrorAtCurrent()` with the given message otherwise.
           *
           *  @param expected   The expected TokenType
           *  @param message    Error message to be given on mismatch
           */
          void Consume(Scanner::TokenType expected, const std::string& message);

          /*
           *  To be called after an error is found.
           *  Will advance the compiler until a semicolon or keyword is found.
           */
          void Synchronize();

          void ParsePrecedence(Precedence);

          void Statement();

          void ClassDeclaration();
          void FuncDeclaration();
          void VarDeclaration();
          void FinalDeclaration();

          void Expression();
          void ExpressionStatement();
          void ForStatement();
          void IfStatement();
          void PrintStatement();
          void ReturnStatement();
          void WhileStatement();
          void Block();

          void Grouping(bool canAssign);
          void Call(bool canAssign);
          void Dot(bool canAssign);
          void Unary(bool canAssign);
          void Binary(bool canAssign);
          void Variable(bool canAssign);
          void String(bool canAssign);
          void Number(bool canAssign);
          void And(bool canAssign);
          void Or(bool canAssign);
          void Literal(bool canAssign);

          void ErrorAtCurrent(const std::string& message);
          void ErrorAtPrevious(const std::string& message);
          void Error(const std::optional<Scanner::Token>& token, const std::string& message);

        private:

          Scanner::Scanner m_Scanner;
          std::optional<Scanner::Token> m_Current, m_Previous;

          std::unordered_map<Scanner::TokenType, ParseRule> m_ParseRules;

          bool m_PanicMode = false, m_HadError = false;
      };
    }
  }
}

