#include <google/protobuf/text_format.h>

#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <variant>

#include "antlr4-runtime.h"
#include "liblcvm.h"
#include "policy_protovisitor.h"
#include "rules.pb.h"
#include "rulesLexer.h"
#include "rulesParser.h"
#include "tree/Trees.h"

struct ParserContext {
  std::unique_ptr<std::ifstream> stream;
  std::unique_ptr<antlr4::ANTLRInputStream> input;
  std::unique_ptr<rulesLexer> lexer;
  std::unique_ptr<antlr4::CommonTokenStream> tokens;
  std::unique_ptr<rulesParser> parser;
  // raw pointer, not unique_ptr, as we do not own this memory
  rulesParser::ProgramContext* tree = nullptr;
};

ParserContext create_parser_content(const std::string& infile) {
  ParserContext ctx;

  // Hold the stream open so it stays valid while ANTLRInputStream reads from it
  ctx.stream = std::make_unique<std::ifstream>(infile);
  if (!ctx.stream->is_open()) {
    throw std::runtime_error("Failed to open input file: " + infile);
  }

  // parse the file
  ctx.input = std::make_unique<antlr4::ANTLRInputStream>(*ctx.stream);
  ctx.lexer = std::make_unique<rulesLexer>(ctx.input.get());
  ctx.tokens = std::make_unique<antlr4::CommonTokenStream>(ctx.lexer.get());
  ctx.parser = std::make_unique<rulesParser>(ctx.tokens.get());
  ctx.tree = ctx.parser->program();

  // check the contents
  if (!ctx.tree || ctx.tree->children.empty()) {
    std::cerr << "Parse error: Empty or invalid input" << infile << "\n";
    throw std::runtime_error("Parse error: Empty or invalid input");
  }
  return ctx;
}

void printTree(antlr4::tree::ParseTree* tree, rulesParser* parser,
               std::ofstream& outfile_stream, int indent = 0) {
  if (!tree) return;

  std::string ind(indent * 2, ' ');
  std::string text = antlr4::tree::Trees::getNodeText(tree, parser);
  outfile_stream << ind << text << "\n";

  // Only try to access children if this is a rule node
  auto ruleCtx = dynamic_cast<antlr4::ParserRuleContext*>(tree);
  if (ruleCtx && !ruleCtx->children.empty()) {
    for (auto* child : ruleCtx->children) {
      printTree(child, parser, outfile_stream, indent + 1);
    }
  }
}

void printContent(ParserContext& ctx, const std::string& outfile) {
  // print the contents

  // std::cout << ctx.tree->toStringTree(ctx.parser) << std::endl;
  std::ofstream outfile_stream(outfile);
  if (!outfile_stream) {
    std::cerr << "Error: Could not open " << outfile << " for writing.\n";
    throw std::runtime_error("Failed to open output file: " + outfile);
  }
  printTree(ctx.tree, ctx.parser.get(), outfile_stream);
}

dsl::RuleSet convert_parser_context_to_proto(ParserContext& ctx) {
  RulesToProtoVisitor visitor;
  dsl::RuleSet ruleSet;

  for (auto stmt : ctx.tree->statement()) {
    auto rule = std::any_cast<dsl::Rule>(visitor.visitStatement(stmt));
    ruleSet.add_rules()->CopyFrom(rule);
  }

  return ruleSet;
}

void writeProtoToFile(dsl::RuleSet ruleSet, std::string outfile) {
  // Serialize to text proto
  google::protobuf::TextFormat::Printer printer;
  std::string textProto;
  printer.PrintToString(ruleSet, &textProto);

  // write to file
  std::ofstream outfile_stream(outfile);
  if (!outfile_stream) {
    std::cerr << "Error: Could not open " << outfile << " for writing.\n";
    throw std::runtime_error("Failed to open output file: " + outfile);
  }
  outfile_stream << textProto;
  outfile_stream.close();
}

// recursive evaluator
bool evaluate_rule(const dsl::RuleSet& rules,
                   const std::map<std::string, LiblcvmValue>& dict);

bool eval_expr(const dsl::Expr& expr,
               const std::map<std::string, LiblcvmValue>& dict);

