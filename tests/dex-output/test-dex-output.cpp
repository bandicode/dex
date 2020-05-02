// Copyright (C) 2019 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "test-dex-output.h"

#include "dex/model/since.h"

#include "dex/common/file-utils.h"

#include "dex/output/json-annotator.h"
#include "dex/output/json-export.h"
#include "dex/output/markdown-export.h"

#include <cxx/class.h>
#include <cxx/documentation.h>
#include <cxx/function.h>
#include <cxx/namespace.h>
#include <cxx/program.h>
#include <cxx/variable.h>

#include <dom/image.h>
#include <dom/list.h>
#include <dom/paragraph/link.h>
#include <dom/paragraph/textstyle.h>

#include <json-toolkit/stringify.h>

#include <iostream>

static std::shared_ptr<dom::Paragraph> make_par(const std::string& str)
{
  return std::make_shared<dom::Paragraph>(str);
}

template<typename T, typename...Args>
std::shared_ptr<T> make(Args&&... args)
{
  return std::make_shared<T>(std::forward<Args>(args)...);
}

static std::shared_ptr<cxx::Program> example_prog_with_class()
{
  auto prog = std::make_shared<cxx::Program>();
  auto global = prog->globalNamespace();

  auto vector = std::make_shared<cxx::Class>("vector", global);
  auto doc = std::make_shared<dex::ClassDocumentation>();
  doc->brief() = "sequence container that encapsulates dynamic size arrays";
  doc->description().push_back(make_par("The elements are stored contiguously, ..."));
  doc->description().push_back(make_par("The storage of the vector is handled automatically, ..."));
  vector->setDocumentation(doc);

  global->entities().push_back(vector);

  return prog;
}

static std::shared_ptr<dex::Model> example_prog_with_class_image_description()
{
  auto model = std::make_shared<dex::Model>();

  auto prog = std::make_shared<cxx::Program>();
  auto global = prog->globalNamespace();

  auto vector = std::make_shared<cxx::Class>("vector", global);
  auto doc = std::make_shared<dex::ClassDocumentation>();
  auto img = make<dom::Image>("test.jpg");
  doc->description().push_back(img);
  vector->setDocumentation(doc);

  global->entities().push_back(vector);

  model->setProgram(prog);

  return model;
}

static std::shared_ptr<dex::Model> example_prog_with_class_list_description()
{
  auto model = std::make_shared<dex::Model>();

  auto prog = std::make_shared<cxx::Program>();
  auto global = prog->globalNamespace();

  auto vector = std::make_shared<cxx::Class>("vector", global);
  auto doc = std::make_shared<dex::ClassDocumentation>();
  auto list = make<dom::List>();
  auto listitem = make<dom::ListItem>();
  listitem->content.push_back(make<dom::Paragraph>("first item"));
  list->items.push_back(listitem);
  listitem = make<dom::ListItem>();
  listitem->content.push_back(make<dom::Paragraph>("second item"));
  list->items.push_back(listitem);
  doc->description().push_back(list);
  vector->setDocumentation(doc);

  global->entities().push_back(vector);

  model->setProgram(prog);

  return model;
}

static std::shared_ptr<cxx::Program> example_prog_with_fun()
{
  auto prog = std::make_shared<cxx::Program>();
  auto global = prog->globalNamespace();

  auto getenv = std::make_shared<cxx::Function>("getenv", global);
  auto doc = std::make_shared<dex::FunctionDocumentation>();
  doc->brief() = "get value from environment variables";
  doc->parameters().push_back("name of the environment variable");
  doc->returnValue() = "value of environment variable";
  doc->since() = dex::Since{ "C++98" };
  doc->description().push_back(make_par("Searches the environment list provided by the host environment..."));
  doc->description().push_back(make_par("Modifying the string returned by getenv invokes undefined behavior."));
  getenv->setDocumentation(doc);

  global->entities().push_back(getenv);

  return prog;
}

static std::shared_ptr<cxx::Program> example_prog_with_var()
{
  auto prog = std::make_shared<cxx::Program>();
  auto global = prog->globalNamespace();

  auto variable = std::make_shared<cxx::Variable>(cxx::Type("double"), "pi", global);
  auto doc = std::make_shared<dex::VariableDocumentation>();
  doc->brief() = "the math constant pi";
  doc->description().push_back(make_par("This mathematical constant is roughly equal to 3."));
  variable->setDocumentation(doc);

  global->entities().push_back(variable);

  return prog;
}

static std::shared_ptr<cxx::Program> example_prog_with_class_and_fun()
{
  auto prog = std::make_shared<cxx::Program>();
  auto global = prog->globalNamespace();

  auto complex = std::make_shared<cxx::Class>("complex", global);
  global->entities().push_back(complex);

  auto real = std::make_shared<cxx::Function>("real", complex);
  complex->members().emplace_back(real, cxx::AccessSpecifier::PUBLIC);

  return prog;
}

