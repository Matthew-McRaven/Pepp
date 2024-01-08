#include "rules_clike.h"

#include "style/types.hpp"

static const auto single_quote_str="((\')(?![\'])(([^\'|\\\\]){1}|((\\\\)([\'|b|f|n|r|t|v|\"|\\\\]))|((\\\\)(([x|X])([0-9|A-F|a-f]{2}))))(\'))";
const auto single_quote_re = QRegularExpression(single_quote_str, QRegularExpression::PatternOption::CaseInsensitiveOption);

static const auto double_quote_str="((\")((([^\"|\\\\])|((\\\\)([\'|b|f|n|r|t|v|\"|\\\\]))|((\\\\)(([x|X])([0-9|A-F|a-f]{2}))))*)(\"))";
const auto double_quote_re = QRegularExpression(double_quote_str, QRegularExpression::PatternOption::CaseInsensitiveOption);

QList<highlight::Rule> highlight::rules_c()
{
    QList<highlight::Rule> rules;

    auto function_re = QRegularExpression("\\b[A-Za-z0-9_]+(?=[\\s]*\\()", QRegularExpression::PatternOption::CaseInsensitiveOption);
    rules.append({function_re, style::Types::FunctionDec});

    QStringList typesKeywordPatterns;
    typesKeywordPatterns << "\\bbool\\b" << "\\bchar\\b" << "\\bconst\\b" << "\\bcase\\b"
                        << "\\benum\\b" << "\\bint\\b" << "\\bnamespace\\b" << "\\bstruct\\b"
                        << "\\busing\\b" << "\\#include\\b" << "\\bvoid\\b";
    auto declaration_re = QRegularExpression(typesKeywordPatterns.join("|"), QRegularExpression::PatternOption::CaseInsensitiveOption);
    rules.append({declaration_re, style::Types::OtherKeyword});

    QStringList keywordPatterns;
    keywordPatterns << "\\bwhile\\b" << "\\bfor\\b" << "\\bswitch\\b"
                    << "\\bif\\b" << "\\bdo\\b" << "\\bmalloc\\b"
                    << "\\breturn\\b" << "\\belse\\b";
    auto keyword_re = QRegularExpression(keywordPatterns.join("|"), QRegularExpression::PatternOption::CaseInsensitiveOption);
    rules.append({declaration_re, style::Types::Keyword});

    auto class_re = QRegularExpression("\\bQ[A-Za-z]+\\b", QRegularExpression::PatternOption::CaseInsensitiveOption);
    rules.append({class_re, style::Types::FunctionDec});

    rules.append(Rule{QRegularExpression("/\\*"), style::Types::Comment, 0, 1, 1});
    rules.append(Rule{QRegularExpression("\\*/"), style::Types::Comment, 1, 0, 1});
    rules.append(Rule{QRegularExpression(".*"), style::Types::Comment, 1, 1, 1});
    rules.append(Rule{QRegularExpression("//.*$"), style::Types::Comment, 0, 0, 1});
    rules.append(Rule{single_quote_re, style::Types::Quoted});
    rules.append(Rule{double_quote_re, style::Types::Quoted});

    return rules;
}

QList<highlight::Rule> highlight::rules_cpp()
{
    return {};
}
