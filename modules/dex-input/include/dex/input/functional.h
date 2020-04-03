// Copyright (C) 2020 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_INPUT_FUNCTIONAL_H
#define DEX_INPUT_FUNCTIONAL_H

#include "dex/dex-input.h"

#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace dex
{

typedef std::variant<bool, int, double, std::string> Argument;
typedef std::map<std::string, Argument> Options;

struct FunctionCall
{
  std::string function;
  std::vector<Argument> arguments;
  Options options;

  template<typename T>
  const T& arg(size_t index) const
  {
    return std::get<T>(arguments.at(index));
  }

  Argument opt(const std::string& key, Argument default_value) const
  {
    auto it = options.find(key);
    return it == options.end() ? default_value : it->second;
  }

  template<typename T, typename = decltype(std::get<T>(std::declval<Argument>()))>
  T opt(const std::string& key, T default_value) const
  {
    auto it = options.find(key);
    return it == options.end() ? default_value : std::get<T>(it->second);
  }
};

struct DEX_INPUT_API Functions
{
  static const std::string PAR;

  static const std::string CLASS;
  static const std::string ENDCLASS;
  static const std::string FUNCTION;
  static const std::string ENDFN;
  static const std::string NAMESPACE;
  static const std::string ENDNAMESPACE;
  static const std::string ENUM;
  static const std::string ENDENUM;
  static const std::string ENUMVALUE;
  static const std::string ENDENUMVALUE;
  static const std::string BRIEF;
  static const std::string SINCE;
  static const std::string BEGINSINCE;
  static const std::string ENDSINCE;
  static const std::string PARAM;
  static const std::string RETURNS;

  static const std::string IMAGE;

  static const std::string LIST;
  static const std::string ENDLIST;
  static const std::string LI;
};

} // namespace dex

#endif // DEX_INPUT_FUNCTIONAL_H