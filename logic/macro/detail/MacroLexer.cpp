
// Generated from Macro.g4 by ANTLR 4.13.1


#include "MacroLexer.h"


using namespace antlr4;

using namespace macro::detail;


using namespace antlr4;

namespace {

struct MacroLexerStaticData final {
  MacroLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  MacroLexerStaticData(const MacroLexerStaticData&) = delete;
  MacroLexerStaticData(MacroLexerStaticData&&) = delete;
  MacroLexerStaticData& operator=(const MacroLexerStaticData&) = delete;
  MacroLexerStaticData& operator=(MacroLexerStaticData&&) = delete;

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

::antlr4::internal::OnceFlag macrolexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
MacroLexerStaticData *macrolexerLexerStaticData = nullptr;

void macrolexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (macrolexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(macrolexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<MacroLexerStaticData>(
    std::vector<std::string>{
      "SPACING", "NEWLINE", "NameChar", "NameStartChar", "IDENTIFIER", "AT_IDENTIFIER", 
      "DEC_DIGIT", "UNSIGNED_DECIMAL"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
    },
    std::vector<std::string>{
    },
    std::vector<std::string>{
      "", "SPACING", "NEWLINE", "IDENTIFIER", "AT_IDENTIFIER", "UNSIGNED_DECIMAL"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,0,5,54,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,
  	2,7,7,7,1,0,4,0,19,8,0,11,0,12,0,20,1,0,1,0,1,1,1,1,1,1,3,1,28,8,1,1,
  	1,1,1,1,2,1,2,3,2,34,8,2,1,3,1,3,1,4,1,4,5,4,40,8,4,10,4,12,4,43,9,4,
  	1,5,1,5,1,5,1,6,1,6,1,7,4,7,51,8,7,11,7,12,7,52,0,0,8,1,1,3,2,5,0,7,0,
  	9,3,11,4,13,0,15,5,1,0,7,2,0,9,9,32,32,2,0,10,10,13,13,1,0,13,13,1,0,
  	10,10,2,0,48,57,95,95,2,0,65,90,97,122,1,0,48,57,55,0,1,1,0,0,0,0,3,1,
  	0,0,0,0,9,1,0,0,0,0,11,1,0,0,0,0,15,1,0,0,0,1,18,1,0,0,0,3,27,1,0,0,0,
  	5,33,1,0,0,0,7,35,1,0,0,0,9,37,1,0,0,0,11,44,1,0,0,0,13,47,1,0,0,0,15,
  	50,1,0,0,0,17,19,7,0,0,0,18,17,1,0,0,0,19,20,1,0,0,0,20,18,1,0,0,0,20,
  	21,1,0,0,0,21,22,1,0,0,0,22,23,6,0,0,0,23,2,1,0,0,0,24,28,7,1,0,0,25,
  	26,7,2,0,0,26,28,7,3,0,0,27,24,1,0,0,0,27,25,1,0,0,0,28,29,1,0,0,0,29,
  	30,6,1,0,0,30,4,1,0,0,0,31,34,3,7,3,0,32,34,7,4,0,0,33,31,1,0,0,0,33,
  	32,1,0,0,0,34,6,1,0,0,0,35,36,7,5,0,0,36,8,1,0,0,0,37,41,3,7,3,0,38,40,
  	3,5,2,0,39,38,1,0,0,0,40,43,1,0,0,0,41,39,1,0,0,0,41,42,1,0,0,0,42,10,
  	1,0,0,0,43,41,1,0,0,0,44,45,5,64,0,0,45,46,3,9,4,0,46,12,1,0,0,0,47,48,
  	7,6,0,0,48,14,1,0,0,0,49,51,3,13,6,0,50,49,1,0,0,0,51,52,1,0,0,0,52,50,
  	1,0,0,0,52,53,1,0,0,0,53,16,1,0,0,0,6,0,20,27,33,41,52,1,6,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  macrolexerLexerStaticData = staticData.release();
}

}

MacroLexer::MacroLexer(CharStream *input) : Lexer(input) {
  MacroLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *macrolexerLexerStaticData->atn, macrolexerLexerStaticData->decisionToDFA, macrolexerLexerStaticData->sharedContextCache);
}

MacroLexer::~MacroLexer() {
  delete _interpreter;
}

std::string MacroLexer::getGrammarFileName() const {
  return "Macro.g4";
}

const std::vector<std::string>& MacroLexer::getRuleNames() const {
  return macrolexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& MacroLexer::getChannelNames() const {
  return macrolexerLexerStaticData->channelNames;
}

const std::vector<std::string>& MacroLexer::getModeNames() const {
  return macrolexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& MacroLexer::getVocabulary() const {
  return macrolexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView MacroLexer::getSerializedATN() const {
  return macrolexerLexerStaticData->serializedATN;
}

const atn::ATN& MacroLexer::getATN() const {
  return *macrolexerLexerStaticData->atn;
}




void MacroLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  macrolexerLexerInitialize();
#else
  ::antlr4::internal::call_once(macrolexerLexerOnceFlag, macrolexerLexerInitialize);
#endif
}
