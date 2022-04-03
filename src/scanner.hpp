#include <cstddef>
#include <unordered_map>
#include <optional>
#include <string>
#include <sstream>
#include <string_view>
#include <vector>

#include "../include/fmt/format.h"

namespace Grace
{
  namespace Scanner
  {
    enum class TokenType
    {
      // Lexical tokens
      Char,
      EndOfFile,
      Error,
      Double,
      Identifier,
      Integer,
      String,
      IntIdent,
      FloatIdent,
      BoolIdent,
      StringIdent,
      CharIdent,
      
      // Symbols
      Colon,
      Semicolon,
      LeftParen,
      RightParen,
      Comma,
      Dot,
      DotDot,
      Minus,
      Mod,
      Plus,
      Slash,
      Star,
      StarStar,
      Bang,
      BangEqual,
      Equal,
      EqualEqual,
      LessThan,
      GreaterThan,
      LessEqual,
      GreaterEqual,

      // Keywords
      And,
      Assert,
      Break,
      By,
      Class, 
      Else,
      End, 
      False,
      Final,
      For,
      Func,
      If,
      In,
      InstanceOf,
      Null,
      Or,
      Print,
      PrintLn,
      Return,
      This,
      True,
      Var,
      While,
    };

    class Token
    {
      public:
        
        Token(TokenType, 
            std::size_t start, 
            std::size_t length, 
            int line, 
            int column,
            const std::string& code
        );

        Token(TokenType, int line, int column, const std::string& errorMessage);

        std::string ToString() const;
        constexpr inline TokenType GetType() const { return m_Type; }
        constexpr inline int GetLine() const { return m_Line; }
        constexpr inline int GetColumn() const { return m_Column; }
        inline const std::string& GetErrorMessage() const { return m_ErrorMessage; }
        constexpr inline std::size_t GetLength() const { return m_Length; }
        inline const std::string_view& GetText() const { return m_Text; }
        constexpr const char* GetData() const { return m_Text.data(); }

      private:

        TokenType m_Type;
        std::size_t m_Start, m_Length;
        int m_Line, m_Column;
        std::string_view m_Text;
        std::string m_ErrorMessage;
    };

    class Scanner 
    {
      public:

        Scanner(std::string&& code);

        Token ScanToken();
        std::string GetCodeAtLine(int line) const;

      private:

        void SkipWhitespace();

        char Advance();
        bool IsAtEnd() const;
        bool MatchChar(char);
        char Peek();
        char PeekNext();
        char PeekPrevious();

        Token ErrorToken(const std::string& message);
        Token Identifier();
        Token Number();
        Token MakeToken(TokenType) const;
        Token MakeString();
        Token MakeChar();

      private:

        std::string m_CodeString;
        std::size_t m_Start = 0, m_Current = 0;
        int m_Line = 1, m_Column = 1;
    };
  } // namespace scanner
} // namespace Grace

template<>
struct fmt::formatter<Grace::Scanner::TokenType> : fmt::formatter<std::string_view>
  {
    template<typename FormatContext>
    auto format(Grace::Scanner::TokenType type, FormatContext& context) -> decltype(context.out())
    {
      using namespace Grace::Scanner;

      std::string_view name = "unknown";
      switch (type) {
        case TokenType::Char: name = "TokenType::Char"; break;
        case TokenType::EndOfFile: name = "TokenType::EndOfFile"; break;
        case TokenType::Error: name = "TokenType::Error"; break;
        case TokenType::Double: name = "TokenType::Float"; break;
        case TokenType::Identifier: name = "TokenType::Identifier"; break;
        case TokenType::Integer: name = "TokenType::Integer"; break;
        case TokenType::String: name = "TokenType::String"; break;
        case TokenType::Colon: name = "TokenType::Colon"; break;
        case TokenType::Semicolon: name = "TokenType::Semicolon"; break;
        case TokenType::LeftParen: name = "TokenType::LeftParen"; break;
        case TokenType::RightParen: name = "TokenType::RightParen"; break;
        case TokenType::Comma: name = "TokenType::Comma"; break;
        case TokenType::Dot: name = "TokenType::Dot"; break;
        case TokenType::DotDot: name = "TokenType::DotDot"; break;
        case TokenType::Minus: name = "TokenType::Minus"; break;
        case TokenType::Mod: name = "TokenType::Mod"; break;
        case TokenType::Plus: name = "TokenType::Plus"; break;
        case TokenType::Slash: name = "TokenType::Slash"; break;
        case TokenType::Star: name = "TokenType::Star"; break;
        case TokenType::StarStar: name = "TokenType::StarStar"; break;
        case TokenType::Bang: name = "TokenType::Bang"; break;
        case TokenType::BangEqual: name = "TokenType::BangEqual"; break;
        case TokenType::Equal: name = "TokenType::Equal"; break;
        case TokenType::EqualEqual: name = "TokenType::EqualEqual"; break;
        case TokenType::LessThan: name = "TokenType::LessThan"; break;
        case TokenType::GreaterThan: name = "TokenType::GreaterThan"; break;
        case TokenType::LessEqual: name = "TokenType::LessEqual"; break;
        case TokenType::GreaterEqual: name = "TokenType::GreaterEqual"; break;
        case TokenType::Assert: name = "TokenType::Assert"; break;
        case TokenType::Break: name = "TokenType::Break"; break;
        case TokenType::By: name = "TokenType::By"; break;
        case TokenType::Class: name = "TokenType::Class"; break;
        case TokenType::End: name = "TokenType::End"; break;
        case TokenType::Else: name = "TokenType::Else"; break;
        case TokenType::False: name = "TokenType::False"; break;
        case TokenType::Final: name = "TokenType::Final"; break;
        case TokenType::For: name = "TokenType::For"; break;
        case TokenType::Func: name = "TokenType::Func"; break;
        case TokenType::If: name = "TokenType::If"; break;
        case TokenType::In: name = "TokenType::In"; break;
        case TokenType::InstanceOf: name = "TokenType::InstanceOf"; break;
        case TokenType::Null: name = "TokenType::Null"; break;
        case TokenType::While: name = "TokenType::While"; break;
        case TokenType::Print: name = "TokenType::Print"; break;
        case TokenType::PrintLn: name = "TokenType::PrintLn"; break;
        case TokenType::Return: name = "TokenType::Return"; break;
        case TokenType::This: name = "TokenType::This"; break;
        case TokenType::True: name = "TokenType::True"; break;
        case TokenType::Var: name = "TokenType::Var"; break;
        case TokenType::Or: name = "TokenType::Or"; break;
        case TokenType::And: name = "TokenType::And"; break;
        case TokenType::IntIdent: name = "TokenType::IntIdent"; break;
        case TokenType::FloatIdent: name = "TokenType::FloatIdent"; break;
        case TokenType::BoolIdent: name = "TokenType::BoolIdent"; break;
        case TokenType::StringIdent: name = "TokenType::StringIdent"; break;
        case TokenType::CharIdent: name = "TokenType::CharIdent"; break;
      }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};

