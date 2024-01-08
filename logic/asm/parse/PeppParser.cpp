
// Generated from PeppParser.g4 by ANTLR 4.13.1


#include "PeppParserListener.h"
#include "PeppParserVisitor.h"

#include "PeppParser.h"


using namespace antlrcpp;
using namespace parse;

using namespace antlr4;

namespace {

struct PeppParserStaticData final {
  PeppParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  PeppParserStaticData(const PeppParserStaticData&) = delete;
  PeppParserStaticData(PeppParserStaticData&&) = delete;
  PeppParserStaticData& operator=(const PeppParserStaticData&) = delete;
  PeppParserStaticData& operator=(PeppParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag peppparserParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
PeppParserStaticData *peppparserParserStaticData = nullptr;

void peppparserParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (peppparserParserStaticData != nullptr) {
    return;
  }
#else
  assert(peppparserParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<PeppParserStaticData>(
    std::vector<std::string>{
      "prog", "instruction", "directive", "invoke_macro", "symbol", "stat", 
      "argument", "argument_list"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "'$'", "", "", "", "':'", "", "", "", "", 
      "", "';'", "", "','"
    },
    std::vector<std::string>{
      "", "SPACING", "NEWLINE", "STRING", "CHARACTER", "IDENTIFIER", "DOLLAR", 
      "PLACEHOLDER_MACRO", "DOT_IDENTIFIER", "AT_IDENTIFIER", "COLON", "SYMBOL", 
      "PLACEHOLDER_SYMBOL", "UNSIGNED_DECIMAL", "SIGNED_DECIMAL", "HEXADECIMAL", 
      "SEMICOLON", "COMMENT", "COMMA"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,18,88,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,7,
  	7,7,1,0,1,0,1,0,1,0,1,0,1,0,1,0,5,0,24,8,0,10,0,12,0,27,9,0,1,0,1,0,1,
  	1,1,1,1,1,1,1,3,1,35,8,1,1,1,3,1,38,8,1,1,2,1,2,3,2,42,8,2,1,3,1,3,3,
  	3,46,8,3,1,4,1,4,1,5,3,5,51,8,5,1,5,1,5,3,5,55,8,5,1,5,3,5,58,8,5,1,5,
  	1,5,3,5,62,8,5,1,5,1,5,3,5,66,8,5,1,5,1,5,3,5,70,8,5,1,5,3,5,73,8,5,1,
  	5,3,5,76,8,5,1,6,1,6,1,7,1,7,1,7,5,7,83,8,7,10,7,12,7,86,9,7,1,7,0,0,
  	8,0,2,4,6,8,10,12,14,0,3,2,0,5,5,7,7,1,0,11,12,3,0,3,5,7,7,13,15,97,0,
  	25,1,0,0,0,2,37,1,0,0,0,4,39,1,0,0,0,6,43,1,0,0,0,8,47,1,0,0,0,10,75,
  	1,0,0,0,12,77,1,0,0,0,14,79,1,0,0,0,16,24,5,2,0,0,17,18,3,10,5,0,18,19,
  	5,2,0,0,19,24,1,0,0,0,20,21,3,10,5,0,21,22,4,0,0,0,22,24,1,0,0,0,23,16,
  	1,0,0,0,23,17,1,0,0,0,23,20,1,0,0,0,24,27,1,0,0,0,25,23,1,0,0,0,25,26,
  	1,0,0,0,26,28,1,0,0,0,27,25,1,0,0,0,28,29,5,0,0,1,29,1,1,0,0,0,30,31,
  	7,0,0,0,31,34,3,12,6,0,32,33,5,18,0,0,33,35,7,0,0,0,34,32,1,0,0,0,34,
  	35,1,0,0,0,35,38,1,0,0,0,36,38,7,0,0,0,37,30,1,0,0,0,37,36,1,0,0,0,38,
  	3,1,0,0,0,39,41,5,8,0,0,40,42,3,14,7,0,41,40,1,0,0,0,41,42,1,0,0,0,42,
  	5,1,0,0,0,43,45,5,9,0,0,44,46,3,14,7,0,45,44,1,0,0,0,45,46,1,0,0,0,46,
  	7,1,0,0,0,47,48,7,1,0,0,48,9,1,0,0,0,49,51,3,8,4,0,50,49,1,0,0,0,50,51,
  	1,0,0,0,51,52,1,0,0,0,52,54,3,2,1,0,53,55,5,17,0,0,54,53,1,0,0,0,54,55,
  	1,0,0,0,55,76,1,0,0,0,56,58,3,8,4,0,57,56,1,0,0,0,57,58,1,0,0,0,58,59,
  	1,0,0,0,59,61,3,4,2,0,60,62,5,17,0,0,61,60,1,0,0,0,61,62,1,0,0,0,62,76,
  	1,0,0,0,63,65,4,5,1,0,64,66,3,8,4,0,65,64,1,0,0,0,65,66,1,0,0,0,66,67,
  	1,0,0,0,67,69,3,6,3,0,68,70,5,17,0,0,69,68,1,0,0,0,69,70,1,0,0,0,70,76,
  	1,0,0,0,71,73,3,8,4,0,72,71,1,0,0,0,72,73,1,0,0,0,73,74,1,0,0,0,74,76,
  	5,17,0,0,75,50,1,0,0,0,75,57,1,0,0,0,75,63,1,0,0,0,75,72,1,0,0,0,76,11,
  	1,0,0,0,77,78,7,2,0,0,78,13,1,0,0,0,79,84,3,12,6,0,80,81,5,18,0,0,81,
  	83,3,12,6,0,82,80,1,0,0,0,83,86,1,0,0,0,84,82,1,0,0,0,84,85,1,0,0,0,85,
  	15,1,0,0,0,86,84,1,0,0,0,15,23,25,34,37,41,45,50,54,57,61,65,69,72,75,
  	84
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  peppparserParserStaticData = staticData.release();
}

}

PeppParser::PeppParser(TokenStream *input) : PeppParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

PeppParser::PeppParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  PeppParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *peppparserParserStaticData->atn, peppparserParserStaticData->decisionToDFA, peppparserParserStaticData->sharedContextCache, options);
}

PeppParser::~PeppParser() {
  delete _interpreter;
}

const atn::ATN& PeppParser::getATN() const {
  return *peppparserParserStaticData->atn;
}

std::string PeppParser::getGrammarFileName() const {
  return "PeppParser.g4";
}

const std::vector<std::string>& PeppParser::getRuleNames() const {
  return peppparserParserStaticData->ruleNames;
}

const dfa::Vocabulary& PeppParser::getVocabulary() const {
  return peppparserParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView PeppParser::getSerializedATN() const {
  return peppparserParserStaticData->serializedATN;
}


//----------------- ProgContext ------------------------------------------------------------------

PeppParser::ProgContext::ProgContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* PeppParser::ProgContext::EOF() {
  return getToken(PeppParser::EOF, 0);
}

std::vector<tree::TerminalNode *> PeppParser::ProgContext::NEWLINE() {
  return getTokens(PeppParser::NEWLINE);
}

tree::TerminalNode* PeppParser::ProgContext::NEWLINE(size_t i) {
  return getToken(PeppParser::NEWLINE, i);
}

std::vector<PeppParser::StatContext *> PeppParser::ProgContext::stat() {
  return getRuleContexts<PeppParser::StatContext>();
}

PeppParser::StatContext* PeppParser::ProgContext::stat(size_t i) {
  return getRuleContext<PeppParser::StatContext>(i);
}


size_t PeppParser::ProgContext::getRuleIndex() const {
  return PeppParser::RuleProg;
}

void PeppParser::ProgContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterProg(this);
}

void PeppParser::ProgContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitProg(this);
}


std::any PeppParser::ProgContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitProg(this);
  else
    return visitor->visitChildren(this);
}

