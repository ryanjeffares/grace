/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the VM class, specifically the functions that handle binary and unary operations. 
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include "objects/grace_list.hpp"
#include "vm.hpp"

using namespace Grace::VM;

bool VM::HandleAddition(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() + c2.Get<std::int64_t>());
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) + c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() + static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() + c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Char) {
        std::string res;
        res.push_back(c1.Get<char>());
        res.push_back(c2.Get<char>());
        stack.emplace_back(res);
        return true;
      }
      return false;
    }
    case Value::Type::String: {
      switch (c2.GetType()) {
        case Value::Type::String: {
          stack.emplace_back(c1.Get<std::string>() + c2.Get<std::string>());
          return true;
        }
        case Value::Type::Char: {
          stack.emplace_back(c1.Get<std::string>() + c2.Get<char>());
          return true;
        }
        default: {
          stack.emplace_back(c1.Get<std::string>() + c2.AsString());
          return true;
        }
      }
    }
    default:
      return false;
  }
}

bool VM::HandleSubtraction(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() - c2.Get<std::int64_t>());
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) - c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() - static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() - c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    default:
      return false;
  }
}

bool VM::HandleDivision(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() / c2.Get<std::int64_t>());
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) / c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() / static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() / c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    default:
      return false;
  }
}

bool VM::HandleMultiplication(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() * c2.Get<std::int64_t>());
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) * c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() * static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() * c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Int) {
        stack.emplace_back(std::string(c2.Get<std::int64_t>(), c1.Get<char>()));
        return true;
      }
      return false;
    }
    case Value::Type::String: {
      if (c2.GetType() == Value::Type::Int) {
        std::string res;
        if (c2.Get<std::int64_t>() > 0) {
          for (auto i = 0; i < c2.Get<std::int64_t>(); i++) {
            res += c1.Get<std::string>();
          }
        }
        stack.emplace_back(res);
        return true;
      }
      return false;
    }
    case Value::Type::Object: {
      if (auto list = dynamic_cast<GraceList*>(c1.GetObject())) {
        if (c2.GetType() != Value::Type::Int) {
          return false;
        }
        stack.emplace_back(Value::CreateList(*list, c2.Get<std::int64_t>()));
        return true; 
      }
      return false;
    }
    default:
      return false;
  }
}

bool VM::HandleMod(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() % c2.Get<std::int64_t>());
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(std::fmod(static_cast<double>(c1.Get<std::int64_t>()), c2.Get<double>()));
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(std::fmod(c1.Get<double>(), static_cast<double>(c2.Get<std::int64_t>())));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(std::fmod(c1.Get<double>(), c2.Get<double>()));
          return true;
        }
        default:
          return false;
      }
    }
    default:
      return false;
  }
}

