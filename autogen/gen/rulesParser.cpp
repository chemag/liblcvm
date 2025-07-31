
// Generated from rules.g4 by ANTLR 4.13.1


#include "rulesListener.h"
#include "rulesVisitor.h"

#include "rulesParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct RulesParserStaticData final {
  RulesParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  RulesParserStaticData(const RulesParserStaticData&) = delete;
  RulesParserStaticData(RulesParserStaticData&&) = delete;
  RulesParserStaticData& operator=(const RulesParserStaticData&) = delete;
  RulesParserStaticData& operator=(RulesParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag rulesParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
RulesParserStaticData *rulesParserStaticData = nullptr;

void rulesParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (rulesParserStaticData != nullptr) {
    return;
  }
#else
  assert(rulesParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<RulesParserStaticData>(
    std::vector<std::string>{
      "program", "statement", "expr", "compOp", "value", "number"
    },
    std::vector<std::string>{
      "", "'or'", "'and'", "'not'", "'in'", "'range'", "'=='", "'!='", "'<'", 
      "'>'", "'<='", "'>='", "'('", "')'", "','", "'warn'", "'error'"
    },
    std::vector<std::string>{
      "", "OR", "AND", "NOT", "IN", "RANGE", "EQ", "NE", "LT", "GT", "LE", 
      "GE", "LPAREN", "RPAREN", "COMMA", "WARN", "ERROR", "IDENT", "NUMBER", 
      "STRING", "WS"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,20,65,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,1,0,4,0,14,
  	8,0,11,0,12,0,15,1,0,1,0,1,1,1,1,1,1,1,1,1,2,1,2,1,2,1,2,1,2,1,2,1,2,
  	1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,44,8,2,1,2,1,
  	2,1,2,1,2,1,2,1,2,5,2,52,8,2,10,2,12,2,55,9,2,1,3,1,3,1,4,1,4,3,4,61,
  	8,4,1,5,1,5,1,5,0,1,4,6,0,2,4,6,8,10,0,2,1,0,15,16,1,0,6,11,65,0,13,1,
  	0,0,0,2,19,1,0,0,0,4,43,1,0,0,0,6,56,1,0,0,0,8,60,1,0,0,0,10,62,1,0,0,
  	0,12,14,3,2,1,0,13,12,1,0,0,0,14,15,1,0,0,0,15,13,1,0,0,0,15,16,1,0,0,
  	0,16,17,1,0,0,0,17,18,5,0,0,1,18,1,1,0,0,0,19,20,7,0,0,0,20,21,5,19,0,
  	0,21,22,3,4,2,0,22,3,1,0,0,0,23,24,6,2,-1,0,24,25,5,3,0,0,25,44,3,4,2,
  	4,26,27,5,12,0,0,27,28,3,4,2,0,28,29,5,13,0,0,29,44,1,0,0,0,30,31,5,17,
  	0,0,31,32,3,6,3,0,32,33,3,8,4,0,33,44,1,0,0,0,34,35,5,17,0,0,35,36,5,
  	4,0,0,36,37,5,5,0,0,37,38,5,12,0,0,38,39,3,10,5,0,39,40,5,14,0,0,40,41,
  	3,10,5,0,41,42,5,13,0,0,42,44,1,0,0,0,43,23,1,0,0,0,43,26,1,0,0,0,43,
  	30,1,0,0,0,43,34,1,0,0,0,44,53,1,0,0,0,45,46,10,6,0,0,46,47,5,1,0,0,47,
  	52,3,4,2,7,48,49,10,5,0,0,49,50,5,2,0,0,50,52,3,4,2,6,51,45,1,0,0,0,51,
  	48,1,0,0,0,52,55,1,0,0,0,53,51,1,0,0,0,53,54,1,0,0,0,54,5,1,0,0,0,55,
  	53,1,0,0,0,56,57,7,1,0,0,57,7,1,0,0,0,58,61,5,19,0,0,59,61,3,10,5,0,60,
  	58,1,0,0,0,60,59,1,0,0,0,61,9,1,0,0,0,62,63,5,18,0,0,63,11,1,0,0,0,5,
  	15,43,51,53,60
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  rulesParserStaticData = staticData.release();
}

}

rulesParser::rulesParser(TokenStream *input) : rulesParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

rulesParser::rulesParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  rulesParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *rulesParserStaticData->atn, rulesParserStaticData->decisionToDFA, rulesParserStaticData->sharedContextCache, options);
}

rulesParser::~rulesParser() {
  delete _interpreter;
}

const atn::ATN& rulesParser::getATN() const {
  return *rulesParserStaticData->atn;
}

std::string rulesParser::getGrammarFileName() const {
  return "rules.g4";
}

const std::vector<std::string>& rulesParser::getRuleNames() const {
  return rulesParserStaticData->ruleNames;
}

