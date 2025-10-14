
// Generated from rules.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "rulesVisitor.h"


/**
 * This class provides an empty implementation of rulesVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  rulesBaseVisitor : public rulesVisitor {
public:

  virtual std::any visitProgram(rulesParser::ProgramContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVersion(rulesParser::VersionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatement(rulesParser::StatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAndExpr(rulesParser::AndExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCompareExpr(rulesParser::CompareExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNotExpr(rulesParser::NotExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParenExpr(rulesParser::ParenExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRangeExpr(rulesParser::RangeExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOrExpr(rulesParser::OrExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCompOp(rulesParser::CompOpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitValue(rulesParser::ValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumber(rulesParser::NumberContext *ctx) override {
    return visitChildren(ctx);
  }


};

