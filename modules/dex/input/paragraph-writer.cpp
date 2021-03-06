// Copyright (C) 2020 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/input/paragraph-writer.h"

#include "dex/model/inline-math.h"
#include "dex/model/display-math.h"
#include "dex/model/paragraph-annotations.h"

#include "dex/input/parser-errors.h"

#include <tex/parsing/mathparserfrontend.h>

#include <cassert>
#include <stdexcept>

namespace dex
{

ParagraphWriter::ParagraphWriter()
{
  m_output = std::make_shared<dex::Paragraph>();
}

ParagraphWriter::~ParagraphWriter()
{

}

void ParagraphWriter::write(char c)
{
  if (m_math_parser)
    m_math_parser->writeChar(c);

  output()->addChar(c);
}

void ParagraphWriter::write(const std::string& str)
{
  output()->addText(str);
}

void ParagraphWriter::writeCs(const std::string& str)
{
  if (!m_math_parser)
    throw std::runtime_error{ "Unknown control sequence outside of math mode" };

  m_math_parser->writeControlSequence(str);

  write("\\" + str);
  write(' ');
}

void ParagraphWriter::mathshift()
{
  if (m_math_parser)
  {
    m_math_parser->finish();
    tex::MathList mlist = m_math_parser->output();

    auto data = m_pending_metadata.back();

    if (!data->is<dex::InlineMath>())
      throw std::runtime_error{ "ParagraphWriter::mathshift() mismatch" };

    m_pending_metadata.pop_back();

    dex::Paragraph& par = *output();
    par.addChar('$');
    DisplayMath::normalize(par.text(), data->range().begin());
    size_t end = par.length();
    data->range() = dex::ParagraphRange(par, data->range().begin(), end);

    static_cast<dex::GenericParagraphMetaData<dex::InlineMath>*>(data.get())->value().mlist = mlist;

    par.addMetaData(data);
  }
  else
  {
    dex::Paragraph& par = *output();
    size_t start = par.length();
    dex::ParagraphRange range{ par, start, par.length() };

    auto data = std::make_shared<dex::GenericParagraphMetaData<dex::InlineMath>>(range, dex::InlineMath());
    m_pending_metadata.push_back(data);

    par.addChar('$');

    m_math_parser.reset(new tex::parsing::MathParserFrontend);
  }
}

void ParagraphWriter::alignmenttab()
{
  m_math_parser->alignmentTab();
  write('&');
}

void ParagraphWriter::superscript()
{
  if (m_math_parser)
    m_math_parser->beginSuperscript();

  write('^');
}

void ParagraphWriter::subscript()
{
  if (m_math_parser)
    m_math_parser->beginSubscript();

  write('_');
}

void ParagraphWriter::begintextbf()
{
  beginStyledText("bold");
}

void ParagraphWriter::endtextbf()
{
  endStyledText("bold");
}

void ParagraphWriter::begintextit()
{
  beginStyledText("italic");
}

void ParagraphWriter::endtextit()
{
  endStyledText("italic");
}

void ParagraphWriter::begintexttt()
{
  beginStyledText("code");
}

void ParagraphWriter::endtexttt()
{
  endStyledText("code");
}

void ParagraphWriter::writeLink(std::string url, const std::string& text)
{
  dex::Paragraph& par = *output();
  size_t start = par.length();
  par.addText(text);

  auto link = std::make_shared<dex::Link>(dex::ParagraphRange(par, start, par.length()), std::move(url));
  par.addMetaData(link);
}

void ParagraphWriter::writeStyledText(std::string style_name, const std::string& text)
{
  dex::Paragraph& par = *output();
  size_t start = par.length();
  par.addText(text);

  auto style = std::make_shared<dex::TextStyle>(dex::ParagraphRange(par, start, par.length()), std::move(style_name));
  par.addMetaData(style);
}

void ParagraphWriter::writeSince(const std::string& version, const std::string& text)
{
  dex::Paragraph& par = *output();
  size_t start = par.length();
  par.addText(text);

  par.add<dex::Since>(dex::ParagraphRange(par, start, par.length()), version);
}

void ParagraphWriter::index(std::string key)
{
  dex::Paragraph& par = out();
  par.add<dex::ParIndexEntry>(dex::ParagraphRange(par, par.length(), par.length()), std::move(key));
}

void ParagraphWriter::finish()
{
  dex::Paragraph& par = *output();
  
  // Removing trailing space, if any
  if (par.length() > 0 && par.text().back() == ' ')
  {
    par.text().pop_back();

    for (auto data : par.metadata())
    {
      if (data->range().end() > par.length())
      {
        const dex::ParagraphRange parrange = data->range();
        data->range() = dex::ParagraphRange(parrange.paragraph(), parrange.begin(), par.length());
      }
    }
  }
}

std::shared_ptr<dex::Paragraph> ParagraphWriter::output() const
{
  return m_output;
}

dex::Paragraph& ParagraphWriter::out()
{
  return *m_output;
}


void ParagraphWriter::beginStyledText(std::string style)
{
  dex::Paragraph& par = *output();
  size_t start = par.length();

  auto data = std::make_shared<dex::TextStyle>(dex::ParagraphRange(par, start, par.length()), std::move(style));
  m_pending_metadata.push_back(data);
}

void ParagraphWriter::endStyledText(const char* style)
{
  auto data = m_pending_metadata.back();

  if (!data->is<dex::TextStyle>() || static_cast<dex::TextStyle*>(data.get())->style() != style)
    throw std::runtime_error{ "ParagraphWriter::endStyledText() mismatch" };

  m_pending_metadata.pop_back();

  dex::Paragraph& par = *output();
  size_t end = par.length();
  data->range() = dex::ParagraphRange(par, data->range().begin(), end);

  par.addMetaData(data);
}

} // namespace dex