const dfa::Vocabulary& rulesParser::getVocabulary() const {
  return rulesParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView rulesParser::getSerializedATN() const {
  return rulesParserStaticData->serializedATN;
}


//----------------- ProgramContext ------------------------------------------------------------------

rulesParser::ProgramContext::ProgramContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* rulesParser::ProgramContext::EOF() {
  return getToken(rulesParser::EOF, 0);
}

std::vector<rulesParser::StatementContext *> rulesParser::ProgramContext::statement() {
  return getRuleContexts<rulesParser::StatementContext>();
}

rulesParser::StatementContext* rulesParser::ProgramContext::statement(size_t i) {
  return getRuleContext<rulesParser::StatementContext>(i);
}


size_t rulesParser::ProgramContext::getRuleIndex() const {
  return rulesParser::RuleProgram;
}

void rulesParser::ProgramContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterProgram(this);
}

void rulesParser::ProgramContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitProgram(this);
}


std::any rulesParser::ProgramContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<rulesVisitor*>(visitor))
    return parserVisitor->visitProgram(this);
  else
    return visitor->visitChildren(this);
}

rulesParser::ProgramContext* rulesParser::program() {
  ProgramContext *_localctx = _tracker.createInstance<ProgramContext>(_ctx, getState());
  enterRule(_localctx, 0, rulesParser::RuleProgram);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(13); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(12);
      statement();
      setState(15); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == rulesParser::WARN

    || _la == rulesParser::ERROR);
    setState(17);
    match(rulesParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StatementContext ------------------------------------------------------------------

rulesParser::StatementContext::StatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* rulesParser::StatementContext::STRING() {
  return getToken(rulesParser::STRING, 0);
}

rulesParser::ExprContext* rulesParser::StatementContext::expr() {
  return getRuleContext<rulesParser::ExprContext>(0);
}

tree::TerminalNode* rulesParser::StatementContext::WARN() {
  return getToken(rulesParser::WARN, 0);
}

tree::TerminalNode* rulesParser::StatementContext::ERROR() {
  return getToken(rulesParser::ERROR, 0);
}


size_t rulesParser::StatementContext::getRuleIndex() const {
  return rulesParser::RuleStatement;
}

void rulesParser::StatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStatement(this);
}

void rulesParser::StatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStatement(this);
}


std::any rulesParser::StatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<rulesVisitor*>(visitor))
    return parserVisitor->visitStatement(this);
  else
    return visitor->visitChildren(this);
}

rulesParser::StatementContext* rulesParser::statement() {
  StatementContext *_localctx = _tracker.createInstance<StatementContext>(_ctx, getState());
  enterRule(_localctx, 2, rulesParser::RuleStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(19);
    _la = _input->LA(1);
    if (!(_la == rulesParser::WARN

    || _la == rulesParser::ERROR)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(20);
    match(rulesParser::STRING);
    setState(21);
    expr(0);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExprContext ------------------------------------------------------------------

rulesParser::ExprContext::ExprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t rulesParser::ExprContext::getRuleIndex() const {
  return rulesParser::RuleExpr;
}

void rulesParser::ExprContext::copyFrom(ExprContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- AndExprContext ------------------------------------------------------------------

std::vector<rulesParser::ExprContext *> rulesParser::AndExprContext::expr() {
  return getRuleContexts<rulesParser::ExprContext>();
}

rulesParser::ExprContext* rulesParser::AndExprContext::expr(size_t i) {
  return getRuleContext<rulesParser::ExprContext>(i);
}

tree::TerminalNode* rulesParser::AndExprContext::AND() {
  return getToken(rulesParser::AND, 0);
}

rulesParser::AndExprContext::AndExprContext(ExprContext *ctx) { copyFrom(ctx); }

void rulesParser::AndExprContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAndExpr(this);
}
void rulesParser::AndExprContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAndExpr(this);
}

std::any rulesParser::AndExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<rulesVisitor*>(visitor))
    return parserVisitor->visitAndExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- CompareExprContext ------------------------------------------------------------------

tree::TerminalNode* rulesParser::CompareExprContext::IDENT() {
  return getToken(rulesParser::IDENT, 0);
}

rulesParser::CompOpContext* rulesParser::CompareExprContext::compOp() {
  return getRuleContext<rulesParser::CompOpContext>(0);
}

rulesParser::ValueContext* rulesParser::CompareExprContext::value() {
  return getRuleContext<rulesParser::ValueContext>(0);
}

rulesParser::CompareExprContext::CompareExprContext(ExprContext *ctx) { copyFrom(ctx); }

void rulesParser::CompareExprContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCompareExpr(this);
}
void rulesParser::CompareExprContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCompareExpr(this);
}

