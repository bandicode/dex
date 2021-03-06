// Copyright (C) 2019-2021 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "test-dex-input.h"

#include "dex/model/model.h"

#include "dex/input/paragraph-writer.h"
#include "dex/input/parser-machine.h"

#include "dex/input/function-caller.h"
#include "dex/input/conditional-evaluator.h"
#include "dex/input/document-writer.h"

#include <QFile>

tex::parsing::Token tok(std::string str)
{
  return tex::parsing::Token{ std::move(str) };
}

tex::parsing::Token tok(char c)
{
  return tex::parsing::Token{ tex::parsing::CharacterToken{c, tex::parsing::Lexer::DefaultCatCodes[static_cast<unsigned char>(c)] } };
}

void write_chars(dex::FunctionCaller& parser, const std::string& str)
{
  for (char c : str)
  {
    parser.write(tok(c));
  }
}

void write_chars_lexer(dex::FunctionCaller& parser, const std::string& str)
{
  for (char c : str)
  {
    parser.machine().lexer().write(c);

    if (!parser.machine().lexer().output().empty())
    {
      tex::parsing::Token tok = tex::parsing::read(parser.machine().lexer().output());
      parser.write(std::move(tok));
    }
  }
}

void write_cs(dex::FunctionCaller& parser, const std::string& cs)
{
  parser.write(tok(cs));
}

template<typename...Str>
void write_cs(dex::FunctionCaller& parser, const std::string& cs, const Str& ...strs)
{
  parser.write(tok(cs));
  write_cs(parser, strs...);
}

void TestDexInput::argumentParsing()
{
  dex::ParserMachine machine;
  dex::FunctionCaller parser{ machine };

  write_cs(parser, "p@rseint", "c@ll", "f@@");
  write_chars(parser, "123 ");

  QVERIFY(parser.hasPendingCall());
  QVERIFY(parser.call().function == "f@@");
  QVERIFY(parser.call().arguments.size() == 1);
  QVERIFY(std::holds_alternative<int>(parser.call().arguments.front()));
  QVERIFY(std::get<int>(parser.call().arguments.front()) == 123);

  parser.clearPendingCall();
    
  write_cs(parser, "p@rsebool", "p@rseword", "c@ll", "b@r");
  write_chars(parser, "1 hello ");

  QVERIFY(parser.hasPendingCall());
  QVERIFY(parser.call().function == "b@r");
  QVERIFY(parser.call().arguments.size() == 2);
  QVERIFY(std::holds_alternative<bool>(parser.call().arguments.front()));
  QVERIFY(std::get<bool>(parser.call().arguments.front()));
  QVERIFY(std::holds_alternative<std::string>(parser.call().arguments.back()));
  QVERIFY(std::get<std::string>(parser.call().arguments.back()) == "hello");

  parser.clearPendingCall();

  write_cs(parser, "p@rseword", "p@rseline", "c@ll", "p@r@m");
  write_chars(parser, "there General Kenobi!");
  parser.write(tex::parsing::CharacterToken{ '\n', tex::parsing::CharCategory::Active });

  QVERIFY(parser.hasPendingCall());
  QVERIFY(parser.call().function == "p@r@m");
  QVERIFY(parser.call().arguments.size() == 2);
  QVERIFY(std::holds_alternative<std::string>(parser.call().arguments.front()));
  QVERIFY(std::get<std::string>(parser.call().arguments.front()) == "there");
  QVERIFY(std::holds_alternative<std::string>(parser.call().arguments.back()));
  QVERIFY(std::get<std::string>(parser.call().arguments.back()) == "General Kenobi!");

  parser.clearPendingCall();

  write_cs(parser, "p@rseoptions", "c@ll", "@pts");
  write_chars(parser, "[standalone, key=value]");

  QVERIFY(parser.hasPendingCall());
  QVERIFY(parser.call().function == "@pts");
  QVERIFY(parser.call().arguments.empty());
  QVERIFY(parser.call().options.size() == 2);
  QVERIFY(std::get<std::string>(parser.call().options.at("")) == "standalone");
  QVERIFY(std::get<std::string>(parser.call().options.at("key")) == "value");

  parser.clearPendingCall();

  write_cs(parser, "p@rseword", "c@ll", "im@ge");
  write_chars(parser, "{test-image.jpg}");

  QVERIFY(parser.hasPendingCall());
  QVERIFY(parser.call().function == "im@ge");
  QVERIFY(parser.call().arguments.size() == 1);
  QVERIFY(std::get<std::string>(parser.call().arguments.at(0)) == "test-image.jpg");

  parser.clearPendingCall();

  write_cs(parser, "p@rseline", "c@ll", "foo");
  write_chars_lexer(parser, "{This one extends after\n the end of the line}");

  QVERIFY(parser.hasPendingCall());
  QVERIFY(parser.call().function == "foo");
  QVERIFY(parser.call().arguments.size() == 1);
  QVERIFY(std::get<std::string>(parser.call().arguments.at(0)) == "This one extends after the end of the line");
}