PeppParser::ProgContext* PeppParser::prog() {
  ProgContext *_localctx = _tracker.createInstance<ProgContext>(_ctx, getState());
  enterRule(_localctx, 0, PeppParser::RuleProg);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(25);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(23);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 0, _ctx)) {
        case 1: {
          setState(16);
          match(PeppParser::NEWLINE);
          break;
        }

        case 2: {
          setState(17);
          stat();
          setState(18);
          match(PeppParser::NEWLINE);
          break;
        }

        case 3: {
          setState(20);
          stat();
          setState(21);

          if (!(EOF)) throw FailedPredicateException(this, "EOF");
          break;
        }

        default:
          break;
        } 
      }
      setState(27);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx);
    }
    setState(28);
    match(PeppParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- InstructionContext ------------------------------------------------------------------

PeppParser::InstructionContext::InstructionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t PeppParser::InstructionContext::getRuleIndex() const {
  return PeppParser::RuleInstruction;
}

void PeppParser::InstructionContext::copyFrom(InstructionContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- NonUnaryInstructionContext ------------------------------------------------------------------

PeppParser::ArgumentContext* PeppParser::NonUnaryInstructionContext::argument() {
  return getRuleContext<PeppParser::ArgumentContext>(0);
}

std::vector<tree::TerminalNode *> PeppParser::NonUnaryInstructionContext::IDENTIFIER() {
  return getTokens(PeppParser::IDENTIFIER);
}

tree::TerminalNode* PeppParser::NonUnaryInstructionContext::IDENTIFIER(size_t i) {
  return getToken(PeppParser::IDENTIFIER, i);
}

std::vector<tree::TerminalNode *> PeppParser::NonUnaryInstructionContext::PLACEHOLDER_MACRO() {
  return getTokens(PeppParser::PLACEHOLDER_MACRO);
}

tree::TerminalNode* PeppParser::NonUnaryInstructionContext::PLACEHOLDER_MACRO(size_t i) {
  return getToken(PeppParser::PLACEHOLDER_MACRO, i);
}

tree::TerminalNode* PeppParser::NonUnaryInstructionContext::COMMA() {
  return getToken(PeppParser::COMMA, 0);
}

PeppParser::NonUnaryInstructionContext::NonUnaryInstructionContext(InstructionContext *ctx) { copyFrom(ctx); }

void PeppParser::NonUnaryInstructionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterNonUnaryInstruction(this);
}
void PeppParser::NonUnaryInstructionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitNonUnaryInstruction(this);
}

