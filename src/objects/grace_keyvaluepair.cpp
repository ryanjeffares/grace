/*
 *  The Grace Programming Language.
 *
 *  This file contains the out of line definitions for the GraceKeyValuePair class, the object held by a GraceDictionary
 *
 *  Copyright (c) 2022 - Present, Ryan Jeffares.
 *  All rights reserved.
 *
 *  For licensing information, see grace.hpp
 */

#include <fmt/core.h>

#include "grace_keyvaluepair.hpp"

using namespace Grace;

GraceKeyValuePair::GraceKeyValuePair(VM::Value&& key, VM::Value&& value)
  : m_Key(std::move(key)), m_Value(std::move(value))
{

}

GraceKeyValuePair::GraceKeyValuePair(std::pair<VM::Value, VM::Value>&& pair)
  : m_Key(std::move(pair.first)), m_Value(std::move(pair.second))
{

}

void GraceKeyValuePair::DebugPrint() const
{
  fmt::print("KeyValuePair: {}\n", ToString());
}

void GraceKeyValuePair::Print(bool err) const
{
  auto stream = err ? stderr : stdout;
  fmt::print(stream, "{}", ToString());
}

void GraceKeyValuePair::PrintLn(bool err) const
{
  auto stream = err ? stderr : stdout;
  fmt::print(stream, "{}\n", ToString());
}

std::string GraceKeyValuePair::ToString() const
{
  std::string res = "{";
  if (m_Key.GetType() == VM::Value::Type::String) {
    res.push_back('"');
    res.append(m_Key.AsString());
    res.push_back('"');
  } else if (m_Key.GetType() == VM::Value::Type::Char) {
    res.push_back('\'');
    res.append(m_Key.AsString());
    res.push_back('\'');
  } else {
    res.append(m_Key.AsString());
  }

  res.append(": ");
  
  if (m_Value.GetType() == VM::Value::Type::String) {
    res.push_back('"');
    res.append(m_Value.AsString());
    res.push_back('"');
  } else if (m_Value.GetType() == VM::Value::Type::Char) {
    res.push_back('\'');
    res.append(m_Value.AsString());
    res.push_back('\'');
  } else {
    res.append(m_Value.AsString());
  }

  res.push_back('}');
  return res;
}

bool GraceKeyValuePair::AsBool() const
{
  return m_Key.AsBool() && m_Value.AsBool();
}