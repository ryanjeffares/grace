#pragma once

#include <cstring>
#include <functional>
#include <iostream>
#include <exception>
#include <string>
#include <string_view>

#include "../../include/fmt/core.h"
#include "../../include/fmt/format.h"

#include "grace_object.hpp"

namespace Grace 
{

  struct IndexOutOfRangeException : public std::exception
  {
    IndexOutOfRangeException(std::size_t index, std::size_t length)
      : m_Message(fmt::format("Index out of range for Grace::String::operator[](std::size_t): the index is {} but the length is {}", index, length))
    {
      
    }

    const char* what() const noexcept override
    {
      return m_Message.c_str(); 
    }

  private:
    std::string m_Message;
  };

  /*
   *  String implementation optimised for size
   *
   *  This class only holds a single `char*` to a null terminated string and is not polymorphic,
   *  therefore only taking up 8 bytes. Inheriting from the class is therefore disallowed.
   *
   *  Constructors, copy constructors, and operator overloads are defined for Grace::String, 
   *  `std::string`, and `const char*` for ease of use, however cause a lot of heap allocations 
   *  as no buffer is used for the sake of size.
   *
   *  `fmt::formatter<Grace::String>` is implemented, but there is no implementaion for 
   *  `std::ostream`. The `Data()` method will return the underlying `char*`.
   */
  class String final
  {
    public:

      String();
      ~String();

      String(const String&);
      String(String&&);

      String(const std::string&);
      String(const char*);
      String(const char*, const char*);
      String(char);

      String& operator=(const String&);
      String& operator=(const std::string&);
      String& operator=(const char*);
      String& operator=(String&&);

      bool operator==(const String& other) const
      {
        if (m_Data == nullptr || other.m_Data == nullptr) {
          return false;
        }
        return std::strcmp(m_Data, other.m_Data) == 0;
      }

      bool operator==(const std::string& str) const
      {
        if (m_Data == nullptr) {
          return false;
        }
        return std::strcmp(m_Data, str.data()) == 0;
      }

      bool operator==(const char* data) const
      {
        if (data == nullptr || m_Data == nullptr) {
          return false;
        }
        return std::strcmp(m_Data, data) == 0;
      }

      String operator+(const String& other) const
      {
        return String(m_Data, other.m_Data);
      }

      String operator+(const std::string& str) const
      {
        return String(m_Data, str.data());
      }

      String operator+(const char* data) const
      {
        return String(m_Data, data);
      }

      String& operator+=(const String& other)
      {
        if (this != &other) {
          if (m_Data != nullptr && other.m_Data != nullptr) {
            auto size = Length() + other.Length();
            auto data = new char[size + 1];
            std::strcpy(data, m_Data);
            std::strcpy(data + std::strlen(m_Data), other.m_Data);
            data[size] = '\0';
            delete[] m_Data;
            m_Data = data;
          } else if (m_Data == nullptr && other.m_Data != nullptr) {
            auto size = other.Length();
            m_Data = new char[size + 1];
            m_Data[size] = '\0';
          }
        }
        return *this;
      }

      String& operator+=(const std::string& str)
      {
        if (m_Data == nullptr) {
          m_Data = new char[str.length() + 1];
          std::strcpy(m_Data, str.data());
          m_Data[str.length()] = '\0';
          return *this;
        }

        auto size = std::strlen(m_Data) + str.length();
        auto dest = new char[size + 1];
        std::strcpy(dest, m_Data);
        std::strcpy(dest + std::strlen(m_Data), str.data());
        delete[] m_Data;
        m_Data = dest;
        m_Data[size] = '\0';
        return *this;
      }

      String& operator+=(const char* data)
      {
        if (m_Data == nullptr) {
          auto size = std::strlen(data);
          m_Data = new char[size + 1];
          std::strcpy(m_Data, data);
          m_Data[size] = '\0';
          return *this;
        }

        auto size = std::strlen(m_Data) + std::strlen(data);
        auto dest = new char[size + 1];
        std::strcpy(dest, m_Data);
        std::strcpy(dest + std::strlen(m_Data), data);
        delete[] m_Data;
        m_Data = dest;
        m_Data[size] = '\0';
        return *this;
      }

      String& operator+=(char c)
      {
        if (m_Data == nullptr) {
          m_Data = new char[2];
          m_Data[0] = c;
          m_Data[1] = '\0';
          return *this;
        }

        auto size = std::strlen(m_Data);
        auto dest = new char[size + 2];
        std::strcpy(dest, m_Data);
        delete[] m_Data;
        m_Data = dest;
        m_Data[size] = c;
        m_Data[size + 1] = '\0';
        return *this;
      }

      inline const char& operator[](std::size_t index) const
      {
        if (index >= Length()) {
          throw IndexOutOfRangeException(index, Length());
        }
        return m_Data[index];
      }

      inline std::size_t Length() const 
      { 
        if (m_Data == nullptr) {
          return 0;
        }
        return std::strlen(m_Data); 
      }

      inline const char* const Data() const { return m_Data; }

    private:
      
      char* m_Data;
  };
}

namespace std 
{
  template<>
  struct std::hash<Grace::String>
  {
    std::size_t operator()(const Grace::String& s) const
    {
      std::size_t hash = 2166136261;
      for (auto i = 0; i < s.Length(); i++) {
        hash ^= static_cast<std::uint8_t>(s[i]);
        hash *= 16777619;
      }
      return hash;
    }
  };
}

template<>
struct fmt::formatter<Grace::String> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  auto format(Grace::String str, FormatContext& context) -> decltype(context.out())
  {
    std::string_view text = str.Data() == nullptr ? "" : str.Data();
    return fmt::formatter<std::string_view>::format(text, context);
  }
};