std::any PeppParser::NonUnaryInstructionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitNonUnaryInstruction(this);
  else
    return visitor->visitChildren(this);
}
//----------------- UnaryInstructionContext ------------------------------------------------------------------

tree::TerminalNode* PeppParser::UnaryInstructionContext::IDENTIFIER() {
  return getToken(PeppParser::IDENTIFIER, 0);
}

tree::TerminalNode* PeppParser::UnaryInstructionContext::PLACEHOLDER_MACRO() {
  return getToken(PeppParser::PLACEHOLDER_MACRO, 0);
}

PeppParser::UnaryInstructionContext::UnaryInstructionContext(InstructionContext *ctx) { copyFrom(ctx); }

void PeppParser::UnaryInstructionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnaryInstruction(this);
}
void PeppParser::UnaryInstructionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnaryInstruction(this);
}

std::any PeppParser::UnaryInstructionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitUnaryInstruction(this);
  else
    return visitor->visitChildren(this);
}
PeppParser::InstructionContext* PeppParser::instruction() {
  InstructionContext *_localctx = _tracker.createInstance<InstructionContext>(_ctx, getState());
  enterRule(_localctx, 2, PeppParser::RuleInstruction);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(37);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<PeppParser::NonUnaryInstructionContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(30);
      _la = _input->LA(1);
      if (!(_la == PeppParser::IDENTIFIER

      || _la == PeppParser::PLACEHOLDER_MACRO)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(31);
      argument();
      setState(34);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 2, _ctx)) {
      case 1: {
        setState(32);
        match(PeppParser::COMMA);
        setState(33);
        _la = _input->LA(1);
        if (!(_la == PeppParser::IDENTIFIER

        || _la == PeppParser::PLACEHOLDER_MACRO)) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        break;
      }

      default:
        break;
      }
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<PeppParser::UnaryInstructionContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(36);
      _la = _input->LA(1);
      if (!(_la == PeppParser::IDENTIFIER

      || _la == PeppParser::PLACEHOLDER_MACRO)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- DirectiveContext ------------------------------------------------------------------

PeppParser::DirectiveContext::DirectiveContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* PeppParser::DirectiveContext::DOT_IDENTIFIER() {
  return getToken(PeppParser::DOT_IDENTIFIER, 0);
}

PeppParser::Argument_listContext* PeppParser::DirectiveContext::argument_list() {
  return getRuleContext<PeppParser::Argument_listContext>(0);
}


size_t PeppParser::DirectiveContext::getRuleIndex() const {
  return PeppParser::RuleDirective;
}

void PeppParser::DirectiveContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDirective(this);
}