std::any rulesParser::CompareExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<rulesVisitor*>(visitor))
    return parserVisitor->visitCompareExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NotExprContext ------------------------------------------------------------------

tree::TerminalNode* rulesParser::NotExprContext::NOT() {
  return getToken(rulesParser::NOT, 0);
}

rulesParser::ExprContext* rulesParser::NotExprContext::expr() {
  return getRuleContext<rulesParser::ExprContext>(0);
}

rulesParser::NotExprContext::NotExprContext(ExprContext *ctx) { copyFrom(ctx); }

void rulesParser::NotExprContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterNotExpr(this);
}
void rulesParser::NotExprContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitNotExpr(this);
}

std::any rulesParser::NotExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<rulesVisitor*>(visitor))
    return parserVisitor->visitNotExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ParenExprContext ------------------------------------------------------------------

tree::TerminalNode* rulesParser::ParenExprContext::LPAREN() {
  return getToken(rulesParser::LPAREN, 0);
}

rulesParser::ExprContext* rulesParser::ParenExprContext::expr() {
  return getRuleContext<rulesParser::ExprContext>(0);
}

tree::TerminalNode* rulesParser::ParenExprContext::RPAREN() {
  return getToken(rulesParser::RPAREN, 0);
}

rulesParser::ParenExprContext::ParenExprContext(ExprContext *ctx) { copyFrom(ctx); }

void rulesParser::ParenExprContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterParenExpr(this);
}
void rulesParser::ParenExprContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitParenExpr(this);
}

std::any rulesParser::ParenExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<rulesVisitor*>(visitor))
    return parserVisitor->visitParenExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- RangeExprContext ------------------------------------------------------------------

tree::TerminalNode* rulesParser::RangeExprContext::IDENT() {
  return getToken(rulesParser::IDENT, 0);
}

tree::TerminalNode* rulesParser::RangeExprContext::IN() {
  return getToken(rulesParser::IN, 0);
}

tree::TerminalNode* rulesParser::RangeExprContext::RANGE() {
  return getToken(rulesParser::RANGE, 0);
}

tree::TerminalNode* rulesParser::RangeExprContext::LPAREN() {
  return getToken(rulesParser::LPAREN, 0);
}

std::vector<rulesParser::NumberContext *> rulesParser::RangeExprContext::number() {
  return getRuleContexts<rulesParser::NumberContext>();
}

rulesParser::NumberContext* rulesParser::RangeExprContext::number(size_t i) {
  return getRuleContext<rulesParser::NumberContext>(i);
}

tree::TerminalNode* rulesParser::RangeExprContext::COMMA() {
  return getToken(rulesParser::COMMA, 0);
}

tree::TerminalNode* rulesParser::RangeExprContext::RPAREN() {
  return getToken(rulesParser::RPAREN, 0);
}

rulesParser::RangeExprContext::RangeExprContext(ExprContext *ctx) { copyFrom(ctx); }

void rulesParser::RangeExprContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRangeExpr(this);
}
void rulesParser::RangeExprContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRangeExpr(this);
}

std::any rulesParser::RangeExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<rulesVisitor*>(visitor))
    return parserVisitor->visitRangeExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- OrExprContext ------------------------------------------------------------------

std::vector<rulesParser::ExprContext *> rulesParser::OrExprContext::expr() {
  return getRuleContexts<rulesParser::ExprContext>();
}

rulesParser::ExprContext* rulesParser::OrExprContext::expr(size_t i) {
  return getRuleContext<rulesParser::ExprContext>(i);
}

tree::TerminalNode* rulesParser::OrExprContext::OR() {
  return getToken(rulesParser::OR, 0);
}

rulesParser::OrExprContext::OrExprContext(ExprContext *ctx) { copyFrom(ctx); }

void rulesParser::OrExprContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterOrExpr(this);
}
void rulesParser::OrExprContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitOrExpr(this);
}

std::any rulesParser::OrExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<rulesVisitor*>(visitor))
    return parserVisitor->visitOrExpr(this);
  else
    return visitor->visitChildren(this);
}

rulesParser::ExprContext* rulesParser::expr() {
   return expr(0);
}

