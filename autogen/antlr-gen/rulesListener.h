
// Generated from /Users/panayotovmarko/Work/liblcvm/policy/rules.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "rulesParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by rulesParser.
 */
class  rulesListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterProgram(rulesParser::ProgramContext *ctx) = 0;
  virtual void exitProgram(rulesParser::ProgramContext *ctx) = 0;

  virtual void enterStatement(rulesParser::StatementContext *ctx) = 0;
  virtual void exitStatement(rulesParser::StatementContext *ctx) = 0;

  virtual void enterAndExpr(rulesParser::AndExprContext *ctx) = 0;
  virtual void exitAndExpr(rulesParser::AndExprContext *ctx) = 0;

  virtual void enterCompareExpr(rulesParser::CompareExprContext *ctx) = 0;
  virtual void exitCompareExpr(rulesParser::CompareExprContext *ctx) = 0;

  virtual void enterNotExpr(rulesParser::NotExprContext *ctx) = 0;
  virtual void exitNotExpr(rulesParser::NotExprContext *ctx) = 0;

  virtual void enterParenExpr(rulesParser::ParenExprContext *ctx) = 0;
  virtual void exitParenExpr(rulesParser::ParenExprContext *ctx) = 0;

  virtual void enterRangeExpr(rulesParser::RangeExprContext *ctx) = 0;
  virtual void exitRangeExpr(rulesParser::RangeExprContext *ctx) = 0;

  virtual void enterOrExpr(rulesParser::OrExprContext *ctx) = 0;
  virtual void exitOrExpr(rulesParser::OrExprContext *ctx) = 0;

  virtual void enterCompOp(rulesParser::CompOpContext *ctx) = 0;
  virtual void exitCompOp(rulesParser::CompOpContext *ctx) = 0;

  virtual void enterValue(rulesParser::ValueContext *ctx) = 0;
  virtual void exitValue(rulesParser::ValueContext *ctx) = 0;

  virtual void enterNumber(rulesParser::NumberContext *ctx) = 0;
  virtual void exitNumber(rulesParser::NumberContext *ctx) = 0;


};