void PeppParser::DirectiveContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDirective(this);
}


std::any PeppParser::DirectiveContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitDirective(this);
  else
    return visitor->visitChildren(this);
}

PeppParser::DirectiveContext* PeppParser::directive() {
  DirectiveContext *_localctx = _tracker.createInstance<DirectiveContext>(_ctx, getState());
  enterRule(_localctx, 4, PeppParser::RuleDirective);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(39);
    match(PeppParser::DOT_IDENTIFIER);
    setState(41);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 4, _ctx)) {
    case 1: {
      setState(40);
      argument_list();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Invoke_macroContext ------------------------------------------------------------------

PeppParser::Invoke_macroContext::Invoke_macroContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* PeppParser::Invoke_macroContext::AT_IDENTIFIER() {
  return getToken(PeppParser::AT_IDENTIFIER, 0);
}

PeppParser::Argument_listContext* PeppParser::Invoke_macroContext::argument_list() {
  return getRuleContext<PeppParser::Argument_listContext>(0);
}


size_t PeppParser::Invoke_macroContext::getRuleIndex() const {
  return PeppParser::RuleInvoke_macro;
}

void PeppParser::Invoke_macroContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterInvoke_macro(this);
}

void PeppParser::Invoke_macroContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitInvoke_macro(this);
}


std::any PeppParser::Invoke_macroContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitInvoke_macro(this);
  else
    return visitor->visitChildren(this);
}

PeppParser::Invoke_macroContext* PeppParser::invoke_macro() {
  Invoke_macroContext *_localctx = _tracker.createInstance<Invoke_macroContext>(_ctx, getState());
  enterRule(_localctx, 6, PeppParser::RuleInvoke_macro);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(43);
    match(PeppParser::AT_IDENTIFIER);
    setState(45);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 5, _ctx)) {
    case 1: {
      setState(44);
      argument_list();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SymbolContext ------------------------------------------------------------------

PeppParser::SymbolContext::SymbolContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* PeppParser::SymbolContext::SYMBOL() {
  return getToken(PeppParser::SYMBOL, 0);
}

tree::TerminalNode* PeppParser::SymbolContext::PLACEHOLDER_SYMBOL() {
  return getToken(PeppParser::PLACEHOLDER_SYMBOL, 0);
}


size_t PeppParser::SymbolContext::getRuleIndex() const {
  return PeppParser::RuleSymbol;
}

void PeppParser::SymbolContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSymbol(this);
}

void PeppParser::SymbolContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSymbol(this);
}


std::any PeppParser::SymbolContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitSymbol(this);
  else
    return visitor->visitChildren(this);
}

PeppParser::SymbolContext* PeppParser::symbol() {
  SymbolContext *_localctx = _tracker.createInstance<SymbolContext>(_ctx, getState());
  enterRule(_localctx, 8, PeppParser::RuleSymbol);
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
    setState(47);
    _la = _input->LA(1);
    if (!(_la == PeppParser::SYMBOL

    || _la == PeppParser::PLACEHOLDER_SYMBOL)) {
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

//----------------- StatContext ------------------------------------------------------------------

PeppParser::StatContext::StatContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t PeppParser::StatContext::getRuleIndex() const {
  return PeppParser::RuleStat;
}

void PeppParser::StatContext::copyFrom(StatContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- CommentLineContext ------------------------------------------------------------------

tree::TerminalNode* PeppParser::CommentLineContext::COMMENT() {
  return getToken(PeppParser::COMMENT, 0);
}

PeppParser::SymbolContext* PeppParser::CommentLineContext::symbol() {
  return getRuleContext<PeppParser::SymbolContext>(0);
}

PeppParser::CommentLineContext::CommentLineContext(StatContext *ctx) { copyFrom(ctx); }

void PeppParser::CommentLineContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCommentLine(this);
}
void PeppParser::CommentLineContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCommentLine(this);
}

std::any PeppParser::CommentLineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitCommentLine(this);
  else
    return visitor->visitChildren(this);
}
//----------------- InstructionLineContext ------------------------------------------------------------------

