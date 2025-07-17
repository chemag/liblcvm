#include <google/protobuf/text_format.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <variant>

#include "antlr.protovisitor.h"
#include "antlr4-runtime.h"
#include "gen/rules.pb.h"
#include "gen/rulesLexer.h"
#include "gen/rulesParser.h"
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

ParserContext createParserContext(const std::string& infile) {
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

dsl::RuleSet convertToProto(ParserContext& ctx) {
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


// variant operation
using Value = std::variant<int, double, std::string>;

template<typename T>
double to_number(const T& value) {
    if constexpr (std::is_same_v<T, int>) return static_cast<double>(value);
    if constexpr (std::is_same_v<T, double>) return value;
    throw std::runtime_error("Unsupported numeric type");
}

double to_double(const Value& value) {
   if (std::holds_alternative<int>(value)) {
        return static_cast<double>(std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    } else {
        throw std::runtime_error("Value is not numeric");
    }
}

std::string to_string_value(const Value& value) {
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    } else {
        throw std::runtime_error("Value is not a string");
    }
}


// recursive evaluator
bool evaluate_rule(const dsl::RuleSet& rules, const std::map<std::string, Value>& dict);

bool eval_expr(const dsl::Expr& expr, const std::map<std::string, Value>& dict);

bool eval_comparison(const dsl::Comparison& cmp, const std::map<std::string, Value>& dict) {
    auto it = dict.find(cmp.column());
    if (it == dict.end()) return false;

    const Value& val = it->second;

    if (cmp.op() == dsl::ComparisonOpType::EQ || cmp.op() == dsl::ComparisonOpType::NE) {
        if (std::holds_alternative<std::string>(val)) {
            std::string rhs = cmp.value();
            std::string lhs = to_string_value(val);
            if (cmp.op() == dsl::ComparisonOpType::EQ) return lhs == rhs;
            if (cmp.op() == dsl::ComparisonOpType::NE) return lhs != rhs;
        } else {
            double lhs = to_double(val);
            double rhs = std::stod(cmp.value());
            if (cmp.op() == dsl::ComparisonOpType::EQ) return lhs == rhs;
            if (cmp.op() == dsl::ComparisonOpType::NE) return lhs != rhs;
        }
    }

    // Numeric comparisons
    double lhs = to_double(val);
    double rhs = std::stod(cmp.value());

    switch (cmp.op()) {
        case dsl::ComparisonOpType::LT:  return lhs < rhs;
        case dsl::ComparisonOpType::LE:  return lhs <= rhs;
        case dsl::ComparisonOpType::GT:  return lhs > rhs;
        case dsl::ComparisonOpType::GE:  return lhs >= rhs;
        default:
            throw std::runtime_error("Unsupported comparison op");
    }
}

bool eval_range(const dsl::RangeCheck& range, const std::map<std::string, Value>& dict) {
    auto it = dict.find(range.column());
    if (it == dict.end()) return false;

    double val = to_double(it->second);
    double low = range.low();
    double high = range.high();

    return val >= low && val <= high;
}

bool eval_not(const dsl::NotExpr& not_expr, const std::map<std::string, Value>& dict) {
    return !eval_expr(not_expr.expr(), dict);
}

bool eval_logical(const dsl::Logical& logic, const std::map<std::string, Value>& dict) {
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

bool eval_expr(const dsl::Expr& expr, const std::map<std::string, Value>& dict) {
    switch (expr.expr_kind_case()) {
        case dsl::Expr::kComparison: return eval_comparison(expr.comparison(), dict);
        case dsl::Expr::kRange:   return eval_range(expr.range(), dict);
        case dsl::Expr::kNotExpr:     return eval_not(expr.not_expr(), dict);
        case dsl::Expr::kLogical: return eval_logical(expr.logical(), dict);
        default: return false;
    }
}


void evaluate_rules(const dsl::RuleSet& rules, const std::map<std::string, Value>& dict) {
    for (const auto& rule : rules.rules()) {
        bool result = eval_expr(rule.condition(), dict);
        std::cout << rule.label() << ": " << (result ? "true" : "false") << std::endl;
    }
}

// main file
int main() {
  auto infile = "input.txt";
  auto outfile_text = "output.txt";
  auto outfile = "output.pbtxt";

  try {
    // parse the file
    auto ctx = createParserContext(infile);
    try {
      // print as text complains
      printContent(ctx, outfile_text);
    } catch (const std::exception& ex) {
      // ignore it
    }
    // convert to protobuf
    dsl::RuleSet ruleSet = convertToProto(ctx);
    // print protobuf to text file
    writeProtoToFile(ruleSet, outfile);

    // evaluate a dictionary against the config
    std::map<std::string, Value> dict = {
        {"age", 30},
        {"name", std::string("Bob")},
        {"score", 15.5}
    };
    evaluate_rules(ruleSet, dict);

  } catch (const std::exception& ex) {
    std::cerr << "Fatal error: " << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
