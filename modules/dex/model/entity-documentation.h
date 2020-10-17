// Copyright (C) 2019 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_MODEL_ENTITY_DOCUMENTATION_H
#define DEX_MODEL_ENTITY_DOCUMENTATION_H

#include "dex/dex-model.h"

#include "dex/model/since.h"

#include <dom/node.h>
#include <cxx/documentation.h>

#include <optional>

namespace dex
{

class DEX_MODEL_API EntityDocumentation : public cxx::Documentation
{
private:
  std::optional<std::string> m_brief;
  std::optional<Since> m_since;
  std::vector<std::shared_ptr<dom::Node>> m_description;

public:
  EntityDocumentation() = default;

  explicit EntityDocumentation(const cxx::SourceLocation& loc);

  virtual const std::string& type() const = 0;

  template<typename T>
  bool is() const;

  std::optional<std::string>& brief();
  const std::optional<std::string>& brief() const;

  std::optional<Since>& since();
  const std::optional<Since>& since() const;

  std::vector<std::shared_ptr<dom::Node>>& description();
  const std::vector<std::shared_ptr<dom::Node>>& description() const;
};

template<typename T>
bool test_documentation_type(const EntityDocumentation& doc)
{
  return T::TypeId == doc.type();
}

template<typename T>
inline bool EntityDocumentation::is() const
{
  return test_documentation_type<T>(*this);
}

} // namespace dex

namespace dex
{

inline EntityDocumentation::EntityDocumentation(const cxx::SourceLocation& loc)
  : cxx::Documentation(loc)
{

}

inline std::optional<std::string>& EntityDocumentation::brief()
{
  return m_brief;
}

inline const std::optional<std::string>& EntityDocumentation::brief() const
{
  return m_brief;
}

inline std::optional<Since>& EntityDocumentation::since()
{
  return m_since;
}

inline const std::optional<Since>& EntityDocumentation::since() const
{
  return m_since;
}

inline std::vector<std::shared_ptr<dom::Node>>& EntityDocumentation::description()
{
  return m_description;
}

inline const std::vector<std::shared_ptr<dom::Node>>& EntityDocumentation::description() const
{
  return m_description;
}

} // namespace dex

#endif // DEX_MODEL_ENTITY_DOCUMENTATION_H