rulesParser::ExprContext* rulesParser::expr(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  rulesParser::ExprContext *_localctx = _tracker.createInstance<ExprContext>(_ctx, parentState);
  rulesParser::ExprContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 4;
  enterRecursionRule(_localctx, 4, rulesParser::RuleExpr, precedence);

    

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(43);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<NotExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(24);
      match(rulesParser::NOT);
      setState(25);
      expr(4);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<ParenExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(26);
      match(rulesParser::LPAREN);
      setState(27);
      expr(0);
      setState(28);
      match(rulesParser::RPAREN);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<CompareExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(30);
      match(rulesParser::IDENT);
      setState(31);
      compOp();
      setState(32);
      value();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<RangeExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(34);
      match(rulesParser::IDENT);
      setState(35);
      match(rulesParser::IN);
      setState(36);
      match(rulesParser::RANGE);
      setState(37);
      match(rulesParser::LPAREN);
      setState(38);
      number();
      setState(39);
      match(rulesParser::COMMA);
      setState(40);
      number();
      setState(41);
      match(rulesParser::RPAREN);
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(53);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(51);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 2, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<OrExprContext>(_tracker.createInstance<ExprContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpr);
          setState(45);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(46);
          match(rulesParser::OR);
          setState(47);
          expr(7);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<AndExprContext>(_tracker.createInstance<ExprContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpr);
          setState(48);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(49);
          match(rulesParser::AND);
          setState(50);
          expr(6);
          break;
        }

        default:
          break;
        } 
      }
      setState(55);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- CompOpContext ------------------------------------------------------------------

rulesParser::CompOpContext::CompOpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* rulesParser::CompOpContext::EQ() {
  return getToken(rulesParser::EQ, 0);
}

tree::TerminalNode* rulesParser::CompOpContext::NE() {
  return getToken(rulesParser::NE, 0);
}

tree::TerminalNode* rulesParser::CompOpContext::LT() {
  return getToken(rulesParser::LT, 0);
}

tree::TerminalNode* rulesParser::CompOpContext::GT() {
  return getToken(rulesParser::GT, 0);
}

tree::TerminalNode* rulesParser::CompOpContext::LE() {
  return getToken(rulesParser::LE, 0);
}

tree::TerminalNode* rulesParser::CompOpContext::GE() {
  return getToken(rulesParser::GE, 0);
}


size_t rulesParser::CompOpContext::getRuleIndex() const {
  return rulesParser::RuleCompOp;
}

void rulesParser::CompOpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCompOp(this);
}

void rulesParser::CompOpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCompOp(this);
}


std::any rulesParser::CompOpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<rulesVisitor*>(visitor))
    return parserVisitor->visitCompOp(this);
  else
    return visitor->visitChildren(this);
}

rulesParser::CompOpContext* rulesParser::compOp() {
  CompOpContext *_localctx = _tracker.createInstance<CompOpContext>(_ctx, getState());
  enterRule(_localctx, 6, rulesParser::RuleCompOp);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(56);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 4032) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ValueContext ------------------------------------------------------------------

rulesParser::ValueContext::ValueContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* rulesParser::ValueContext::STRING() {
  return getToken(rulesParser::STRING, 0);
}

rulesParser::NumberContext* rulesParser::ValueContext::number() {
  return getRuleContext<rulesParser::NumberContext>(0);
}


size_t rulesParser::ValueContext::getRuleIndex() const {
  return rulesParser::RuleValue;
}

void rulesParser::ValueContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterValue(this);
}

void rulesParser::ValueContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitValue(this);
}


std::any rulesParser::ValueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<rulesVisitor*>(visitor))
    return parserVisitor->visitValue(this);
  else
    return visitor->visitChildren(this);
}

rulesParser::ValueContext* rulesParser::value() {
  ValueContext *_localctx = _tracker.createInstance<ValueContext>(_ctx, getState());
  enterRule(_localctx, 8, rulesParser::RuleValue);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(60);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case rulesParser::STRING: {
        enterOuterAlt(_localctx, 1);
        setState(58);
        match(rulesParser::STRING);
        break;
      }

      case rulesParser::NUMBER: {
        enterOuterAlt(_localctx, 2);
        setState(59);
        number();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NumberContext ------------------------------------------------------------------

rulesParser::NumberContext::NumberContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* rulesParser::NumberContext::NUMBER() {
  return getToken(rulesParser::NUMBER, 0);
}


size_t rulesParser::NumberContext::getRuleIndex() const {
  return rulesParser::RuleNumber;
}

void rulesParser::NumberContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterNumber(this);
}

void rulesParser::NumberContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<rulesListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitNumber(this);
}


std::any rulesParser::NumberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<rulesVisitor*>(visitor))
    return parserVisitor->visitNumber(this);
  else
    return visitor->visitChildren(this);
}

rulesParser::NumberContext* rulesParser::number() {
  NumberContext *_localctx = _tracker.createInstance<NumberContext>(_ctx, getState());
  enterRule(_localctx, 10, rulesParser::RuleNumber);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(62);
    match(rulesParser::NUMBER);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

bool rulesParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 2: return exprSempred(antlrcpp::downCast<ExprContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool rulesParser::exprSempred(ExprContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 6);
    case 1: return precpred(_ctx, 5);

  default:
    break;
  }
  return true;
}

void rulesParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  rulesParserInitialize();
#else
  ::antlr4::internal::call_once(rulesParserOnceFlag, rulesParserInitialize);
#endif
}
