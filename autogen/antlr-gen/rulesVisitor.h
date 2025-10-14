
// Generated from rules.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "rulesParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by rulesParser.
 */
class  rulesVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by rulesParser.
   */
    virtual std::any visitProgram(rulesParser::ProgramContext *context) = 0;

    virtual std::any visitVersion(rulesParser::VersionContext *context) = 0;

    virtual std::any visitStatement(rulesParser::StatementContext *context) = 0;

    virtual std::any visitAndExpr(rulesParser::AndExprContext *context) = 0;

    virtual std::any visitCompareExpr(rulesParser::CompareExprContext *context) = 0;

    virtual std::any visitNotExpr(rulesParser::NotExprContext *context) = 0;

    virtual std::any visitParenExpr(rulesParser::ParenExprContext *context) = 0;

    virtual std::any visitRangeExpr(rulesParser::RangeExprContext *context) = 0;

    virtual std::any visitOrExpr(rulesParser::OrExprContext *context) = 0;

    virtual std::any visitCompOp(rulesParser::CompOpContext *context) = 0;

    virtual std::any visitValue(rulesParser::ValueContext *context) = 0;

    virtual std::any visitNumber(rulesParser::NumberContext *context) = 0;


};

