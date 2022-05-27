/*
 *  The Grace Programming Language.
 *
 *  This file contains the GraceString clas, which represents a string in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#ifndef GRACE_STRING_HPP
#define GRACE_STRING_HPP

#include <string>

#include "../grace.hpp"
#include "grace_object.hpp"

namespace Grace
{
  class GraceString : public GraceObject
  {
    public:
      GraceString() = default;
      GraceString(const std::string&);
      GraceString(std::string&&);
      GraceString(const GraceString&);
      GraceString(GraceString&&);

      ~GraceString();

      GraceString& operator=(const std::string&);
      GraceString& operator=(std::string&&);
      GraceString& operator=(const GraceString&);
      GraceString& operator=(GraceString&&);

      void DebugPrint() const override;
      void Print() const override;
      void PrintLn() const override;
      GRACE_NODISCARD std::string ToString() const override;
      GRACE_NODISCARD bool AsBool() const override;
      GRACE_NODISCARD std::string ObjectName() const override;

    private:
      std::string m_String;
  };
} // namespace Grace

#endif  // ifndef GRACE_STRING_HPP