void TestDexInput::conditionalEvaluator()
{
  dex::InputStream istream;
  tex::parsing::Lexer lexer;
  tex::parsing::Preprocessor preproc;
  dex::ConditionalEvaluator parser{ istream, lexer, preproc };

  istream.inject("{[");

  parser.write(tok('a'));
  QVERIFY(parser.output().size() == 1);
  parser.write(tok("c@ll"));
  QVERIFY(parser.output().size() == 2);

  parser.write(tok("testleftbr@ce"));
  QVERIFY(parser.output().size() == 2);
  QVERIFY(preproc.br);

  QVERIFY(istream.readChar() == '{');

  parser.write(tok("testnextch@r"));
  parser.write(tok(']'));
  QVERIFY(parser.output().size() == 2);
  QVERIFY(!preproc.br);

  parser.write(tok("testnextch@r"));
  parser.write(tok('['));
  QVERIFY(parser.output().size() == 2);
  QVERIFY(preproc.br);
}

void TestDexInput::documentWriterParagraph()
{
  dex::DocumentWriter writer;

  //writer.startParagraph();

  writer.c("std::vector");
  writer.write(" is a sequence container that encapsulates dynamic size arrays");
  writer.write('.');
  writer.par();

  writer.beginSinceBlock("C++03");
  writer.write("The elements are stored contiguously, ");
  writer.paragraphWriter().writeLink("#more", "...");
  writer.endSinceBlock();

  QVERIFY(writer.output()->childNodes().size() == 4);

  QVERIFY(writer.output()->childNodes().at(0)->is<dex::Paragraph>());
  QVERIFY(writer.output()->childNodes().at(2)->is<dex::Paragraph>());

  auto par = std::static_pointer_cast<dex::Paragraph>(writer.output()->childNodes().at(0));
  QVERIFY(par->text() == "std::vector is a sequence container that encapsulates dynamic size arrays.");
  QVERIFY(par->metadata().size() == 1);
  QVERIFY(par->metadata().front()->is<dex::TextStyle>());
  QVERIFY(std::static_pointer_cast<dex::TextStyle>(par->metadata().front())->style() == "code");

  par = std::static_pointer_cast<dex::Paragraph>(writer.output()->childNodes().at(2));
  QVERIFY(par->text() == "The elements are stored contiguously, ...");
  QVERIFY(par->metadata().size() == 1);
  QVERIFY(par->metadata().back()->is<dex::Link>());
  QVERIFY(par->metadata().back()->range().text() == "...");
}

void TestDexInput::documentWriterList()
{
  dex::DocumentWriter writer;

  writer.list();
  writer.li({}, {});
  writer.write("List item number 1");
  writer.li({}, {});
  writer.write("Number 2");
  writer.endlist();

  QVERIFY(writer.output()->childNodes().size() == 1);
  QVERIFY(writer.output()->childNodes().at(0)->is<dex::List>());

  auto list = std::static_pointer_cast<dex::List>(writer.output()->childNodes().at(0));

  QVERIFY(list->items.size() == 2);

  QVERIFY(list->items.front()->childNodes().size() == 1);
  QVERIFY(list->items.back()->childNodes().size() == 1);

  QVERIFY(list->items.front()->childNodes().front()->is<dex::Paragraph>());

  auto par = std::static_pointer_cast<dex::Paragraph>(list->items.front()->childNodes().front());
  QVERIFY(par->text() == "List item number 1");

  par = std::static_pointer_cast<dex::Paragraph>(list->items.back()->childNodes().front());
  QVERIFY(par->text() == "Number 2");
}

