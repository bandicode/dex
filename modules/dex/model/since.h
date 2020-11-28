// Copyright (C) 2019-2020 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_MODEL_SINCE_H
#define DEX_MODEL_SINCE_H

#include "dex/dex-model.h"

#include <dom/element.h>

namespace dex
{

class DEX_MODEL_API Since
{
private:
  std::string m_version;

public:
  Since(std::string version);

  static const std::string TypeId;

  const std::string& version() const;
};

class DEX_MODEL_API BeginSince : public dom::Element
{
public:
  std::string version;

public:
  explicit BeginSince(std::string v);

  static const std::string TypeId;
  const std::string& className() const override;
};

class DEX_MODEL_API EndSince : public dom::Element
{
public:
  std::weak_ptr<BeginSince> beginsince;

public:
  explicit EndSince(std::shared_ptr<BeginSince> bsince);

  static const std::string TypeId;
  const std::string& className() const override;
};

} // namespace dex

#endif // DEX_MODEL_SINCE_H