PeppParser::InstructionContext* PeppParser::InstructionLineContext::instruction() {
  return getRuleContext<PeppParser::InstructionContext>(0);
}

PeppParser::SymbolContext* PeppParser::InstructionLineContext::symbol() {
  return getRuleContext<PeppParser::SymbolContext>(0);
}

tree::TerminalNode* PeppParser::InstructionLineContext::COMMENT() {
  return getToken(PeppParser::COMMENT, 0);
}

PeppParser::InstructionLineContext::InstructionLineContext(StatContext *ctx) { copyFrom(ctx); }

void PeppParser::InstructionLineContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterInstructionLine(this);
}
void PeppParser::InstructionLineContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitInstructionLine(this);
}

std::any PeppParser::InstructionLineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitInstructionLine(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MacroLineContext ------------------------------------------------------------------

PeppParser::Invoke_macroContext* PeppParser::MacroLineContext::invoke_macro() {
  return getRuleContext<PeppParser::Invoke_macroContext>(0);
}

PeppParser::SymbolContext* PeppParser::MacroLineContext::symbol() {
  return getRuleContext<PeppParser::SymbolContext>(0);
}

tree::TerminalNode* PeppParser::MacroLineContext::COMMENT() {
  return getToken(PeppParser::COMMENT, 0);
}

PeppParser::MacroLineContext::MacroLineContext(StatContext *ctx) { copyFrom(ctx); }

void PeppParser::MacroLineContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMacroLine(this);
}
void PeppParser::MacroLineContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMacroLine(this);
}

std::any PeppParser::MacroLineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitMacroLine(this);
  else
    return visitor->visitChildren(this);
}
//----------------- DirectiveLineContext ------------------------------------------------------------------

PeppParser::DirectiveContext* PeppParser::DirectiveLineContext::directive() {
  return getRuleContext<PeppParser::DirectiveContext>(0);
}

PeppParser::SymbolContext* PeppParser::DirectiveLineContext::symbol() {
  return getRuleContext<PeppParser::SymbolContext>(0);
}

tree::TerminalNode* PeppParser::DirectiveLineContext::COMMENT() {
  return getToken(PeppParser::COMMENT, 0);
}

PeppParser::DirectiveLineContext::DirectiveLineContext(StatContext *ctx) { copyFrom(ctx); }

void PeppParser::DirectiveLineContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDirectiveLine(this);
}
void PeppParser::DirectiveLineContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDirectiveLine(this);
}

std::any PeppParser::DirectiveLineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitDirectiveLine(this);
  else
    return visitor->visitChildren(this);
}
PeppParser::StatContext* PeppParser::stat() {
  StatContext *_localctx = _tracker.createInstance<StatContext>(_ctx, getState());
  enterRule(_localctx, 10, PeppParser::RuleStat);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(75);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<PeppParser::InstructionLineContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(50);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == PeppParser::SYMBOL

      || _la == PeppParser::PLACEHOLDER_SYMBOL) {
        setState(49);
        symbol();
      }
      setState(52);
      instruction();
      setState(54);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 7, _ctx)) {
      case 1: {
        setState(53);
        match(PeppParser::COMMENT);
        break;
      }

      default:
        break;
      }
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<PeppParser::DirectiveLineContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(57);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == PeppParser::SYMBOL

      || _la == PeppParser::PLACEHOLDER_SYMBOL) {
        setState(56);
        symbol();
      }
      setState(59);
      directive();
      setState(61);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx)) {
      case 1: {
        setState(60);
        match(PeppParser::COMMENT);
        break;
      }

      default:
        break;
      }
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<PeppParser::MacroLineContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(63);

      if (!(allow_macros)) throw FailedPredicateException(this, "allow_macros");
      setState(65);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == PeppParser::SYMBOL

      || _la == PeppParser::PLACEHOLDER_SYMBOL) {
        setState(64);
        symbol();
      }
      setState(67);
      invoke_macro();
      setState(69);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 11, _ctx)) {
      case 1: {
        setState(68);
        match(PeppParser::COMMENT);
        break;
      }

      default:
        break;
      }
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<PeppParser::CommentLineContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(72);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == PeppParser::SYMBOL

      || _la == PeppParser::PLACEHOLDER_SYMBOL) {
        setState(71);
        symbol();
      }
      setState(74);
      match(PeppParser::COMMENT);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ArgumentContext ------------------------------------------------------------------