void TestDexOutput::jsonExport()
{
  {
    auto model = std::make_shared<dex::Model>();
    model->setProgram(example_prog_with_class());

    json::Object jexport = dex::JsonExport::serialize(*model).toObject();

    QVERIFY(jexport.data().size() == 1);

    jexport = jexport["program"]["global_namespace"].toObject();

    QVERIFY(jexport.data().size() == 3);
    QVERIFY(jexport.data().at("entities").length() == 1);
    QVERIFY(jexport.data().at("entities").toArray().length() == 1);
    QVERIFY(jexport.data().at("entities").at(0)["name"] == "vector");
  }
  
  {
    auto model = std::make_shared<dex::Model>();
    model->setProgram(example_prog_with_fun());

    json::Object jexport = dex::JsonExport::serialize(*model).toObject();
    jexport = jexport["program"]["global_namespace"].toObject();

    QVERIFY(jexport.data().size() == 3);
    QVERIFY(jexport.data().at("entities").length() == 1);
    QVERIFY(jexport.data().at("entities").toArray().length() == 1);
    QVERIFY(jexport.data().at("entities").at(0)["name"] == "getenv");

    jexport = jexport.data().at("entities").at(0)["documentation"].toObject();
    QVERIFY(jexport["since"] == "C++98");
    QVERIFY(jexport["parameters"].at(0) == "name of the environment variable");
    QVERIFY(jexport["returns"] == "value of environment variable");
  }

  {
    auto model = std::make_shared<dex::Model>();
    model->setProgram(example_prog_with_var());

    json::Object jexport = dex::JsonExport::serialize(*model).toObject();
    jexport = jexport["program"]["global_namespace"].toObject();

    QVERIFY(jexport.data().size() == 3);
    QVERIFY(jexport.data().at("entities").length() == 1);
    QVERIFY(jexport.data().at("entities").toArray().length() == 1);
    QVERIFY(jexport.data().at("entities").at(0)["name"] == "pi");
    QVERIFY(jexport.data().at("entities").at(0)["vartype"] == "double");

    jexport = jexport.data().at("entities").at(0)["documentation"].toObject();
    QVERIFY(jexport["description"].at(0)["text"] == "This mathematical constant is roughly equal to 3.");
  }
}

void TestDexOutput::jsonAnnotator()
{
  {
    auto model = std::make_shared<dex::Model>();
    model->setProgram(example_prog_with_class());

    json::Object jexport = dex::JsonExport::serialize(*model).toObject();

    dex::JsonPathAnnotator annotator;
    annotator.annotate(jexport);

    QVERIFY(jexport.data().size() == 1);

    jexport = jexport["program"]["global_namespace"].toObject();

    QVERIFY(jexport.data().size() == 4);
    QVERIFY(jexport.data().at("entities").length() == 1);
    QVERIFY(jexport.data().at("entities").toArray().length() == 1);

    json::Object vec = jexport.data().at("entities").at(0).toObject();
    QVERIFY(vec["_path"] == "$.program.global_namespace.entities[0]");

    auto path = dex::JsonPathAnnotator::parse("$.program.global_namespace.entities[0]");
    auto expected = std::vector<std::variant<size_t, std::string>>{ std::string("program"), std::string("global_namespace"), std::string("entities"), 0 };
    QVERIFY(path == expected);
  }

  {
    auto model = std::make_shared<dex::Model>();
    model->setProgram(example_prog_with_class_and_fun());

    json::Object jexport = dex::JsonExport::serialize(*model);

    dex::JsonPathAnnotator annotator;
    annotator.annotate(jexport);

    QVERIFY(jexport.data().size() == 1);

    jexport = jexport["program"]["global_namespace"].toObject();

    json::Object complex = jexport.data().at("entities").at(0).toObject();
    QVERIFY(complex["_path"] == "$.program.global_namespace.entities[0]");
    json::Object real = complex.data().at("members").at(0).toObject();
    QVERIFY(real["_path"] == "$.program.global_namespace.entities[0].members[0]");

    auto path = dex::JsonPathAnnotator::parse("$.program.global_namespace.entities[0].members[0]");
    auto expected = std::vector<std::variant<size_t, std::string>>{ std::string("program"), std::string("global_namespace"), std::string("entities"), 0, std::string("members"), 0 };
    QVERIFY(path == expected);
  }
}

void TestDexOutput::markdownExport()
{
  {
    auto model = std::make_shared<dex::Model>();
    model->setProgram(example_prog_with_class());

    dex::MarkdownExport md_export;

    md_export.dump(model, QDir::current());

    std::string content = dex::file_utils::read_all("classes/vector.md");

    const std::string expected =
      "\n# vector Class\n\n**Brief:** sequence container that encapsulates dynamic size arrays\n\n"
      "## Detailed description\n\nThe elements are stored contiguously, ...\n\n"
      "The storage of the vector is handled automatically, ...\n\n"
      "## Members documentation\n\n";

    QVERIFY(content == expected);
  }

  {
    auto model = example_prog_with_class_image_description();

    dex::MarkdownExport md_export;

    md_export.dump(model, QDir::current());

    std::string content = dex::file_utils::read_all("classes/vector.md");

    const std::string expected =
      "\n# vector Class\n\n"
      "## Detailed description\n\n![image](test.jpg)\n\n"
      "## Members documentation\n\n";

    QVERIFY(content == expected);
  }

  {
    auto model = example_prog_with_class_list_description();

    dex::MarkdownExport md_export;

    md_export.dump(model, QDir::current());

    std::string content = dex::file_utils::read_all("classes/vector.md");

    const std::string expected =
      "\n# vector Class\n\n"
      "## Detailed description\n\n- first item\n- second item\n\n"
      "## Members documentation\n\n";

    QVERIFY(content == expected);
  }
}