void TestDexInput::parserMachineImage()
{
  dex::ParserMachine parser;

  QFile file{ "test.cpp" };
  QVERIFY(file.open(QIODevice::WriteOnly));

  file.write(
    "/*!\n"
    " * \\class vector\n"
    " *\n"
    " * This is a first paragraph.\n"
    " * \\image[width=66]{test.png}\n"
    " * This is a second paragraph.\n"
    " */\n"
  );

  file.close();

  parser.process(QFileInfo{ "test.cpp" });

  QFile::remove("test.cpp");

  std::shared_ptr<dex::Namespace> ns = parser.output()->program()->globalNamespace();

  QVERIFY(ns->entities.size() > 0);
  QVERIFY(ns->entities.front()->is<dex::Class>());
  auto vec = std::static_pointer_cast<dex::Class>(ns->entities.front());
  QVERIFY(vec->description->childNodes().size() == 3);

  QVERIFY(vec->description->childNodes().at(0)->is<dex::Paragraph>());
  auto par = std::static_pointer_cast<dex::Paragraph>(vec->description->childNodes().at(0));
  QVERIFY(par->text() == "This is a first paragraph.");

  QVERIFY(vec->description->childNodes().at(1)->is<dex::Image>());
  auto img = std::static_pointer_cast<dex::Image>(vec->description->childNodes().at(1));
  QVERIFY(img->src == "test.png");
  QVERIFY(img->width == 66);

  QVERIFY(vec->description->childNodes().at(2)->is<dex::Paragraph>());
  par = std::static_pointer_cast<dex::Paragraph>(vec->description->childNodes().at(2));
  QVERIFY(par->text() == "This is a second paragraph.");
}

void TestDexInput::parserMachineList()
{
  dex::ParserMachine parser;

  QFile file{ "test.cpp" };
  QVERIFY(file.open(QIODevice::WriteOnly));

  file.write(
    "/*!\n"
    " * \\class vector\n"
    " *\n"
    " * \\list\n"
    " *   \\li first item\n"
    " *   \\li second item:\n"
    " *     \\list\n"
    " *       \\li nested item\n"
    " *     \\endlist\n"
    " * \\endlist\n"
    " */\n"
  );

  file.close();

  parser.process(QFileInfo{ "test.cpp" });

  QFile::remove("test.cpp");

  std::shared_ptr<dex::Namespace> ns = parser.output()->program()->globalNamespace();

  QVERIFY(ns->entities.size() > 0);
  QVERIFY(ns->entities.front()->is<dex::Class>());
  auto vec = std::static_pointer_cast<dex::Class>(ns->entities.front());
  QVERIFY(vec->description->childNodes().size() == 1);
  QVERIFY(vec->description->childNodes().front()->is<dex::List>());

  auto lst = std::static_pointer_cast<dex::List>(vec->description->childNodes().front());
  
  QVERIFY(lst->items.size() == 2);
  QVERIFY(lst->items.back()->childNodes().size() == 2);
  QVERIFY(lst->items.back()->childNodes().back()->is<dex::List>());

  lst = std::static_pointer_cast<dex::List>(lst->items.back()->childNodes().back());
  QVERIFY(lst->items.size() == 1);
  QVERIFY(lst->items.front()->childNodes().size() == 1);
  QVERIFY(lst->items.front()->childNodes().front()->is<dex::Paragraph>());

  auto par = std::static_pointer_cast<dex::Paragraph>(lst->items.front()->childNodes().front());
  QVERIFY(par->text() == "nested item");
}

void TestDexInput::parserMachineClass()
{
  dex::ParserMachine parser;

  QFile file{ "test.cpp" };
  QVERIFY(file.open(QIODevice::WriteOnly));

  file.write(
    "// The following block is recognized by dex\n"
    "/*!\n"
    " * \\class vector\n"
    " * \\brief sequence container that encapsulates dynamic size arrays\n"
    " *\n"
    " * The elements are stored contiguously, ...\n"
    " */\n"
  );

  file.close();

  parser.process(QFileInfo{ "test.cpp" });

  std::shared_ptr<dex::Namespace> ns = parser.output()->program()->globalNamespace();

  QVERIFY(ns->entities.size() > 0);
  QVERIFY(ns->entities.front()->is<dex::Class>());
  auto vec = std::static_pointer_cast<dex::Class>(ns->entities.front());
  QVERIFY(vec->name == "vector");
  QVERIFY(vec->brief.value() == "sequence container that encapsulates dynamic size arrays");
  QVERIFY(vec->description->childNodes().size() == 1);
  QVERIFY(vec->description->childNodes().front()->is<dex::Paragraph>());
  auto paragraph = std::static_pointer_cast<dex::Paragraph>(vec->description->childNodes().front());
  QVERIFY(paragraph->text() == "The elements are stored contiguously, ...");

  QFile::remove("test.cpp");
}

