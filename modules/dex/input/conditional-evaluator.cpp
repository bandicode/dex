// Copyright (C) 2019 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/input/conditional-evaluator.h"

#include "dex/input/parser-machine.h"
#include "dex/input/parser-errors.h"

#include <tex/lexer.h>

#include <cassert>
#include <stdexcept>

namespace dex
{

ConditionalEvaluator::ConditionalEvaluator(ParserMachine& machine)
  : ConditionalEvaluator{machine.inputStream(), machine.lexer(), machine.preprocessor()}
{

}

ConditionalEvaluator::ConditionalEvaluator(InputStream& is, tex::parsing::Lexer& lex, tex::parsing::Preprocessor& preproc)
  : m_inputstream { is },
    m_lexer{lex},
    m_preprocessor{preproc},
    m_state{ State::Idle }
{

}

InputStream& ConditionalEvaluator::inputStream()
{
  return m_inputstream;
}

tex::parsing::Lexer& ConditionalEvaluator::lexer()
{
  return m_lexer;
}

ConditionalEvaluator::State& ConditionalEvaluator::state()
{
  return m_state;
}

void ConditionalEvaluator::write(tex::parsing::Token&& tok)
{
  switch (state())
  {
  case State::Idle:
  {
    if (tok.isControlSequence())
    {
      if (tok.controlSequence() == "testleftbr@ce")
      {
        m_preprocessor.br = inputStream().peekChar() == '{';
      }
      else if (tok.controlSequence() == "testnextch@r")
      {
        m_state = State::WaitingTestNextChar;
      }
      else
      {
        tex::parsing::write(std::move(tok), m_output);
      }
    }
    else
    {
      tex::parsing::write(std::move(tok), m_output);
    }
  }
  break;
  case State::WaitingTestNextChar:
  {
    if (tok.isControlSequence())
      throw UnexpectedControlSequence{ tok.controlSequence() };

    if(lexer().output().empty())
      m_preprocessor.br = inputStream().peekChar() == tok.characterToken().value;
    else
      m_preprocessor.br = lexer().output().front().isCharacterToken() 
        && lexer().output().front().characterToken().value == tok.characterToken().value;

    m_state = State::Idle;
  }
  break;
  default:
    break;
  }
}

} // namespace dex
