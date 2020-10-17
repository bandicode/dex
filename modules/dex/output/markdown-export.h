// Copyright (C) 2019 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef DEX_OUTPUT_MARKDOWN_EXPORT_H
#define DEX_OUTPUT_MARKDOWN_EXPORT_H

#include "dex/output/liquid-exporter.h"
#include "dex/output/liquid-stringifier.h"

namespace dom
{
class ParagraphIterator;
} // namespace dom;

namespace dex
{

class MarkdownStringifier : public LiquidStringifier
{
public:
  explicit MarkdownStringifier(LiquidExporter& exp);

protected:
  std::string stringify_list(const dom::List& list) const override;
  std::string stringify_listitem(const dom::ListItem& li) const override;
  std::string stringify_paragraph(const dom::Paragraph& par) const override;
  std::string stringify_image(const dom::Image& img) const override;
  std::string stringify_math(const dex::DisplayMath& math) const override;
  std::string stringify_section(const dex::Sectioning& sec) const override;
};

} // namespace dex

#endif // DEX_OUTPUT_MARKDOWN_EXPORT_H