void TestDexInput::parserMachineFunction()
{
  dex::ParserMachine parser;

  QFile file{ "test.cpp" };
  QVERIFY(file.open(QIODevice::WriteOnly));

  file.write(
    "// The following block is recognized by dex\n"
    "/*!\n"
    " * \\fn char* getenv(const char* env_var);\n"
    " * \\brief get value from environment variables\n"
    " * \\param name of the environment variable\n"
    " * \\returns value of environment variable\n"
    " * \\since C++98\n"
    " *\n"
    " * Searches the environment list provided by the host environment...\n"
    " *\n"
    " * Modifying the string returned by getenv invokes undefined behavior.\n"
    " */\n"
  );

  file.close();

  parser.process(QFileInfo{ "test.cpp" });

  std::shared_ptr<dex::Namespace> ns = parser.output()->program()->globalNamespace();

  QVERIFY(ns->entities.size() > 0);
  QVERIFY(ns->entities.front()->is<dex::Function>());
  auto getenv = std::static_pointer_cast<dex::Function>(ns->entities.front());
  QVERIFY(getenv->name == "getenv");
  QVERIFY(getenv->brief.value() == "get value from environment variables");
  QVERIFY(getenv->since.value().version() == "C++98");
  QVERIFY(getenv->parameters.size() == 1);
  auto funparam = getenv->parameters.front();
  QVERIFY(funparam->brief == "name of the environment variable");
  QVERIFY(getenv->return_type.brief.value_or("") == "value of environment variable");
  QVERIFY(getenv->description->childNodes().size() == 2);
  QVERIFY(getenv->description->childNodes().front()->is<dex::Paragraph>());
  QVERIFY(getenv->description->childNodes().back()->is<dex::Paragraph>());
  auto paragraph = std::static_pointer_cast<dex::Paragraph>(getenv->description->childNodes().front());
  QVERIFY(paragraph->text() == "Searches the environment list provided by the host environment...");
  paragraph = std::static_pointer_cast<dex::Paragraph>(getenv->description->childNodes().back());
  QVERIFY(paragraph->text() == "Modifying the string returned by getenv invokes undefined behavior.");

  QFile::remove("test.cpp");
}

void TestDexInput::parserMachineEnum()
{
  dex::ParserMachine parser;

  QFile file{ "test.cpp" };
  QVERIFY(file.open(QIODevice::WriteOnly));

  file.write(
    "/*!\n"
    " * \\enum Corner\n"
    " * \\brief describes a corner\n"
    " *\n"
    " * This is not that useful.\n"
    " *\n"
    " * \\value TopLeft the top left corner\n"
    " * \\value TopRight the top right corner\n"
    " * \\value BottomLeft the bottom left corner\n"
    " * \\value BottomRight the bottom right corner\n"
    " */\n"
  );

  file.close();

  parser.process(QFileInfo{ "test.cpp" });

  QFile::remove("test.cpp");

  std::shared_ptr<dex::Namespace> ns = parser.output()->program()->globalNamespace();

  QVERIFY(ns->entities.size() > 0);
  QVERIFY(ns->entities.front()->is<dex::Enum>());
  auto corner = std::static_pointer_cast<dex::Enum>(ns->entities.front());
  QVERIFY(corner->name == "Corner");
  QVERIFY(corner->values.size() == 4);
  QVERIFY(corner->brief.value() == "describes a corner");
  QVERIFY(corner->description->childNodes().size() == 1);
  QVERIFY(corner->description->childNodes().front()->is<dex::Paragraph>());
  auto paragraph = std::static_pointer_cast<dex::Paragraph>(corner->description->childNodes().front());
  QVERIFY(paragraph->text() == "This is not that useful.");

  auto top_left = corner->values.at(0);
  QVERIFY(top_left->name == "TopLeft");
  QVERIFY(top_left->description->childNodes().size() == 1);
  QVERIFY(top_left->description->childNodes().front()->is<dex::Paragraph>());
  paragraph = std::static_pointer_cast<dex::Paragraph>(top_left->description->childNodes().front());
  QVERIFY(paragraph->text() == "the top left corner");

  auto bottom_right = corner->values.at(3);
  QVERIFY(bottom_right->name == "BottomRight");
  QVERIFY(bottom_right->description->childNodes().size() == 1);
  QVERIFY(bottom_right->description->childNodes().front()->is<dex::Paragraph>());
  paragraph = std::static_pointer_cast<dex::Paragraph>(bottom_right->description->childNodes().front());
  QVERIFY(paragraph->text() == "the bottom right corner");
}

