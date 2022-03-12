#include <cstddef>
#include <unordered_map>
#include <optional>
#include <string>
#include <sstream>
#include <string_view>

#include <fmt/format.h>

namespace Grace
{
  namespace Scanner
  {
    enum class TokenType
    {
      // Lexical tokens
      EndOfFile,
      Error,
      Float,
      Identifier,
      Integer,
      String,
      
      // Symbols
      Colon,
      Semicolon,
      LeftParen,
      RightParen,
      Comma,
      Dot,
      Minus,
      Plus,
      Slash,
      Star,
      Bang,
      BangEqual,
      Equal,
      EqualEqual,
      LessThan,
      GreaterThan,
      LessEqual,
      GreaterEqual,

      // Keywords
      As, 
      Class, 
      End, 
      False,
      Final,
      For,
      Func,
      If,
      While,
      Print,
      This,
      True,
      Var,
    };

        class Token
    {
      public:
        
        Token(TokenType, 
            std::size_t start, 
            std::size_t length, 
            std::size_t line, 
            const std::string& code
        );

        std::string ToString() const;
        TokenType GetType() const;

      private:

        TokenType m_Type;
        std::size_t m_Start, m_Length, m_Line;
        std::string_view m_Text;
    };

    class Scanner 
    {
      public:

        Scanner(std::stringstream&& code);

        Token ScanToken();

      private:

        void SkipWhitespace();

        char Advance();
        bool IsAtEnd() const;
        bool MatchChar(char);
        char Peek();
        char PeekNext();

        Token ErrorToken(const std::string& message);
        Token Identifier();
        Token Number();
        Token MakeToken(TokenType) const;
        Token MakeString();

      private:

        std::stringstream m_CodeStream;
        std::string m_CodeString;
        std::size_t m_Start = 0, m_Current = 0;
        int m_Line = 1;
    };
  } // namespace scanner
} // namespace Grace

template<>
struct fmt::formatter<Grace::Scanner::TokenType> : fmt::formatter<std::string_view>
  {
    template<typename FormatContext>
    auto format(Grace::Scanner::TokenType type, FormatContext& context) -> decltype(context.out())
    {
      std::string_view name = "unknown";
      switch (type) {
        case Grace::Scanner::TokenType::EndOfFile: name = "EndOfFile"; break;
        case Grace::Scanner::TokenType::Error: name = "Error"; break;
        case Grace::Scanner::TokenType::Float: name = "Float"; break;
        case Grace::Scanner::TokenType::Identifier: name = "Identifier"; break;
        case Grace::Scanner::TokenType::Integer: name = "Integer"; break;
        case Grace::Scanner::TokenType::String: name = "String"; break;
        case Grace::Scanner::TokenType::Colon: name = "Colon"; break;
        case Grace::Scanner::TokenType::Semicolon: name = "Semicolon"; break;
        case Grace::Scanner::TokenType::LeftParen: name = "LeftParen"; break;
        case Grace::Scanner::TokenType::RightParen: name = "RightParen"; break;
        case Grace::Scanner::TokenType::Comma: name = "Comma"; break;
        case Grace::Scanner::TokenType::Dot: name = "Dot"; break;
        case Grace::Scanner::TokenType::Minus: name = "Minus"; break;
        case Grace::Scanner::TokenType::Plus: name = "Plus"; break;
        case Grace::Scanner::TokenType::Slash: name = "Slash"; break;
        case Grace::Scanner::TokenType::Star: name = "Star"; break;
        case Grace::Scanner::TokenType::Bang: name = "Bang"; break;
        case Grace::Scanner::TokenType::BangEqual: name = "BangEqual"; break;
        case Grace::Scanner::TokenType::Equal: name = "Equal"; break;
        case Grace::Scanner::TokenType::EqualEqual: name = "EqualEqual"; break;
        case Grace::Scanner::TokenType::LessThan: name = "LessThan"; break;
        case Grace::Scanner::TokenType::GreaterThan: name = "GreaterThan"; break;
        case Grace::Scanner::TokenType::LessEqual: name = "LessEqual"; break;
        case Grace::Scanner::TokenType::GreaterEqual: name = "GreaterEqual"; break;
        case Grace::Scanner::TokenType::As: name = "As"; break;
        case Grace::Scanner::TokenType::Class: name = "Class"; break;
        case Grace::Scanner::TokenType::End: name = "End"; break;
        case Grace::Scanner::TokenType::False: name = "False"; break;
        case Grace::Scanner::TokenType::Final: name = "Final"; break;
        case Grace::Scanner::TokenType::For: name = "For"; break;
        case Grace::Scanner::TokenType::Func: name = "Func"; break;
        case Grace::Scanner::TokenType::If: name = "If"; break;
        case Grace::Scanner::TokenType::While: name = "While"; break;
        case Grace::Scanner::TokenType::Print: name = "Print"; break;
        case Grace::Scanner::TokenType::This: name = "This"; break;
        case Grace::Scanner::TokenType::True: name = "True"; break;
        case Grace::Scanner::TokenType::Var: name = "Var"; break;
      }
    return fmt::formatter<std::string_view>::format(name, context);
  }
};