bool eval_comparison(const dsl::Comparison& cmp,
                     const std::map<std::string, LiblcvmValue>& dict) {
  auto it = dict.find(cmp.column());
  if (it == dict.end()) return false;

  const LiblcvmValue& val = it->second;

  if (std::holds_alternative<std::string>(val)) {
    // string comparisons
    std::string rhs = cmp.value();
    std::string lhs = liblcvmvalue_to_string(val);
    switch (cmp.op()) {
      case dsl::ComparisonOpType::EQ:
        return lhs == rhs;
      case dsl::ComparisonOpType::NE:
        return lhs != rhs;
      default:
        throw std::runtime_error("Unsupported comparison op for strings: " +
                                 std::to_string(static_cast<int>(cmp.op())));
    }
  }

  // numeric comparisons
  double lhs = liblcvmvalue_to_double(val);
  double rhs = std::stod(cmp.value());
  switch (cmp.op()) {
    case dsl::ComparisonOpType::EQ:
      return lhs == rhs;
    case dsl::ComparisonOpType::NE:
      return lhs != rhs;
    case dsl::ComparisonOpType::LT:
      return lhs < rhs;
    case dsl::ComparisonOpType::LE:
      return lhs <= rhs;
    case dsl::ComparisonOpType::GT:
      return lhs > rhs;
    case dsl::ComparisonOpType::GE:
      return lhs >= rhs;
    default:
      throw std::runtime_error("Unsupported comparison op for numeric: " +
                               std::to_string(static_cast<int>(cmp.op())));
  }
}

bool eval_range(const dsl::RangeCheck& range,
                const std::map<std::string, LiblcvmValue>& dict) {
  auto it = dict.find(range.column());
  if (it == dict.end()) return false;

  double val = liblcvmvalue_to_double(it->second);
  double low = range.low();
  double high = range.high();

  return val >= low && val <= high;
}

bool eval_not(const dsl::NotExpr& not_expr,
              const std::map<std::string, LiblcvmValue>& dict) {
  return !eval_expr(not_expr.expr(), dict);
}

bool eval_logical(const dsl::Logical& logic,
                  const std::map<std::string, LiblcvmValue>& dict) {
  switch (logic.op()) {
    case dsl::LogicOpType::AND:
      for (const auto& e : logic.operands())
        if (!eval_expr(e, dict)) return false;
      return true;
    case dsl::LogicOpType::OR:
      for (const auto& e : logic.operands())
        if (eval_expr(e, dict)) return true;
      return false;
    default:
      return false;
  }
}

bool eval_expr(const dsl::Expr& expr,
               const std::map<std::string, LiblcvmValue>& dict) {
  switch (expr.expr_kind_case()) {
    case dsl::Expr::kComparison:
      return eval_comparison(expr.comparison(), dict);
    case dsl::Expr::kRange:
      return eval_range(expr.range(), dict);
    case dsl::Expr::kNotExpr:
      return eval_not(expr.not_expr(), dict);
    case dsl::Expr::kLogical:
      return eval_logical(expr.logical(), dict);
    default:
      return false;
  }
}

void evaluate_rules(const dsl::RuleSet& rules,
                    const std::map<std::string, LiblcvmValue>& dict,
                    std::list<std::string>* warn_list,
                    std::list<std::string>* error_list) {
  for (const auto& rule : rules.rules()) {
    bool result = eval_expr(rule.condition(), dict);
    if (result) {
      if (rule.severity() == dsl::SeverityType::WARN && warn_list) {
        warn_list->push_back(rule.label());
      } else if (rule.severity() == dsl::SeverityType::ERROR && error_list) {
        error_list->push_back(rule.label());
      }
    }
  }
}

ParserContext create_parser_content_from_string(const std::string& policy_str) {
  ParserContext ctx;
  ctx.input = std::make_unique<antlr4::ANTLRInputStream>(policy_str);
  ctx.lexer = std::make_unique<rulesLexer>(ctx.input.get());
  ctx.tokens = std::make_unique<antlr4::CommonTokenStream>(ctx.lexer.get());
  ctx.parser = std::make_unique<rulesParser>(ctx.tokens.get());
  ctx.tree = ctx.parser->program();
  if (!ctx.tree || ctx.tree->children.empty()) {
    throw std::runtime_error("Parse error: Empty or invalid input");
  }
  return ctx;
}

int policy_runner(const std::string& policy_str, LiblcvmKeyList* pkeys,
                  LiblcvmValList* pvals, std::list<std::string>* warn_list,
                  std::list<std::string>* error_list) {
  // convert the keys/vals into a dictionary
  std::map<std::string, LiblcvmValue> dict;
  auto itK = pkeys->begin();
  auto itV = pvals->begin();
  for (; itK != pkeys->end() && itV != pvals->end(); ++itK, ++itV) {
    // overwrites if the same key appears twice
    dict[*itK] = *itV;
  }

  // reset warn_list/error_list
  warn_list->clear();
  error_list->clear();

  // run the policy
  try {
    ParserContext ctx = create_parser_content_from_string(policy_str);
    dsl::RuleSet ruleSet = convert_parser_context_to_proto(ctx);
    evaluate_rules(ruleSet, dict, warn_list, error_list);
  } catch (const std::exception& ex) {
    std::cerr << "Fatal error: " << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
