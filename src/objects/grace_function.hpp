/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceFunction class, the underlying class for runtime functions in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_FUNCTION_HPP
#define GRACE_FUNCTION_HPP

#include "../grace.hpp"

#include "grace_object.hpp"
#include "../ops.hpp"
#include "../value.hpp"

namespace Grace
{  
  class GraceFunction : public GraceObject
  {
  public:

    GraceFunction(std::string name, std::size_t arity, std::string fileName, bool exported);
    ~GraceFunction() override = default;

    void DebugPrint() const override;
    void Print(bool err) const override;
    void PrintLn(bool err) const override;
    GRACE_NODISCARD std::string ToString() const override;

    GRACE_NODISCARD constexpr GRACE_INLINE bool AsBool() const override
    {
      return true;
    }

    GRACE_NODISCARD constexpr GRACE_INLINE std::string_view ObjectName() const override
    {
      return "Function";
    }

    GRACE_NODISCARD constexpr GRACE_INLINE bool IsIterable() const override
    {
      return false;
    }

    GRACE_NODISCARD constexpr GRACE_INLINE GraceObjectType ObjectType() const override
    {
      return GraceObjectType::Function;
    }

    GRACE_NODISCARD GRACE_INLINE GraceFunction* GetAsFunction() override
    {
      return this;
    }

    GRACE_INLINE void PushOp(VM::Ops op, std::size_t line)
    {
      m_OpList.push_back({ op, line });
    }

    template<VM::BuiltinGraceType Type>
    GRACE_INLINE void PushConstant(Type value)
    {
      m_ConstantList.emplace_back(std::move(value));
    }

    GRACE_INLINE void PushConstant(VM::Value value)
    {
      m_ConstantList.push_back(std::move(value));
    }

    GRACE_NODISCARD GRACE_INLINE std::size_t GetNumConstants() const
    {
      return m_ConstantList.size();
    }

    GRACE_NODISCARD GRACE_INLINE std::size_t GetNumOps() const
    {
      return m_OpList.size();
    }

    GRACE_NODISCARD GRACE_INLINE std::optional<VM::Ops> GetLastOp() const
    {
      return m_OpList.empty() ? std::optional<VM::Ops>{} : m_OpList.back().op;
    }

    GRACE_NODISCARD GRACE_INLINE const std::string& GetName() const
    {
      return m_Name;
    }

    GRACE_NODISCARD GRACE_INLINE const std::string& GetFileName() const
    {
      return m_FileName;
    }

    GRACE_NODISCARD GRACE_INLINE const std::vector<VM::OpLine>& GetOpList() const
    {
      return m_OpList;
    }

    GRACE_NODISCARD GRACE_INLINE const std::vector<VM::Value>& GetConstantList() const
    {
      return m_ConstantList;
    }

    GRACE_NODISCARD GRACE_INLINE std::size_t GetOpIndexStart() const
    {
      return m_OpIndexStart;
    }

    GRACE_NODISCARD GRACE_INLINE std::size_t GetConstantIndexStart() const
    {
      return m_ConstantIndexStart;
    }

    template<VM::BuiltinGraceType Type>
    GRACE_INLINE void SetConstantAtIndex(std::size_t index, Type value)
    {
      m_ConstantList[index] = std::move(value);
    }

    GRACE_NODISCARD GRACE_INLINE std::int64_t GetFileNameHash() const
    {
      return m_FileNameHash;
    }

    GRACE_NODISCARD GRACE_INLINE std::size_t GetArity() const
    {
      return m_Arity;
    }

    GRACE_NODISCARD GRACE_INLINE bool IsExported() const
    {
      return m_Exported;
    }

    void CombineOps(std::vector<VM::OpLine>& toFill);
    void CombineConstants(std::vector<VM::Value>& toFill);
    void PrintOps() const;

  private:

    std::string m_Name;
    std::size_t m_Arity;

    std::string m_FileName;
    std::int64_t m_FileNameHash;

    std::vector<VM::OpLine> m_OpList;
    std::vector<VM::Value> m_ConstantList;

    std::size_t m_OpIndexStart {};
    std::size_t m_ConstantIndexStart {};

    bool m_Exported;
  };
} // namespace Grace

#endif // ifndef GRACE_FUNCTION_HPP