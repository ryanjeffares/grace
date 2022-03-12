#include "compiler.h"

using namespace Grace::Compiler;
using namespace Grace::Scanner;

void Grace::Compiler::Compile(const std::string& filePath, bool verbose)
{
  using namespace std::chrono;
  auto start = steady_clock::now();

  std::ifstream inFile(filePath, std::ios::in);
  std::stringstream inStream;
  inStream << inFile.rdbuf();

  Compiler compiler(std::move(inStream));
  compiler.Advance();

  while (!compiler.Match(TokenType::EndOfFile)) {
    compiler.Advance();
  }

  if (verbose) {
    auto end = steady_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    fmt::print("Compilation succeeded in {} μs.\n", duration);
  }
}

Compiler::Compiler(std::stringstream&& code) : m_Scanner(std::move(code))
{

}

void Compiler::Advance()
{
  if (m_Current.has_value()) {
    fmt::print("{}\n", m_Current.value().ToString());
  }
  m_Previous = m_Current;
  m_Current = m_Scanner.ScanToken();
}

bool Compiler::Match(TokenType type)
{
  if (!Check(type)) {
    return false;
  }

  Advance();
  return true;
}

bool Compiler::Check(TokenType type) const
{
  return m_Current.has_value() && m_Current.value().GetType() == type;
}
