#include "vm.hpp"

using namespace Grace::VM;

[[nodiscard]] 
static bool HandleAddition(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.GetType()) {
    case VM::Constant::Type::Int: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<int>() + c2.Get<int>());
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(static_cast<float>(c1.Get<int>()) + c2.Get<float>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<int>() + static_cast<float>(c2.Get<int>()));
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(c1.Get<float>() + c2.Get<float>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Char: {
      if (c2.GetType() == VM::Constant::Type::Char) {
        stack.emplace_back(static_cast<char>(c1.Get<char>() + c2.Get<char>()));
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
static bool HandleSubtraction(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.GetType()) {
    case VM::Constant::Type::Int: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<int>() - c2.Get<int>());
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(static_cast<float>(c1.Get<int>()) - c2.Get<float>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<float>() - static_cast<float>(c2.Get<int>()));
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(c1.Get<float>() - c2.Get<float>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Char: {
      if (c2.GetType() == VM::Constant::Type::Char) {
        stack.emplace_back(static_cast<char>(c1.Get<char>() - c2.Get<char>()));
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
static bool HandleDivision(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.GetType()) {
    case VM::Constant::Type::Int: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<int>() / c2.Get<int>());
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(static_cast<float>(c1.Get<int>()) / c2.Get<float>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<float>() / static_cast<float>(c2.Get<int>()));
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(c1.Get<float>() / c2.Get<float>());
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

[[nodiscard]]
static bool HandleMultiplication(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.GetType()) {
    case VM::Constant::Type::Int: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<int>() * c2.Get<int>());
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(static_cast<float>(c1.Get<int>()) * c2.Get<float>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<float>() * static_cast<float>(c2.Get<int>()));
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(c1.Get<float>() * c2.Get<float>());
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

[[nodiscard]]
static bool HandleEquality(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack, bool equal)
{
  switch (c1.GetType()) {
    case VM::Constant::Type::Int: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Float: {
          stack.emplace_back(equal 
              ? static_cast<float>(c1.Get<int>()) == c2.Get<float>()
              : static_cast<float>(c1.Get<int>()) != c2.Get<float>());
          return true;
        }
        case VM::Constant::Type::Int: {
          stack.emplace_back(equal 
              ? c1.Get<int>() == c2.Get<int>()
              : c1.Get<int>() != c2.Get<int>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(equal 
              ? c1.Get<float>() == static_cast<float>(c2.Get<int>())
              : c1.Get<float>() != static_cast<float>(c2.Get<int>()));
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(equal 
              ? c1.Get<float>() == c2.Get<int>()
              : c1.Get<float>() != c2.Get<int>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Bool: {
      if (c2.GetType() == VM::Constant::Type::Bool) {
        stack.emplace_back(equal 
            ? c1.Get<bool>() == c2.Get<bool>()
            : c1.Get<bool>() != c2.Get<bool>());
        return true;
      }
      return false;
    }
    case VM::Constant::Type::Char: {
      if (c2.GetType() == VM::Constant::Type::Char) {
        stack.emplace_back(equal 
            ? c1.Get<char>() == c2.Get<char>()
            : c1.Get<char>() != c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
static bool HandleLessThan(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.GetType()) {
    case VM::Constant::Type::Int: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Float: {                                          
          stack.emplace_back(static_cast<float>(c1.Get<int>()) < c2.Get<float>());
          return true;
        }
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<int>() < c2.Get<int>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<float>() < static_cast<float>(c2.Get<int>()));
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(c1.Get<float>() < c2.Get<float>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Char: {
      if (c2.GetType() == VM::Constant::Type::Char) {
        stack.emplace_back(c1.Get<char>() < c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
static bool HandleLessEqual(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.GetType()) {
    case VM::Constant::Type::Int: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Float: {                                          
          stack.emplace_back(static_cast<float>(c1.Get<int>()) <= c2.Get<float>());
          return true;
        }
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<int>() <= c2.Get<int>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<float>() <= static_cast<float>(c2.Get<int>()));
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(c1.Get<float>() <= c2.Get<float>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Char: {
      if (c2.GetType() == VM::Constant::Type::Char) {
        stack.emplace_back(c1.Get<char>() <= c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
static bool HandleGreaterThan(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.GetType()) {
    case VM::Constant::Type::Int: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Float: {                                          
          stack.emplace_back(static_cast<float>(c1.Get<int>()) > c2.Get<float>());
          return true;
        }
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<int>() > c2.Get<int>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<float>() > static_cast<float>(c2.Get<int>()));
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(c1.Get<float>() > c2.Get<float>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Char: {
      if (c2.GetType() == VM::Constant::Type::Char) {
        stack.emplace_back(c1.Get<char>() > c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
static bool HandleGreaterEqual(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.GetType()) {
    case VM::Constant::Type::Int: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Float: {                                          
          stack.emplace_back(static_cast<float>(c1.Get<int>()) >= c2.Get<float>());
          return true;
        }
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<int>() >= c2.Get<int>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.GetType()) {
        case VM::Constant::Type::Int: {
          stack.emplace_back(c1.Get<float>() >= static_cast<float>(c2.Get<int>()));
          return true;
        }
        case VM::Constant::Type::Float: {
          stack.emplace_back(c1.Get<float>() >= c2.Get<float>());
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Char: {
      if (c2.GetType() == VM::Constant::Type::Char) {
        stack.emplace_back(c1.Get<char>() >= c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}
