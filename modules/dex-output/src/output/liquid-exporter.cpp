// Copyright (C) 2019 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/output/liquid-exporter.h"

#include "dex/output/json-annotator.h"
#include "dex/output/json-export.h"

#include "dex/common/errors.h"

#include "dex/model/model.h"

#include <cxx/class.h>
#include <cxx/documentation.h>
#include <cxx/function.h>
#include <cxx/namespace.h>
#include <cxx/program.h>

#include <dom/image.h>
#include <dom/list.h>

namespace dex
{

struct ClassDumper : ModelVisitor
{
  LiquidExporter& exporter;
  json::Object& serializedModel;

  ClassDumper(LiquidExporter& e, json::Object& m)
    : exporter{ e },
    serializedModel{ m }
  {
  }

  void visit_class(const cxx::Class& cla) override
  {
    json::Object obj = JsonPathAnnotator::get(path(), serializedModel).toObject();

    exporter.dump(cla, obj);

    ModelVisitor::visit_class(cla);
  }
};

void LiquidExporter::setOutputDir(const QDir& dir)
{
  m_output_dir = dir;
}

void LiquidExporter::render()
{
  dumpClasses();
}

Model::Path LiquidExporter::convertToModelPath(const JsonPath& jspath)
{
  Model::Path result;

  for (const JsonPathElement& jspath_elem : jspath)
  {
    if (std::holds_alternative<size_t>(jspath_elem))
      result.back().index = std::get<size_t>(jspath_elem);
    else
      result.push_back(Model::PathElement(std::get<std::string>(jspath_elem)));
  }

  return result;
}

void LiquidExporter::dumpClasses()
{
  if (m_templates.class_template.nodes().empty())
    return;

  if (!model()->program())
    return;

  ClassDumper dumper{ *this, m_serialized_model };
  dumper.visit(*model());
}

void LiquidExporter::dump(const cxx::Class& cla, const json::Object& obj)
{
  if (obj["url"] == nullptr)
  {
    return;
  }

  const std::string url = obj["url"].toString();

  json::Object context;
  context["model"] = m_serialized_model;
  context["class"] = obj;

  // @TODO: remove this 'prog' property
  context["prog"] = m_serialized_model["program"];

  std::string output = liquid::Renderer::render(m_templates.class_template, context);

  postProcess(output);

  QFileInfo fileinfo{ m_output_dir.absolutePath() + "/" + QString::fromStdString(url) };

  if (!fileinfo.dir().exists())
  {
    const bool success = QDir().mkpath(fileinfo.dir().absolutePath());

    if (!success)
      throw IOException{ fileinfo.dir().absolutePath().toStdString(), "could not create directory" };
  }

  QFile file{ fileinfo.absoluteFilePath() };

  if (!file.open(QIODevice::WriteOnly))
    throw IOException{ fileinfo.absoluteFilePath().toStdString(), "could not open file for writing" };

  file.write(output.data());

  file.close();
}

void LiquidExporter::setModel(std::shared_ptr<Model> model)
{
  m_model = model;
  
  m_serialized_model = JsonExport::serialize(*model).toObject();

  JsonPathAnnotator path_annotator;
  path_annotator.annotate(m_serialized_model);
}

std::string LiquidExporter::stringify(const json::Json& val)
{
  if (!val.isObject() && !val.isArray())
    return Renderer::stringify(val);
  else if (val.isArray())
    return stringify_array(val.toArray());

  json::Object obj = val.toObject();

  auto path_it = obj.data().find("_path");

  if (path_it != obj.data().end())
  {
    std::vector<std::variant<size_t, std::string>> json_path = JsonPathAnnotator::parse(path_it->second.toString());
    Model::Path model_path = convertToModelPath(json_path);
    Model::Node model_node = model()->get(model_path);

    if (std::holds_alternative<std::shared_ptr<dom::Node>>(model_node))
    {
      auto dom_node = std::get< std::shared_ptr<dom::Node>>(model_node);
      return stringify_domnode(*dom_node);
    }
  }
  else
  {
    assert(("element has no path", false));
    return {};
  }

  assert(("Not implemented", false));
  return {};
}

std::string LiquidExporter::stringify_domnode(const dom::Node& node)
{
  if (node.is<dom::Paragraph>())
    return stringify_paragraph(static_cast<const dom::Paragraph&>(node));
  else if (node.is<dom::List>())
    return stringify_list(static_cast<const dom::List&>(node));
  else if (node.is<dom::ListItem>())
    return stringify_listitem(static_cast<const dom::ListItem&>(node));
  else if (node.is<dom::Image>())
    return stringify_image(static_cast<const dom::Image&>(node));

  assert(("dom element not implemented", false));
  return {};
}

std::string LiquidExporter::stringify_domcontent(const dom::Content& content)
{
  std::string result;

  for (const auto& node : content)
  {
    result += stringify_domnode(*node);
  }

  return result;
}

void LiquidExporter::postProcess(std::string& output)
{
  /* no-op */
}

void LiquidExporter::trim_right(std::string& str)
{
  // Remove spaces before end of line '\n'
  {
    size_t w = 0;

    for (size_t r = 0; r < str.size();)
    {
      if (str.at(r) == ' ')
      {
        // Find next non space char
        size_t rr = r;

        while (rr < str.size() && str.at(rr) == ' ') ++rr;

        if (str.at(rr) == '\n') // discard the spaces
        {
          r = rr;
        }
        else // keep the spaces
        {
          while (r < rr)
            str[w++] = str[r++];
        }
      }
      else
      {
        str[w++] = str[r++];
      }
    }

    str.resize(w);
  }
}

void LiquidExporter::simplify_empty_lines(std::string& str)
{
  size_t w = 0;

  for (size_t r = 0; r < str.size();)
  {
    if (str.at(r) == '\n')
    {
      // Find next non new-line char
      size_t rr = r;

      while (rr < str.size() && str.at(rr) == '\n') ++rr;

      if (rr - r >= 3) // discard the extra '\n'
      {
        r = rr;
        str[w++] = '\n';
        str[w++] = '\n';
      }
      else // keep the newlines
      {
        while (r < rr)
          str[w++] = str[r++];
      }
    }
    else
    {
      str[w++] = str[r++];
    }
  }

  str.resize(w);
}

static json::Array array_arg(const json::Json& object)
{
  if (object.isNull())
    return json::Array();
  else if (object.isArray())
    return object.toArray();
  else
    throw std::runtime_error{ "Object is not an array" };
}

json::Json LiquidExporter::applyFilter(const std::string& name, const json::Json& object, const std::vector<json::Json>& args)
{
  if (name == "filter_by_type")
  {
    return filter_by_type(array_arg(object), args.front().toString());
  }
  else if (name == "filter_by_accessibility")
  {
    return filter_by_accessibility(array_arg(object), args.front().toString());
  }
  else if (name == "filter_by_field")
  {
    return filter_by_field(array_arg(object), args.front().toString(), args.back().toString());
  }

  return liquid::Renderer::applyFilter(name, object, args);
}

json::Array LiquidExporter::filter_by_field(const json::Array& list, const std::string& field, const std::string& value)
{
  json::Array result;

  for (const auto& obj : list.data())
  {
    if (obj[field] == value)
      result.push(obj);
  }

  return result;
}

json::Array LiquidExporter::filter_by_type(const json::Array& list, const std::string& type)
{
  static const std::string field = "type";
  return filter_by_field(list, field, type);
}

json::Array LiquidExporter::filter_by_accessibility(const json::Array& list, const std::string& as)
{
  static const std::string field = "accessibility";
  return filter_by_field(list, field, as);
}

} // namespace dex
