#pragma once

#include "rules.pb.h"
#include "rulesBaseVisitor.h"

class RulesToProtoVisitor : public rulesBaseVisitor {
 public:
  RulesToProtoVisitor() = default;

  std::any visitStatement(rulesParser::StatementContext* ctx) override;

  std::any visitAndExpr(rulesParser::AndExprContext* ctx) override;
  std::any visitOrExpr(rulesParser::OrExprContext* ctx) override;
  std::any visitNotExpr(rulesParser::NotExprContext* ctx) override;
  std::any visitParenExpr(rulesParser::ParenExprContext* ctx) override;
  std::any visitCompareExpr(rulesParser::CompareExprContext* ctx) override;
  std::any visitRangeExpr(rulesParser::RangeExprContext* ctx) override;

  // Helpers
  dsl::Expr visitExprTree(rulesParser::ExprContext* ctx);
  dsl::SeverityType getSeverity(const std::string& tokenText);
};
