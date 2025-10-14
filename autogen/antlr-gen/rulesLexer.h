
// Generated from rules.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  rulesLexer : public antlr4::Lexer {
public:
  enum {
    OR = 1, AND = 2, NOT = 3, IN = 4, RANGE = 5, EQ = 6, NE = 7, LT = 8, 
    GT = 9, LE = 10, GE = 11, LPAREN = 12, RPAREN = 13, COMMA = 14, VERSION = 15, 
    WARN = 16, ERROR = 17, IDENT = 18, VERSIONID = 19, NUMBER = 20, STRING = 21, 
    WS = 22, COMMENT = 23
  };

  explicit rulesLexer(antlr4::CharStream *input);

  ~rulesLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