void TestDexInput::parserMachineVariable()
{
  dex::ParserMachine parser;

  QFile file{ "test.cpp" };
  QVERIFY(file.open(QIODevice::WriteOnly));

  file.write(
    "/*!\n"
    " * \\variable std::string name = \"dex\";\n"
    " * \\brief the name of the program\n"
    " * \\since 2020\n"
    " * Stores the name of the program.\n"
    " */\n"
  );

  file.close();

  parser.process(QFileInfo{ "test.cpp" });

  std::shared_ptr<dex::Namespace> ns = parser.output()->program()->globalNamespace();

  QVERIFY(ns->entities.size() > 0);
  QVERIFY(ns->entities.front()->is<dex::Variable>());
  auto variable = std::static_pointer_cast<dex::Variable>(ns->entities.front());
  QVERIFY(variable->name == "name");
  QVERIFY(variable->brief.value() == "the name of the program");
  QVERIFY(variable->since.value().version() == "2020");
 
  QVERIFY(variable->description->childNodes().size() == 1);
  QVERIFY(variable->description->childNodes().front()->is<dex::Paragraph>());
  auto paragraph = std::static_pointer_cast<dex::Paragraph>(variable->description->childNodes().front());
  QVERIFY(paragraph->text() == "Stores the name of the program.");

  QFile::remove("test.cpp");
}

void TestDexInput::parserMachineManual()
{
  {
    QFile file{ "test.dex" };
    QVERIFY(file.open(QIODevice::WriteOnly));

    file.write(
      "\n"
      "\\manual Manual's title\n"
      "\n"
      "\\part First part\n"
      "\n"
      "\\input{toast}"
      "\n"
      "\\chapter{Second chapter}\n"
      "This is the content of the second chapter.\n"
      "\n"
    );

    file.close();
  }

  {
    QFile file{ "toast.dex" };
    QVERIFY(file.open(QIODevice::WriteOnly));

    file.write(
      "\\chapter First chapter\n"
      "This is the content of the first chapter.\n"
    );

    file.close();
  }

  dex::ParserMachine parser;
  parser.process(QFileInfo{ "test.dex" });

  QFile::remove("test.dex");
  QFile::remove("toast.dex");

  QVERIFY(parser.output()->documents.size() == 1);

  std::shared_ptr<dex::Document> man = parser.output()->documents.front();

  QVERIFY(man->childNodes().size() == 1);
  QVERIFY(man->childNodes().front()->is<dex::Sectioning>());

  auto part = std::dynamic_pointer_cast<dex::Sectioning>(man->childNodes().front());
  QVERIFY(part->depth == dex::Sectioning::Part);
  QVERIFY(part->content.size() == 2);

  auto first_chapter = std::dynamic_pointer_cast<dex::Sectioning>(part->content.front());
  QVERIFY(first_chapter != nullptr && first_chapter->depth == dex::Sectioning::Chapter);

  {
    QVERIFY(first_chapter->name == "First chapter");
    QVERIFY(first_chapter->content.size() == 1);

    auto par = std::dynamic_pointer_cast<dex::Paragraph>(first_chapter->content.front());
    QVERIFY(par != nullptr && par->text() == "This is the content of the first chapter.");
  }

  auto second_chapter = std::dynamic_pointer_cast<dex::Sectioning>(part->content.back());
  QVERIFY(second_chapter != nullptr && second_chapter->depth == dex::Sectioning::Chapter);

  {
    QVERIFY(second_chapter->name == "Second chapter");
    QVERIFY(second_chapter->content.size() == 1);

    auto par = std::dynamic_pointer_cast<dex::Paragraph>(second_chapter->content.front());
    QVERIFY(par != nullptr && par->text() == "This is the content of the second chapter.");
  }
}
