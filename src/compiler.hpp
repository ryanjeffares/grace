#pragma once

#include <string>
#include <unordered_map>

#include "../include/fmt/core.h"
#include "../include/fmt/color.h"

#include "scanner.hpp"
#include "vm.hpp"

namespace Grace
{
  namespace Compiler
  {

    /*
     *  Starts the compilation process.
     *
     *  @param fileName  Name of the file to be read
     *  @param code      The code to be compiled.
     *  @param verbose   Verbose mode (display compilation time).
     */
    void Compile(std::string&& fileName, std::string&& code, bool verbose);

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
         *  @param fileName     The file to be read
         *  @param code         The code to be compiled.
         */
        explicit Compiler(std::string&& fileName, std::string&& code, bool verbose);
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

        inline bool HadError() const { return m_HadError; }
        void Finalise(bool verbose);
        std::string GetCodeAtLine(int line) const;

      private:

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

        void EmitOp(VM::Ops, int line);

        template<typename T>
        void EmitConstant(const T& value)
        {
          m_Vm.PushConstant(value);
        }

        void Statement();

        void ClassDeclaration();
        void FuncDeclaration();
        void VarDeclaration();
        void FinalDeclaration();

        void Expression(bool canAssign);
        void ExpressionStatement();
        void ForStatement();
        void IfStatement();
        void InstanceOfStatement();
        void PrintStatement();
        void PrintLnStatement();
        void ReturnStatement();
        void WhileStatement();

        void Or(bool canAssign, bool skipFirst);
        void And(bool canAssign, bool skipFirst);
        void Equality(bool canAssign, bool skipFirst);
        void Comparison(bool canAssign, bool skipFirst);
        void Term(bool canAssign, bool skipFirst);
        void Factor(bool canAssign, bool skipFirst);
        void As(bool canAssign, bool skipFirst);
        void Unary(bool canAssign);
        void Call(bool canAssign);
        void Primary(bool canAssign);

        void Char();
        void String();

        void ErrorAtCurrent(const std::string& message);
        void ErrorAtPrevious(const std::string& message);
        void Error(const std::optional<Scanner::Token>& token, const std::string& message);

      private:

        enum class Context
        {
          TopLevel,
          Function,
        } m_CurrentContext;

        Scanner::Scanner m_Scanner;
        VM::VM m_Vm;

        std::optional<Scanner::Token> m_Current, m_Previous;
        std::string m_CurrentFileName;
        std::unordered_map<std::string, std::pair<bool, std::int64_t>> m_Locals;
        std::hash<std::string> m_Hasher;

        bool m_FunctionHadReturn = false;
        bool m_PanicMode = false, m_HadError = false;
        bool m_Verbose;
    };
  }
}