void VM::HandleEquality(const Value& c1, const Value& c2, std::vector<Value>& stack, bool equal)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Double: {
          stack.emplace_back(equal 
              ? static_cast<double>(c1.Get<std::int64_t>()) == c2.Get<double>()
              : static_cast<double>(c1.Get<std::int64_t>()) != c2.Get<double>());
          break;
        }
        case Value::Type::Int: {
          stack.emplace_back(equal 
              ? c1.Get<std::int64_t>() == c2.Get<std::int64_t>()
              : c1.Get<std::int64_t>() != c2.Get<std::int64_t>());
          break;
        }
        default: {
          stack.emplace_back(false);
          break;
        }
      }
      break;
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(equal 
              ? c1.Get<double>() == static_cast<double>(c2.Get<std::int64_t>())
              : c1.Get<double>() != static_cast<double>(c2.Get<std::int64_t>()));
          break;
        }
        case Value::Type::Double: {
          stack.emplace_back(equal 
              ? c1.Get<double>() == c2.Get<double>()
              : c1.Get<double>() != c2.Get<double>());
          break;
        }
        default: {
          stack.emplace_back(false);
          break;
        }
      }
      break;
    }
    case Value::Type::Bool: {
      if (c2.GetType() == Value::Type::Bool) {
        stack.emplace_back(equal 
            ? c1.Get<bool>() == c2.Get<bool>()
            : c1.Get<bool>() != c2.Get<bool>());
      } else {
        stack.emplace_back(false);
      }
      break;
    }
    case Value::Type::Char: {
      switch (c2.GetType()) {
        case Value::Type::String: {
          stack.emplace_back(c2.Get<std::string>().length() == 1 
              && equal 
              ? c1.Get<char>() == c2.Get<std::string>()[0]
              : c1.Get<char>() != c2.Get<std::string>()[0]);
          break;
        }
        case Value::Type::Char: {
          stack.emplace_back(equal
              ? c1.Get<char>() == c2.Get<char>()
              : c1.Get<char>() != c2.Get<char>());
          break;
        }
        default: {
          stack.emplace_back(false);
          break;
        }
      }
      break;
    }
    case Value::Type::String: {
      switch (c2.GetType()) {
        case Value::Type::String: {
          stack.emplace_back(equal 
              ? c1.Get<std::string>() == c2.Get<std::string>()
              : c1.Get<std::string>() != c2.Get<std::string>());
          break;
        }
        case Value::Type::Char: {
          stack.emplace_back(c1.Get<std::string>().length() == 1 
              && equal
              ? c1.Get<std::string>()[0] == c2.Get<char>()
              : c1.Get<std::string>()[0] != c2.Get<char>());
          break;
        }
        default:
          break;
      }
      break;
    }
    case Value::Type::Null: {
      switch (c2.GetType()) {
        case Value::Type::Null: {
          stack.emplace_back(true);
          break;
        }
        default:
          stack.emplace_back(false);
          break;
      }
      break;
    }
    default:
      stack.emplace_back(false);
      break;
  }
}

bool VM::HandleLessThan(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) < c2.Get<double>());
          return true;
        }
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() < c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() < static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() < c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Char) {
        stack.emplace_back(c1.Get<char>() < c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

bool VM::HandleLessEqual(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) <= c2.Get<double>());
          return true;
        }
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() <= c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() <= static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() <= c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Char) {
        stack.emplace_back(c1.Get<char>() <= c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

bool VM::HandleGreaterThan(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) > c2.Get<double>());
          return true;
        }
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() > c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() > static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() > c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Char) {
        stack.emplace_back(c1.Get<char>() > c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}


bool VM::HandleGreaterEqual(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) >= c2.Get<double>());
          return true;
        }
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() >= c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(c1.Get<double>() >= static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(c1.Get<double>() >= c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Char: {
      if (c2.GetType() == Value::Type::Char) {
        stack.emplace_back(c1.Get<char>() >= c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

bool VM::HandlePower(const Value& c1, const Value& c2, std::vector<Value>& stack)
{
  switch (c1.GetType()) {
    case Value::Type::Int: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(std::pow(c1.Get<std::int64_t>(), c2.Get<std::int64_t>()));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(std::pow(static_cast<double>(c1.Get<std::int64_t>()), c2.Get<double>()));
          return true;
        }
        default:
          return false;
      }
    }
    case Value::Type::Double: {
      switch (c2.GetType()) {
        case Value::Type::Int: {
          stack.emplace_back(std::pow(c1.Get<double>(), static_cast<double>(c2.Get<std::int64_t>())));
          return true;
        }
        case Value::Type::Double: {
          stack.emplace_back(std::pow(c1.Get<double>(), c2.Get<double>()));
          return true;
        }
        default:
          return false;
      }
    }
    default:
      return false;
  }
}

bool VM::HandleNegate(const Value& c, std::vector<Value>& stack)
{
  switch (c.GetType()) {
    case Value::Type::Int: {
      auto val = c.Get<std::int64_t>();
      stack.emplace_back(-val);
      return true;
    }
    case Value::Type::Double: {
      auto val = c.Get<double>();
      stack.emplace_back(-val);
      return true;
    }
    default:
      return false;
  }
}