PeppParser::ArgumentContext::ArgumentContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* PeppParser::ArgumentContext::HEXADECIMAL() {
  return getToken(PeppParser::HEXADECIMAL, 0);
}

tree::TerminalNode* PeppParser::ArgumentContext::UNSIGNED_DECIMAL() {
  return getToken(PeppParser::UNSIGNED_DECIMAL, 0);
}

tree::TerminalNode* PeppParser::ArgumentContext::SIGNED_DECIMAL() {
  return getToken(PeppParser::SIGNED_DECIMAL, 0);
}

tree::TerminalNode* PeppParser::ArgumentContext::STRING() {
  return getToken(PeppParser::STRING, 0);
}

tree::TerminalNode* PeppParser::ArgumentContext::CHARACTER() {
  return getToken(PeppParser::CHARACTER, 0);
}

tree::TerminalNode* PeppParser::ArgumentContext::IDENTIFIER() {
  return getToken(PeppParser::IDENTIFIER, 0);
}

tree::TerminalNode* PeppParser::ArgumentContext::PLACEHOLDER_MACRO() {
  return getToken(PeppParser::PLACEHOLDER_MACRO, 0);
}


size_t PeppParser::ArgumentContext::getRuleIndex() const {
  return PeppParser::RuleArgument;
}

void PeppParser::ArgumentContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterArgument(this);
}

void PeppParser::ArgumentContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitArgument(this);
}


std::any PeppParser::ArgumentContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitArgument(this);
  else
    return visitor->visitChildren(this);
}

PeppParser::ArgumentContext* PeppParser::argument() {
  ArgumentContext *_localctx = _tracker.createInstance<ArgumentContext>(_ctx, getState());
  enterRule(_localctx, 12, PeppParser::RuleArgument);
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
    setState(77);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 57528) != 0))) {
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

//----------------- Argument_listContext ------------------------------------------------------------------

PeppParser::Argument_listContext::Argument_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<PeppParser::ArgumentContext *> PeppParser::Argument_listContext::argument() {
  return getRuleContexts<PeppParser::ArgumentContext>();
}

PeppParser::ArgumentContext* PeppParser::Argument_listContext::argument(size_t i) {
  return getRuleContext<PeppParser::ArgumentContext>(i);
}

std::vector<tree::TerminalNode *> PeppParser::Argument_listContext::COMMA() {
  return getTokens(PeppParser::COMMA);
}

tree::TerminalNode* PeppParser::Argument_listContext::COMMA(size_t i) {
  return getToken(PeppParser::COMMA, i);
}


size_t PeppParser::Argument_listContext::getRuleIndex() const {
  return PeppParser::RuleArgument_list;
}

void PeppParser::Argument_listContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterArgument_list(this);
}

void PeppParser::Argument_listContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<PeppParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitArgument_list(this);
}


std::any PeppParser::Argument_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<PeppParserVisitor*>(visitor))
    return parserVisitor->visitArgument_list(this);
  else
    return visitor->visitChildren(this);
}

PeppParser::Argument_listContext* PeppParser::argument_list() {
  Argument_listContext *_localctx = _tracker.createInstance<Argument_listContext>(_ctx, getState());
  enterRule(_localctx, 14, PeppParser::RuleArgument_list);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(79);
    argument();
    setState(84);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 14, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(80);
        match(PeppParser::COMMA);
        setState(81);
        argument(); 
      }
      setState(86);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 14, _ctx);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

bool PeppParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 0: return progSempred(antlrcpp::downCast<ProgContext *>(context), predicateIndex);
    case 5: return statSempred(antlrcpp::downCast<StatContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool PeppParser::progSempred(ProgContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return EOF;

  default:
    break;
  }
  return true;
}

bool PeppParser::statSempred(StatContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 1: return allow_macros;

  default:
    break;
  }
  return true;
}

void PeppParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  peppparserParserInitialize();
#else
  ::antlr4::internal::call_once(peppparserParserOnceFlag, peppparserParserInitialize);
#endif
}
