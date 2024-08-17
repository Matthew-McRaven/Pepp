
// Generated from PeppLexer.g4 by ANTLR 4.13.1


#include "PeppLexer.h"


using namespace antlr4;

using namespace parse;


using namespace antlr4;

namespace {

struct PeppLexerStaticData final {
  PeppLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  PeppLexerStaticData(const PeppLexerStaticData&) = delete;
  PeppLexerStaticData(PeppLexerStaticData&&) = delete;
  PeppLexerStaticData& operator=(const PeppLexerStaticData&) = delete;
  PeppLexerStaticData& operator=(PeppLexerStaticData&&) = delete;

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

::antlr4::internal::OnceFlag pepplexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
PeppLexerStaticData *pepplexerLexerStaticData = nullptr;

void pepplexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (pepplexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(pepplexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<PeppLexerStaticData>(
    std::vector<std::string>{
      "SPACING", "NEWLINE", "STRING", "CHARACTER", "NameChar", "NameStartChar", 
      "IDENTIFIER", "DOLLAR", "PLACEHOLDER_MACRO", "DOT_IDENTIFIER", "AT_IDENTIFIER", 
      "COLON", "SYMBOL", "PLACEHOLDER_SYMBOL", "DEC_DIGIT", "HEX_DIGIT", 
      "UNSIGNED_DECIMAL", "SIGNED_DECIMAL", "HEXADECIMAL", "SEMICOLON", 
      "COMMENT", "COMMENT_BODY", "COMMA"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "'$'", "", "", "", "':'", "", "", "", "", 
      "", "", "','"
    },
    std::vector<std::string>{
      "", "SPACING", "NEWLINE", "STRING", "CHARACTER", "IDENTIFIER", "DOLLAR", 
      "PLACEHOLDER_MACRO", "DOT_IDENTIFIER", "AT_IDENTIFIER", "COLON", "SYMBOL", 
      "PLACEHOLDER_SYMBOL", "UNSIGNED_DECIMAL", "SIGNED_DECIMAL", "HEXADECIMAL", 
      "COMMENT", "COMMA"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,0,17,163,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
  	6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,
  	7,14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,
  	7,21,2,22,7,22,1,0,4,0,49,8,0,11,0,12,0,50,1,0,1,0,1,1,3,1,56,8,1,1,1,
  	1,1,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,5,2,70,8,2,10,2,12,2,73,9,
  	2,1,2,1,2,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,87,8,3,1,3,1,3,
  	1,4,1,4,3,4,93,8,4,1,5,1,5,1,6,1,6,5,6,99,8,6,10,6,12,6,102,9,6,1,7,1,
  	7,1,8,1,8,4,8,108,8,8,11,8,12,8,109,1,9,1,9,1,9,1,10,1,10,1,10,1,11,1,
  	11,1,12,1,12,1,12,1,13,1,13,1,13,1,14,1,14,1,15,1,15,3,15,130,8,15,1,
  	16,4,16,133,8,16,11,16,12,16,134,1,17,1,17,4,17,139,8,17,11,17,12,17,
  	140,1,18,1,18,1,18,1,18,4,18,147,8,18,11,18,12,18,148,1,19,1,19,1,20,
  	1,20,1,20,1,21,5,21,157,8,21,10,21,12,21,160,9,21,1,22,1,22,0,0,23,1,
  	1,3,2,5,3,7,4,9,0,11,0,13,5,15,6,17,7,19,8,21,9,23,10,25,11,27,12,29,
  	0,31,0,33,13,35,14,37,15,39,0,41,16,43,0,45,17,1,0,11,2,0,9,9,32,32,1,
  	0,13,13,1,0,10,10,2,0,34,34,92,92,2,0,10,10,13,13,2,0,88,88,120,120,2,
  	0,39,39,92,92,3,0,65,90,95,95,97,122,1,0,48,57,2,0,65,70,97,102,2,0,43,
  	43,45,45,171,0,1,1,0,0,0,0,3,1,0,0,0,0,5,1,0,0,0,0,7,1,0,0,0,0,13,1,0,
  	0,0,0,15,1,0,0,0,0,17,1,0,0,0,0,19,1,0,0,0,0,21,1,0,0,0,0,23,1,0,0,0,
  	0,25,1,0,0,0,0,27,1,0,0,0,0,33,1,0,0,0,0,35,1,0,0,0,0,37,1,0,0,0,0,41,
  	1,0,0,0,0,45,1,0,0,0,1,48,1,0,0,0,3,55,1,0,0,0,5,59,1,0,0,0,7,76,1,0,
  	0,0,9,92,1,0,0,0,11,94,1,0,0,0,13,96,1,0,0,0,15,103,1,0,0,0,17,105,1,
  	0,0,0,19,111,1,0,0,0,21,114,1,0,0,0,23,117,1,0,0,0,25,119,1,0,0,0,27,
  	122,1,0,0,0,29,125,1,0,0,0,31,129,1,0,0,0,33,132,1,0,0,0,35,136,1,0,0,
  	0,37,142,1,0,0,0,39,150,1,0,0,0,41,152,1,0,0,0,43,158,1,0,0,0,45,161,
  	1,0,0,0,47,49,7,0,0,0,48,47,1,0,0,0,49,50,1,0,0,0,50,48,1,0,0,0,50,51,
  	1,0,0,0,51,52,1,0,0,0,52,53,6,0,0,0,53,2,1,0,0,0,54,56,7,1,0,0,55,54,
  	1,0,0,0,55,56,1,0,0,0,56,57,1,0,0,0,57,58,7,2,0,0,58,4,1,0,0,0,59,71,
  	5,34,0,0,60,70,8,3,0,0,61,62,5,92,0,0,62,70,8,4,0,0,63,64,5,92,0,0,64,
  	65,7,5,0,0,65,66,1,0,0,0,66,67,3,31,15,0,67,68,3,31,15,0,68,70,1,0,0,
  	0,69,60,1,0,0,0,69,61,1,0,0,0,69,63,1,0,0,0,70,73,1,0,0,0,71,69,1,0,0,
  	0,71,72,1,0,0,0,72,74,1,0,0,0,73,71,1,0,0,0,74,75,5,34,0,0,75,6,1,0,0,
  	0,76,86,5,39,0,0,77,87,8,6,0,0,78,79,5,92,0,0,79,87,9,0,0,0,80,81,5,92,
  	0,0,81,82,7,5,0,0,82,83,1,0,0,0,83,84,3,31,15,0,84,85,3,31,15,0,85,87,
  	1,0,0,0,86,77,1,0,0,0,86,78,1,0,0,0,86,80,1,0,0,0,87,88,1,0,0,0,88,89,
  	5,39,0,0,89,8,1,0,0,0,90,93,3,11,5,0,91,93,2,48,57,0,92,90,1,0,0,0,92,
  	91,1,0,0,0,93,10,1,0,0,0,94,95,7,7,0,0,95,12,1,0,0,0,96,100,3,11,5,0,
  	97,99,3,9,4,0,98,97,1,0,0,0,99,102,1,0,0,0,100,98,1,0,0,0,100,101,1,0,
  	0,0,101,14,1,0,0,0,102,100,1,0,0,0,103,104,5,36,0,0,104,16,1,0,0,0,105,
  	107,3,15,7,0,106,108,3,29,14,0,107,106,1,0,0,0,108,109,1,0,0,0,109,107,
  	1,0,0,0,109,110,1,0,0,0,110,18,1,0,0,0,111,112,5,46,0,0,112,113,3,13,
  	6,0,113,20,1,0,0,0,114,115,5,64,0,0,115,116,3,13,6,0,116,22,1,0,0,0,117,
  	118,5,58,0,0,118,24,1,0,0,0,119,120,3,13,6,0,120,121,3,23,11,0,121,26,
  	1,0,0,0,122,123,3,17,8,0,123,124,3,23,11,0,124,28,1,0,0,0,125,126,7,8,
  	0,0,126,30,1,0,0,0,127,130,3,29,14,0,128,130,7,9,0,0,129,127,1,0,0,0,
  	129,128,1,0,0,0,130,32,1,0,0,0,131,133,3,29,14,0,132,131,1,0,0,0,133,
  	134,1,0,0,0,134,132,1,0,0,0,134,135,1,0,0,0,135,34,1,0,0,0,136,138,7,
  	10,0,0,137,139,3,29,14,0,138,137,1,0,0,0,139,140,1,0,0,0,140,138,1,0,
  	0,0,140,141,1,0,0,0,141,36,1,0,0,0,142,143,5,48,0,0,143,144,7,5,0,0,144,
  	146,1,0,0,0,145,147,3,31,15,0,146,145,1,0,0,0,147,148,1,0,0,0,148,146,
  	1,0,0,0,148,149,1,0,0,0,149,38,1,0,0,0,150,151,5,59,0,0,151,40,1,0,0,
  	0,152,153,3,39,19,0,153,154,3,43,21,0,154,42,1,0,0,0,155,157,8,4,0,0,
  	156,155,1,0,0,0,157,160,1,0,0,0,158,156,1,0,0,0,158,159,1,0,0,0,159,44,
  	1,0,0,0,160,158,1,0,0,0,161,162,5,44,0,0,162,46,1,0,0,0,14,0,50,55,69,
  	71,86,92,100,109,129,134,140,148,158,1,6,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  pepplexerLexerStaticData = staticData.release();
}

}

PeppLexer::PeppLexer(CharStream *input) : Lexer(input) {
  PeppLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *pepplexerLexerStaticData->atn, pepplexerLexerStaticData->decisionToDFA, pepplexerLexerStaticData->sharedContextCache);
}

PeppLexer::~PeppLexer() {
  delete _interpreter;
}

std::string PeppLexer::getGrammarFileName() const {
  return "PeppLexer.g4";
}

const std::vector<std::string>& PeppLexer::getRuleNames() const {
  return pepplexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& PeppLexer::getChannelNames() const {
  return pepplexerLexerStaticData->channelNames;
}

const std::vector<std::string>& PeppLexer::getModeNames() const {
  return pepplexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& PeppLexer::getVocabulary() const {
  return pepplexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView PeppLexer::getSerializedATN() const {
  return pepplexerLexerStaticData->serializedATN;
}

const atn::ATN& PeppLexer::getATN() const {
  return *pepplexerLexerStaticData->atn;
}




void PeppLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  pepplexerLexerInitialize();
#else
  ::antlr4::internal::call_once(pepplexerLexerOnceFlag, pepplexerLexerInitialize);
#endif
}
