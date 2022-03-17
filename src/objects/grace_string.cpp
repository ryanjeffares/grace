#include "grace_string.hpp"

using namespace Grace;

String::String() : m_Data(nullptr)
{

}

String::~String()
{
  if (m_Data != nullptr) {
    delete[] m_Data;
  }
}

String::String(const String& other)
{
  if (other.m_Data != nullptr) {
    m_Data = new char[other.Length() + 1];
    std::strcpy(m_Data, other.m_Data);
    m_Data[other.Length()] = '\0';
  } else {
    m_Data = nullptr;
  }
}

String::String(String&& other)
{
  m_Data = other.m_Data;
  other.m_Data = nullptr;
}

String::String(const std::string& str)
{
  m_Data = new char[str.length() + 1];
  std::strcpy(m_Data, str.data());
  m_Data[str.length()] = '\0';
}

String::String(const char* data)
{
  if (data != nullptr) {
    auto size = std::strlen(data);
    m_Data = new char[size + 1];
    std::strcpy(m_Data, data);
    m_Data[size] = '\0';
  } else {
    m_Data = nullptr;
  }
}

String::String(const char* first, const char* second)
{
  if (first != nullptr && second == nullptr) {
    auto size = std::strlen(first);
    m_Data = new char[size + 1];
    std::strcpy(m_Data, first);
    m_Data[size] = '\0';
  } else if (first == nullptr && second != nullptr) {
    auto size = std::strlen(second);
    m_Data = new char[size + 1];
    std::strcpy(m_Data, second);
    m_Data[size] = '\0';
  } else if (first == nullptr && second == nullptr) {
    m_Data = nullptr;
  } else {
    auto size = std::strlen(first) + std::strlen(second);
    m_Data = new char[size + 1];
    std::strcpy(m_Data, first);
    std::strcpy(m_Data + std::strlen(first), second);
    m_Data[size] = '\0';
  }
}

String::String(char c)
{
  m_Data = new char[2];
  m_Data[0] = c;
  m_Data[1] = '\0';
}

String& String::operator=(const String& other)
{
  if (this != &other) {
    if (m_Data != nullptr) {
      delete[] m_Data;
    }
    if (other.m_Data != nullptr) {
      auto size = other.Length();
      m_Data = new char[size + 1];
      std::strcpy(m_Data, other.m_Data);
      m_Data[size] = '\0';
    }
  }
  return *this;
}

String& String::operator=(const std::string& other)
{
  if (m_Data != nullptr) {
    delete[] m_Data;
  }
  m_Data = new char[other.length() + 1];
  std::strcpy(m_Data, other.data());
  m_Data[other.length()] = '\0';
  return *this;
}

String& String::operator=(const char* data)
{
  if (m_Data != nullptr) {
    delete[] m_Data;
  }
  if (data != nullptr) {
    auto size = std::strlen(data);
    m_Data = new char[size + 1];
    std::strcpy(m_Data, data);
    m_Data[size] = '\0';
  }
  return *this;
}

String& String::operator=(String&& other)
{
  if (m_Data != nullptr) {
    delete[] m_Data;
  }
  m_Data = other.m_Data;
  other.m_Data = nullptr;
  return *this;
}



