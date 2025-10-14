
// Generated from rules.g4 by ANTLR 4.13.1


#include "rulesLexer.h"


using namespace antlr4;



using namespace antlr4;

namespace {

struct RulesLexerStaticData final {
  RulesLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  RulesLexerStaticData(const RulesLexerStaticData&) = delete;
  RulesLexerStaticData(RulesLexerStaticData&&) = delete;
  RulesLexerStaticData& operator=(const RulesLexerStaticData&) = delete;
  RulesLexerStaticData& operator=(RulesLexerStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> channelNames;
  const std::vector<std::string> modeNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag ruleslexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
RulesLexerStaticData *ruleslexerLexerStaticData = nullptr;

void ruleslexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (ruleslexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(ruleslexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<RulesLexerStaticData>(
    std::vector<std::string>{
      "OR", "AND", "NOT", "IN", "RANGE", "EQ", "NE", "LT", "GT", "LE", "GE", 
      "LPAREN", "RPAREN", "COMMA", "VERSION", "WARN", "ERROR", "IDENT", 
      "VERSIONID", "NUMBER", "STRING", "WS", "COMMENT"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
    },
    std::vector<std::string>{
      "", "'or'", "'and'", "'not'", "'in'", "'range'", "'=='", "'!='", "'<'", 
      "'>'", "'<='", "'>='", "'('", "')'", "','", "'version'", "'warn'", 
      "'error'"
    },
    std::vector<std::string>{
      "", "OR", "AND", "NOT", "IN", "RANGE", "EQ", "NE", "LT", "GT", "LE", 
      "GE", "LPAREN", "RPAREN", "COMMA", "VERSION", "WARN", "ERROR", "IDENT", 
      "VERSIONID", "NUMBER", "STRING", "WS", "COMMENT"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,0,23,170,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
  	6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,
  	7,14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,
  	7,21,2,22,7,22,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,2,1,2,1,2,1,2,1,3,1,3,1,
  	3,1,4,1,4,1,4,1,4,1,4,1,4,1,5,1,5,1,5,1,6,1,6,1,6,1,7,1,7,1,8,1,8,1,9,
  	1,9,1,9,1,10,1,10,1,10,1,11,1,11,1,12,1,12,1,13,1,13,1,14,1,14,1,14,1,
  	14,1,14,1,14,1,14,1,14,1,15,1,15,1,15,1,15,1,15,1,16,1,16,1,16,1,16,1,
  	16,1,16,1,17,1,17,5,17,111,8,17,10,17,12,17,114,9,17,1,18,4,18,117,8,
  	18,11,18,12,18,118,1,18,4,18,122,8,18,11,18,12,18,123,1,18,4,18,127,8,
  	18,11,18,12,18,128,1,19,4,19,132,8,19,11,19,12,19,133,1,19,1,19,4,19,
  	138,8,19,11,19,12,19,139,3,19,142,8,19,1,20,1,20,1,20,1,20,5,20,148,8,
  	20,10,20,12,20,151,9,20,1,20,1,20,1,21,4,21,156,8,21,11,21,12,21,157,
  	1,21,1,21,1,22,1,22,5,22,164,8,22,10,22,12,22,167,9,22,1,22,1,22,0,0,
  	23,1,1,3,2,5,3,7,4,9,5,11,6,13,7,15,8,17,9,19,10,21,11,23,12,25,13,27,
  	14,29,15,31,16,33,17,35,18,37,19,39,20,41,21,43,22,45,23,1,0,7,3,0,65,
  	90,95,95,97,122,4,0,48,57,65,90,95,95,97,122,3,0,48,57,65,90,97,122,1,
  	0,48,57,2,0,34,34,92,92,3,0,9,10,13,13,32,32,2,0,10,10,13,13,180,0,1,
  	1,0,0,0,0,3,1,0,0,0,0,5,1,0,0,0,0,7,1,0,0,0,0,9,1,0,0,0,0,11,1,0,0,0,
  	0,13,1,0,0,0,0,15,1,0,0,0,0,17,1,0,0,0,0,19,1,0,0,0,0,21,1,0,0,0,0,23,
  	1,0,0,0,0,25,1,0,0,0,0,27,1,0,0,0,0,29,1,0,0,0,0,31,1,0,0,0,0,33,1,0,
  	0,0,0,35,1,0,0,0,0,37,1,0,0,0,0,39,1,0,0,0,0,41,1,0,0,0,0,43,1,0,0,0,
  	0,45,1,0,0,0,1,47,1,0,0,0,3,50,1,0,0,0,5,54,1,0,0,0,7,58,1,0,0,0,9,61,
  	1,0,0,0,11,67,1,0,0,0,13,70,1,0,0,0,15,73,1,0,0,0,17,75,1,0,0,0,19,77,
  	1,0,0,0,21,80,1,0,0,0,23,83,1,0,0,0,25,85,1,0,0,0,27,87,1,0,0,0,29,89,
  	1,0,0,0,31,97,1,0,0,0,33,102,1,0,0,0,35,108,1,0,0,0,37,121,1,0,0,0,39,
  	131,1,0,0,0,41,143,1,0,0,0,43,155,1,0,0,0,45,161,1,0,0,0,47,48,5,111,
  	0,0,48,49,5,114,0,0,49,2,1,0,0,0,50,51,5,97,0,0,51,52,5,110,0,0,52,53,
  	5,100,0,0,53,4,1,0,0,0,54,55,5,110,0,0,55,56,5,111,0,0,56,57,5,116,0,
  	0,57,6,1,0,0,0,58,59,5,105,0,0,59,60,5,110,0,0,60,8,1,0,0,0,61,62,5,114,
  	0,0,62,63,5,97,0,0,63,64,5,110,0,0,64,65,5,103,0,0,65,66,5,101,0,0,66,
  	10,1,0,0,0,67,68,5,61,0,0,68,69,5,61,0,0,69,12,1,0,0,0,70,71,5,33,0,0,
  	71,72,5,61,0,0,72,14,1,0,0,0,73,74,5,60,0,0,74,16,1,0,0,0,75,76,5,62,
  	0,0,76,18,1,0,0,0,77,78,5,60,0,0,78,79,5,61,0,0,79,20,1,0,0,0,80,81,5,
  	62,0,0,81,82,5,61,0,0,82,22,1,0,0,0,83,84,5,40,0,0,84,24,1,0,0,0,85,86,
  	5,41,0,0,86,26,1,0,0,0,87,88,5,44,0,0,88,28,1,0,0,0,89,90,5,118,0,0,90,
  	91,5,101,0,0,91,92,5,114,0,0,92,93,5,115,0,0,93,94,5,105,0,0,94,95,5,
  	111,0,0,95,96,5,110,0,0,96,30,1,0,0,0,97,98,5,119,0,0,98,99,5,97,0,0,
  	99,100,5,114,0,0,100,101,5,110,0,0,101,32,1,0,0,0,102,103,5,101,0,0,103,
  	104,5,114,0,0,104,105,5,114,0,0,105,106,5,111,0,0,106,107,5,114,0,0,107,
  	34,1,0,0,0,108,112,7,0,0,0,109,111,7,1,0,0,110,109,1,0,0,0,111,114,1,
  	0,0,0,112,110,1,0,0,0,112,113,1,0,0,0,113,36,1,0,0,0,114,112,1,0,0,0,
  	115,117,7,2,0,0,116,115,1,0,0,0,117,118,1,0,0,0,118,116,1,0,0,0,118,119,
  	1,0,0,0,119,120,1,0,0,0,120,122,5,46,0,0,121,116,1,0,0,0,122,123,1,0,
  	0,0,123,121,1,0,0,0,123,124,1,0,0,0,124,126,1,0,0,0,125,127,7,2,0,0,126,
  	125,1,0,0,0,127,128,1,0,0,0,128,126,1,0,0,0,128,129,1,0,0,0,129,38,1,
  	0,0,0,130,132,7,3,0,0,131,130,1,0,0,0,132,133,1,0,0,0,133,131,1,0,0,0,
  	133,134,1,0,0,0,134,141,1,0,0,0,135,137,5,46,0,0,136,138,7,3,0,0,137,
  	136,1,0,0,0,138,139,1,0,0,0,139,137,1,0,0,0,139,140,1,0,0,0,140,142,1,
  	0,0,0,141,135,1,0,0,0,141,142,1,0,0,0,142,40,1,0,0,0,143,149,5,34,0,0,
  	144,148,8,4,0,0,145,146,5,92,0,0,146,148,9,0,0,0,147,144,1,0,0,0,147,
  	145,1,0,0,0,148,151,1,0,0,0,149,147,1,0,0,0,149,150,1,0,0,0,150,152,1,
  	0,0,0,151,149,1,0,0,0,152,153,5,34,0,0,153,42,1,0,0,0,154,156,7,5,0,0,
  	155,154,1,0,0,0,156,157,1,0,0,0,157,155,1,0,0,0,157,158,1,0,0,0,158,159,
  	1,0,0,0,159,160,6,21,0,0,160,44,1,0,0,0,161,165,5,35,0,0,162,164,8,6,
  	0,0,163,162,1,0,0,0,164,167,1,0,0,0,165,163,1,0,0,0,165,166,1,0,0,0,166,
  	168,1,0,0,0,167,165,1,0,0,0,168,169,6,22,0,0,169,46,1,0,0,0,12,0,112,
  	118,123,128,133,139,141,147,149,157,165,1,6,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  ruleslexerLexerStaticData = staticData.release();
}

}

rulesLexer::rulesLexer(CharStream *input) : Lexer(input) {
  rulesLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *ruleslexerLexerStaticData->atn, ruleslexerLexerStaticData->decisionToDFA, ruleslexerLexerStaticData->sharedContextCache);
}

rulesLexer::~rulesLexer() {
  delete _interpreter;
}

std::string rulesLexer::getGrammarFileName() const {
  return "rules.g4";
}

const std::vector<std::string>& rulesLexer::getRuleNames() const {
  return ruleslexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& rulesLexer::getChannelNames() const {
  return ruleslexerLexerStaticData->channelNames;
}

const std::vector<std::string>& rulesLexer::getModeNames() const {
  return ruleslexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& rulesLexer::getVocabulary() const {
  return ruleslexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView rulesLexer::getSerializedATN() const {
  return ruleslexerLexerStaticData->serializedATN;
}

const atn::ATN& rulesLexer::getATN() const {
  return *ruleslexerLexerStaticData->atn;
}




void rulesLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  ruleslexerLexerInitialize();
#else
  ::antlr4::internal::call_once(ruleslexerLexerOnceFlag, ruleslexerLexerInitialize);
#endif
}
