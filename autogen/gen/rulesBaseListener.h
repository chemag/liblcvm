
// Generated from rules.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "rulesListener.h"


/**
 * This class provides an empty implementation of rulesListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  rulesBaseListener : public rulesListener {
public:

  virtual void enterProgram(rulesParser::ProgramContext * /*ctx*/) override { }
  virtual void exitProgram(rulesParser::ProgramContext * /*ctx*/) override { }

  virtual void enterStatement(rulesParser::StatementContext * /*ctx*/) override { }
  virtual void exitStatement(rulesParser::StatementContext * /*ctx*/) override { }

  virtual void enterAndExpr(rulesParser::AndExprContext * /*ctx*/) override { }
  virtual void exitAndExpr(rulesParser::AndExprContext * /*ctx*/) override { }

  virtual void enterCompareExpr(rulesParser::CompareExprContext * /*ctx*/) override { }
  virtual void exitCompareExpr(rulesParser::CompareExprContext * /*ctx*/) override { }

  virtual void enterNotExpr(rulesParser::NotExprContext * /*ctx*/) override { }
  virtual void exitNotExpr(rulesParser::NotExprContext * /*ctx*/) override { }

  virtual void enterParenExpr(rulesParser::ParenExprContext * /*ctx*/) override { }
  virtual void exitParenExpr(rulesParser::ParenExprContext * /*ctx*/) override { }

  virtual void enterRangeExpr(rulesParser::RangeExprContext * /*ctx*/) override { }
  virtual void exitRangeExpr(rulesParser::RangeExprContext * /*ctx*/) override { }

  virtual void enterOrExpr(rulesParser::OrExprContext * /*ctx*/) override { }
  virtual void exitOrExpr(rulesParser::OrExprContext * /*ctx*/) override { }

  virtual void enterCompOp(rulesParser::CompOpContext * /*ctx*/) override { }
  virtual void exitCompOp(rulesParser::CompOpContext * /*ctx*/) override { }

  virtual void enterValue(rulesParser::ValueContext * /*ctx*/) override { }
  virtual void exitValue(rulesParser::ValueContext * /*ctx*/) override { }

  virtual void enterNumber(rulesParser::NumberContext * /*ctx*/) override { }
  virtual void exitNumber(rulesParser::NumberContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

