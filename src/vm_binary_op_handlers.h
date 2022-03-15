#include "vm.h"

using namespace Grace::VM;

static void HandleAddition(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.m_Type) {
    case VM::Constant::Type::Int: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Int);
          c.as.m_Int = c1.as.m_Int + c2.as.m_Int;
          stack.push_back(c);
          break;
        }
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = static_cast<float>(c1.as.m_Int) + c2.as.m_Float;
          stack.push_back(c);
          break;
        }
        default:  // Unreachable
          break;
      }
      break;
    }
    case VM::Constant::Type::Float: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = c1.as.m_Float + static_cast<float>(c2.as.m_Int);
          stack.push_back(c);
          break;
        }
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = c1.as.m_Float + c2.as.m_Float;
          stack.push_back(c);
          break;
        }
        default:  // Unreachable
          break;
      }
      break;
    }
    default:      // Unreachable
      break;
  }
}

static void HandleSubtraction(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.m_Type) {
    case VM::Constant::Type::Int: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Int);
          c.as.m_Int = c1.as.m_Int - c2.as.m_Int;
          stack.push_back(c);
          break;
        }
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = static_cast<float>(c1.as.m_Int) - c2.as.m_Float;
          stack.push_back(c);
          break;
        }
        default:  // Unreachable
          break;
      }
      break;
    }
    case VM::Constant::Type::Float: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = c1.as.m_Float - static_cast<float>(c2.as.m_Int);
          stack.push_back(c);
          break;
        }
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = c1.as.m_Float - c2.as.m_Float;
          stack.push_back(c);
          break;
        }
        default:  // Unreachable
          break;
      }
      break;
    }
    default:      // Unreachable
      break;
  }
}

static void HandleDivision(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.m_Type) {
    case VM::Constant::Type::Int: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Int);
          c.as.m_Int = c1.as.m_Int / c2.as.m_Int;
          stack.push_back(c);
          break;
        }
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = static_cast<float>(c1.as.m_Int) / c2.as.m_Float;
          stack.push_back(c);
          break;
        }
        default:  // Unreachable
          break;
      }
      break;
    }
    case VM::Constant::Type::Float: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = c1.as.m_Float / static_cast<float>(c2.as.m_Int);
          stack.push_back(c);
          break;
        }
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = c1.as.m_Float / c2.as.m_Float;
          stack.push_back(c);
          break;
        }
        default:  // Unreachable
          break;
      }
      break;
    }
    default:      // Unreachable
      break;
  }
}
static void HandleMultiplication(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.m_Type) {
    case VM::Constant::Type::Int: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Int);
          c.as.m_Int = c1.as.m_Int * c2.as.m_Int;
          stack.push_back(c);
          break;
        }
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = static_cast<float>(c1.as.m_Int) * c2.as.m_Float;
          stack.push_back(c);
          break;
        }
        default:  // Unreachable
          break;
      }
      break;
    }
    case VM::Constant::Type::Float: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = c1.as.m_Float * static_cast<float>(c2.as.m_Int);
          stack.push_back(c);
          break;
        }
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Float);
          c.as.m_Float = c1.as.m_Float * c2.as.m_Float;
          stack.push_back(c);
          break;
        }
        default:  // Unreachable
          break;
      }
      break;
    }
    default:      // Unreachable
      break;
  }
}

static bool HandleEquality(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack, bool equal)
{
  switch (c1.m_Type) {
    case VM::Constant::Type::Int: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = static_cast<float>(c1.as.m_Int) == c2.as.m_Float;
          if (!equal) {
            c.as.m_Bool = !c.as.m_Bool;
          }
          stack.push_back(c);
          return true;
        }
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = c1.as.m_Int == c2.as.m_Int;
          if (!equal) {
            c.as.m_Bool = !c.as.m_Bool;
          }
          stack.push_back(c);
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = c1.as.m_Float == static_cast<float>(c2.as.m_Int);
          if (!equal) {
            c.as.m_Bool = !c.as.m_Bool;
          }
          stack.push_back(c);
          return true;
        }
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = c1.as.m_Float == c2.as.m_Float;
          if (!equal) {
            c.as.m_Bool = !c.as.m_Bool;
          }
          stack.push_back(c);
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Bool: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Bool: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = c1.as.m_Bool == c2.as.m_Bool;
          if (!equal) {
            c.as.m_Bool = !c.as.m_Bool;
          }
          stack.push_back(c);
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

static bool HandleLessThan(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.m_Type) {
    case VM::Constant::Type::Int: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = static_cast<float>(c1.as.m_Int) < c2.as.m_Float;
          stack.push_back(c);
          return true;
        }
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = c1.as.m_Int > c2.as.m_Int;
          stack.push_back(c);
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = c1.as.m_Float < static_cast<float>(c2.as.m_Int);
          stack.push_back(c);
          return true;
        }
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = c1.as.m_Float > c2.as.m_Float;
          stack.push_back(c);
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

static bool HandleGreaterThan(const VM::Constant& c1, const VM::Constant& c2, std::vector<VM::Constant>& stack)
{
  switch (c1.m_Type) {
    case VM::Constant::Type::Int: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = static_cast<float>(c1.as.m_Int) > c2.as.m_Float;
          stack.push_back(c);
          return true;
        }
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = c1.as.m_Int > c2.as.m_Int;
          stack.push_back(c);
          return true;
        }
        default:
          return false;
      }
    }
    case VM::Constant::Type::Float: {
      switch (c2.m_Type) {
        case VM::Constant::Type::Int: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = c1.as.m_Float > static_cast<float>(c2.as.m_Int);
          stack.push_back(c);
          return true;
        }
        case VM::Constant::Type::Float: {
          VM::Constant c(VM::Constant::Type::Bool);
          c.as.m_Bool = c1.as.m_Float > c2.as.m_Float;
          stack.push_back(c);
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
