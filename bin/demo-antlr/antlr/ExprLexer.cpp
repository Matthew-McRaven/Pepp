
// Generated from Expr.g4 by ANTLR 4.13.1


#include "ExprLexer.h"


using namespace antlr4;



using namespace antlr4;

namespace {

struct ExprLexerStaticData final {
  ExprLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  ExprLexerStaticData(const ExprLexerStaticData&) = delete;
  ExprLexerStaticData(ExprLexerStaticData&&) = delete;
  ExprLexerStaticData& operator=(const ExprLexerStaticData&) = delete;
  ExprLexerStaticData& operator=(ExprLexerStaticData&&) = delete;

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

::antlr4::internal::OnceFlag exprlexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
ExprLexerStaticData *exprlexerLexerStaticData = nullptr;

void exprlexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (exprlexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(exprlexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<ExprLexerStaticData>(
    std::vector<std::string>{
      "T__0", "T__1", "T__2", "T__3", "T__4", "T__5", "VAR", "NEWLINE", 
      "INT"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
    },
    std::vector<std::string>{
      "", "'*'", "'/'", "'+'", "'-'", "'('", "')'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "VAR", "NEWLINE", "INT"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,0,9,45,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,
  	2,7,7,7,2,8,7,8,1,0,1,0,1,1,1,1,1,2,1,2,1,3,1,3,1,4,1,4,1,5,1,5,1,6,1,
  	6,1,7,4,7,35,8,7,11,7,12,7,36,1,7,1,7,1,8,4,8,42,8,8,11,8,12,8,43,0,0,
  	9,1,1,3,2,5,3,7,4,9,5,11,6,13,7,15,8,17,9,1,0,3,1,0,97,122,3,0,10,10,
  	13,13,32,32,1,0,48,57,46,0,1,1,0,0,0,0,3,1,0,0,0,0,5,1,0,0,0,0,7,1,0,
  	0,0,0,9,1,0,0,0,0,11,1,0,0,0,0,13,1,0,0,0,0,15,1,0,0,0,0,17,1,0,0,0,1,
  	19,1,0,0,0,3,21,1,0,0,0,5,23,1,0,0,0,7,25,1,0,0,0,9,27,1,0,0,0,11,29,
  	1,0,0,0,13,31,1,0,0,0,15,34,1,0,0,0,17,41,1,0,0,0,19,20,5,42,0,0,20,2,
  	1,0,0,0,21,22,5,47,0,0,22,4,1,0,0,0,23,24,5,43,0,0,24,6,1,0,0,0,25,26,
  	5,45,0,0,26,8,1,0,0,0,27,28,5,40,0,0,28,10,1,0,0,0,29,30,5,41,0,0,30,
  	12,1,0,0,0,31,32,7,0,0,0,32,14,1,0,0,0,33,35,7,1,0,0,34,33,1,0,0,0,35,
  	36,1,0,0,0,36,34,1,0,0,0,36,37,1,0,0,0,37,38,1,0,0,0,38,39,6,7,0,0,39,
  	16,1,0,0,0,40,42,7,2,0,0,41,40,1,0,0,0,42,43,1,0,0,0,43,41,1,0,0,0,43,
  	44,1,0,0,0,44,18,1,0,0,0,3,0,36,43,1,6,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  exprlexerLexerStaticData = staticData.release();
}

}

ExprLexer::ExprLexer(CharStream *input) : Lexer(input) {
  ExprLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *exprlexerLexerStaticData->atn, exprlexerLexerStaticData->decisionToDFA, exprlexerLexerStaticData->sharedContextCache);
}

ExprLexer::~ExprLexer() {
  delete _interpreter;
}

std::string ExprLexer::getGrammarFileName() const {
  return "Expr.g4";
}

const std::vector<std::string>& ExprLexer::getRuleNames() const {
  return exprlexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& ExprLexer::getChannelNames() const {
  return exprlexerLexerStaticData->channelNames;
}

const std::vector<std::string>& ExprLexer::getModeNames() const {
  return exprlexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& ExprLexer::getVocabulary() const {
  return exprlexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView ExprLexer::getSerializedATN() const {
  return exprlexerLexerStaticData->serializedATN;
}

const atn::ATN& ExprLexer::getATN() const {
  return *exprlexerLexerStaticData->atn;
}




void ExprLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  exprlexerLexerInitialize();
#else
  ::antlr4::internal::call_once(exprlexerLexerOnceFlag, exprlexerLexerInitialize);
#endif
}
