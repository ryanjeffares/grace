/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceString clas, which represents a string in Grace.
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include "grace_string.hpp"

using namespace Grace;

GraceString::GraceString(const std::string& str)
  : m_String(str)
{

}

GraceString::GraceString(std::string&& str)
  : m_String(std::move(str))
{

}

GraceString::GraceString(const GraceString& other)
  : m_String(other.m_String)
{

}

GraceString::GraceString(GraceString&& other)
  : m_String(std::move(other.m_String))
{

}