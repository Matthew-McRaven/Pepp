
// Generated from Macro.g4 by ANTLR 4.13.1



#include "MacroParser.h"


using namespace antlrcpp;
using namespace macro::detail;

using namespace antlr4;

namespace {

struct MacroParserStaticData final {
  MacroParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  MacroParserStaticData(const MacroParserStaticData&) = delete;
  MacroParserStaticData(MacroParserStaticData&&) = delete;
  MacroParserStaticData& operator=(const MacroParserStaticData&) = delete;
  MacroParserStaticData& operator=(MacroParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag macroParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
MacroParserStaticData *macroParserStaticData = nullptr;

void macroParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (macroParserStaticData != nullptr) {
    return;
  }
#else
  assert(macroParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<MacroParserStaticData>(
    std::vector<std::string>{
      "decl"
    },
    std::vector<std::string>{
    },
    std::vector<std::string>{
      "", "SPACING", "NEWLINE", "IDENTIFIER", "AT_IDENTIFIER", "UNSIGNED_DECIMAL"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,5,6,2,0,7,0,1,0,1,0,1,0,1,0,0,0,1,0,0,0,4,0,2,1,0,0,0,2,3,5,4,0,0,
  	3,4,5,5,0,0,4,1,1,0,0,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  macroParserStaticData = staticData.release();
}

}

MacroParser::MacroParser(TokenStream *input) : MacroParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

MacroParser::MacroParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  MacroParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *macroParserStaticData->atn, macroParserStaticData->decisionToDFA, macroParserStaticData->sharedContextCache, options);
}

MacroParser::~MacroParser() {
  delete _interpreter;
}

const atn::ATN& MacroParser::getATN() const {
  return *macroParserStaticData->atn;
}

std::string MacroParser::getGrammarFileName() const {
  return "Macro.g4";
}

const std::vector<std::string>& MacroParser::getRuleNames() const {
  return macroParserStaticData->ruleNames;
}

const dfa::Vocabulary& MacroParser::getVocabulary() const {
  return macroParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView MacroParser::getSerializedATN() const {
  return macroParserStaticData->serializedATN;
}


//----------------- DeclContext ------------------------------------------------------------------

MacroParser::DeclContext::DeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* MacroParser::DeclContext::AT_IDENTIFIER() {
  return getToken(MacroParser::AT_IDENTIFIER, 0);
}

tree::TerminalNode* MacroParser::DeclContext::UNSIGNED_DECIMAL() {
  return getToken(MacroParser::UNSIGNED_DECIMAL, 0);
}


size_t MacroParser::DeclContext::getRuleIndex() const {
  return MacroParser::RuleDecl;
}


MacroParser::DeclContext* MacroParser::decl() {
  DeclContext *_localctx = _tracker.createInstance<DeclContext>(_ctx, getState());
  enterRule(_localctx, 0, MacroParser::RuleDecl);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(2);
    match(MacroParser::AT_IDENTIFIER);
    setState(3);
    match(MacroParser::UNSIGNED_DECIMAL);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void MacroParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  macroParserInitialize();
#else
  ::antlr4::internal::call_once(macroParserOnceFlag, macroParserInitialize);
#endif
}
