#include "vm.hpp"

using namespace Grace::VM;

[[nodiscard]] 
static bool HandleAddition(const Constant& c1, const Constant& c2, std::vector<Constant>& stack)
{
  switch (c1.GetType()) {
    case Constant::Type::Int: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() + c2.Get<std::int64_t>());
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) + c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Double: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<double>() + static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(c1.Get<double>() + c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Char: {
      if (c2.GetType() == Constant::Type::Char) {
        std::string res;
        res.push_back(c1.Get<char>());
        res.push_back(c2.Get<char>());
        stack.emplace_back(res);
        return true;
      }
      return false;
    }
    case Constant::Type::String: {
      switch (c2.GetType()) {
        case Constant::Type::String: {
          stack.emplace_back(c1.Get<std::string>() + c2.Get<std::string>());
          return true;
        }
        case Constant::Type::Char: {
          stack.emplace_back(c1.Get<std::string>() + c2.Get<char>());
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
static bool HandleSubtraction(const Constant& c1, const Constant& c2, std::vector<Constant>& stack)
{
  switch (c1.GetType()) {
    case Constant::Type::Int: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() - c2.Get<std::int64_t>());
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) - c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Double: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<double>() - static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Constant::Type::Double: {
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

[[nodiscard]]
static bool HandleDivision(const Constant& c1, const Constant& c2, std::vector<Constant>& stack)
{
  switch (c1.GetType()) {
    case Constant::Type::Int: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() / c2.Get<std::int64_t>());
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) / c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Double: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<double>() / static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Constant::Type::Double: {
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

[[nodiscard]]
static bool HandleMultiplication(const Constant& c1, const Constant& c2, std::vector<Constant>& stack)
{
  switch (c1.GetType()) {
    case Constant::Type::Int: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() * c2.Get<std::int64_t>());
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) * c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Double: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<double>() * static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(c1.Get<double>() * c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Char: {
      if (c2.GetType() == Constant::Type::Int) {
        stack.emplace_back(std::string(c2.Get<std::int64_t>(), c1.Get<char>()));
        return true;
      }
      return false;
    }
    case Constant::Type::String: {
      if (c2.GetType() == Constant::Type::Int) {
        std::string res;
        for (auto i = 0; i < c2.Get<std::int64_t>(); i++) {
          res += c1.Get<std::string>();
        }
        stack.emplace_back(res);
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
static bool HandleEquality(const Constant& c1, const Constant& c2, std::vector<Constant>& stack, bool equal)
{
  switch (c1.GetType()) {
    case Constant::Type::Int: {
      switch (c2.GetType()) {
        case Constant::Type::Double: {
          stack.emplace_back(equal 
              ? static_cast<double>(c1.Get<std::int64_t>()) == c2.Get<double>()
              : static_cast<double>(c1.Get<std::int64_t>()) != c2.Get<double>());
          return true;
        }
        case Constant::Type::Int: {
          stack.emplace_back(equal 
              ? c1.Get<std::int64_t>() == c2.Get<std::int64_t>()
              : c1.Get<std::int64_t>() != c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Double: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(equal 
              ? c1.Get<double>() == static_cast<double>(c2.Get<std::int64_t>())
              : c1.Get<double>() != static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(equal 
              ? c1.Get<double>() == c2.Get<double>()
              : c1.Get<double>() != c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Bool: {
      if (c2.GetType() == Constant::Type::Bool) {
        stack.emplace_back(equal 
            ? c1.Get<bool>() == c2.Get<bool>()
            : c1.Get<bool>() != c2.Get<bool>());
        return true;
      }
      return false;
    }
    case Constant::Type::Char: {
      switch (c2.GetType()) {
        case Constant::Type::String: {
          stack.emplace_back(c2.Get<std::string>().length() == 1 
              && equal 
              ? c1.Get<char>() == c2.Get<std::string>()[0]
              : c1.Get<char>() != c2.Get<std::string>()[0]);
          return true;
        }
        case Constant::Type::Char: {
          stack.emplace_back(c1.Get<std::string>().length() == 1 
              && equal
              ? c1.Get<char>() == c2.Get<char>()
              : c1.Get<char>() != c2.Get<char>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::String: {
      switch (c2.GetType()) {
        case Constant::Type::String: {
          stack.emplace_back(equal 
              ? c1.Get<std::string>() == c2.Get<std::string>()
              : c1.Get<std::string>() != c2.Get<std::string>());
          return true;
        }
        case Constant::Type::Char: {
          stack.emplace_back(c1.Get<std::string>().length() == 1 
              && equal
              ? c1.Get<std::string>()[0] == c2.Get<char>()
              : c1.Get<std::string>()[0] != c2.Get<char>());
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
static bool HandleLessThan(const Constant& c1, const Constant& c2, std::vector<Constant>& stack)
{
  switch (c1.GetType()) {
    case Constant::Type::Int: {
      switch (c2.GetType()) {
        case Constant::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) < c2.Get<double>());
          return true;
        }
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() < c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Double: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<double>() < static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(c1.Get<double>() < c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Char: {
      if (c2.GetType() == Constant::Type::Char) {
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
static bool HandleLessEqual(const Constant& c1, const Constant& c2, std::vector<Constant>& stack)
{
  switch (c1.GetType()) {
    case Constant::Type::Int: {
      switch (c2.GetType()) {
        case Constant::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) <= c2.Get<double>());
          return true;
        }
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() <= c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Double: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<double>() <= static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(c1.Get<double>() <= c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Char: {
      if (c2.GetType() == Constant::Type::Char) {
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
static bool HandleGreaterThan(const Constant& c1, const Constant& c2, std::vector<Constant>& stack)
{
  switch (c1.GetType()) {
    case Constant::Type::Int: {
      switch (c2.GetType()) {
        case Constant::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) > c2.Get<double>());
          return true;
        }
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() > c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Double: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<double>() > static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(c1.Get<double>() > c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Char: {
      if (c2.GetType() == Constant::Type::Char) {
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
static bool HandleGreaterEqual(const Constant& c1, const Constant& c2, std::vector<Constant>& stack)
{
  switch (c1.GetType()) {
    case Constant::Type::Int: {
      switch (c2.GetType()) {
        case Constant::Type::Double: {                                          
          stack.emplace_back(static_cast<double>(c1.Get<std::int64_t>()) >= c2.Get<double>());
          return true;
        }
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<std::int64_t>() >= c2.Get<std::int64_t>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Double: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(c1.Get<double>() >= static_cast<double>(c2.Get<std::int64_t>()));
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(c1.Get<double>() >= c2.Get<double>());
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Char: {
      if (c2.GetType() == Constant::Type::Char) {
        stack.emplace_back(c1.Get<char>() >= c2.Get<char>());
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

[[nodiscard]]
static bool HandlePower(const Constant& c1, const Constant& c2, std::vector<Constant>& stack)
{
  switch (c1.GetType()) {
    case Constant::Type::Int: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(std::pow(c1.Get<std::int64_t>(), c2.Get<std::int64_t>()));
          return true;
        }
        case Constant::Type::Double: {
          stack.emplace_back(std::pow(static_cast<double>(c1.Get<std::int64_t>()), c2.Get<double>()));
          return true;
        }
        default:
          return false;
      }
    }
    case Constant::Type::Double: {
      switch (c2.GetType()) {
        case Constant::Type::Int: {
          stack.emplace_back(std::pow(c1.Get<double>(), static_cast<double>(c2.Get<std::int64_t>())));
          return true;
        }
        case Constant::Type::Double: {
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

