#include "policy_protovisitor.h"

using namespace antlrcpp;

std::any RulesToProtoVisitor::visitStatement(
    rulesParser::StatementContext* ctx) {
  dsl::Rule rule;

  auto severityToken = ctx->children[0]->getText();
  rule.set_severity(getSeverity(severityToken));

  std::string label = ctx->STRING()->getText();
  rule.set_label(label.substr(1, label.size() - 2));  // remove quotes

  dsl::Expr expr = visitExprTree(ctx->expr());
  *rule.mutable_condition() = std::move(expr);

  return rule;
}

dsl::SeverityType RulesToProtoVisitor::getSeverity(
    const std::string& tokenText) {
  if (tokenText == "warn") return dsl::SeverityType::WARN;
  if (tokenText == "error") return dsl::SeverityType::ERROR;
  throw std::runtime_error("Unknown severity token: " + tokenText);
}

dsl::Expr RulesToProtoVisitor::visitExprTree(rulesParser::ExprContext* ctx) {
  return std::any_cast<dsl::Expr>(visit(ctx));
}

std::any RulesToProtoVisitor::visitAndExpr(rulesParser::AndExprContext* ctx) {
  dsl::Logical* logic = new dsl::Logical();
  logic->set_op(dsl::AND);

  for (auto* subCtx : ctx->expr()) {
    *logic->add_operands() = visitExprTree(subCtx);
  }

  dsl::Expr e;
  e.set_allocated_logical(logic);
  return e;
}

std::any RulesToProtoVisitor::visitOrExpr(rulesParser::OrExprContext* ctx) {
  dsl::Logical* logic = new dsl::Logical();
  logic->set_op(dsl::OR);

  for (auto* subCtx : ctx->expr()) {
    *logic->add_operands() = visitExprTree(subCtx);
  }

  dsl::Expr e;
  e.set_allocated_logical(logic);
  return e;
}

std::any RulesToProtoVisitor::visitNotExpr(rulesParser::NotExprContext* ctx) {
  dsl::NotExpr* not_expr = new dsl::NotExpr();
  *not_expr->mutable_expr() = visitExprTree(ctx->expr());

  dsl::Expr e;
  e.set_allocated_not_expr(not_expr);
  return e;
}

std::any RulesToProtoVisitor::visitParenExpr(
    rulesParser::ParenExprContext* ctx) {
  return visitExprTree(ctx->expr());
}

std::any RulesToProtoVisitor::visitCompareExpr(
    rulesParser::CompareExprContext* ctx) {
  dsl::Comparison* cmp = new dsl::Comparison();
  cmp->set_column(ctx->IDENT()->getText());

  std::string op = ctx->compOp()->getText();
  if (op == "==")
    cmp->set_op(dsl::EQ);
  else if (op == "!=")
    cmp->set_op(dsl::NE);
  else if (op == ">")
    cmp->set_op(dsl::GT);
  else if (op == ">=")
    cmp->set_op(dsl::GE);
  else if (op == "<")
    cmp->set_op(dsl::LT);
  else if (op == "<=")
    cmp->set_op(dsl::LE);
  else
    throw std::runtime_error("Unknown comparison operator: " + op);

  std::string raw = ctx->value()->getText();
  if (!raw.empty() && raw.front() == '"' && raw.back() == '"') {
    // String literal - remove the surrounding quotes
    cmp->set_value(raw.substr(1, raw.size() - 2));
  } else {
    // Number literal - use as-is
    cmp->set_value(raw);
  }

  dsl::Expr e;
  e.set_allocated_comparison(cmp);
  return e;
}

std::any RulesToProtoVisitor::visitRangeExpr(
    rulesParser::RangeExprContext* ctx) {
  dsl::RangeCheck* range = new dsl::RangeCheck();
  range->set_column(ctx->IDENT()->getText());

  double low = std::stod(ctx->number(0)->getText());
  double high = std::stod(ctx->number(1)->getText());
  range->set_low(low);
  range->set_high(high);

  dsl::Expr e;
  e.set_allocated_range(range);
  return e;
}
