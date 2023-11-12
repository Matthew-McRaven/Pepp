#include "rules_clike.h"

#include "style/types.hpp"

static const auto single_quote_str="((\')(?![\'])(([^\'|\\\\]){1}|((\\\\)([\'|b|f|n|r|t|v|\"|\\\\]))|((\\\\)(([x|X])([0-9|A-F|a-f]{2}))))(\'))";
const auto single_quote_re = QRegularExpression(single_quote_str, QRegularExpression::PatternOption::CaseInsensitiveOption);

static const auto double_quote_str="((\")((([^\"|\\\\])|((\\\\)([\'|b|f|n|r|t|v|\"|\\\\]))|((\\\\)(([x|X])([0-9|A-F|a-f]{2}))))*)(\"))";
const auto double_quote_re = QRegularExpression(double_quote_str, QRegularExpression::PatternOption::CaseInsensitiveOption);

QList<highlight::Rule> highlight::rules_c()
{
    QList<highlight::Rule> rules;

    auto lvalue_re = QRegularExpression("\\b[A-Za-z0-9_]+(?=[\\s]*\\()", QRegularExpression::PatternOption::CaseInsensitiveOption);
    rules.append({lvalue_re, style::Types::Mnemonic});

    QStringList declarationPatterns;
    declarationPatterns << "\\bbool\\b" << "\\bchar\\b" << "\\bconst\\b" << "\\bcase\\b"
    << "\\benum\\b" << "\\bint\\b" << "\\bnamespace\\b" << "\\bstruct\\b"
    << "\\busing\\b" << "\\#include\\b" << "\\bvoid\\b";
    for (const QString &pattern : declarationPatterns) {
        auto re = QRegularExpression(pattern, QRegularExpression::PatternOption::CaseInsensitiveOption);
        rules.append({re, style::Types::Dot});
    }

    rules.append(Rule{QRegularExpression("/\\*"), style::Types::Quoted, 0, 1});
    rules.append(Rule{QRegularExpression("\\*/"), style::Types::Quoted, 1, 0});
    rules.append(Rule{QRegularExpression(".*"), style::Types::Quoted, 1, 1});
    rules.append(Rule{QRegularExpression("//.*$"), style::Types::Quoted});
    rules.append(Rule{single_quote_re, style::Types::Quoted});
    rules.append(Rule{single_quote_re, style::Types::Quoted});
    rules.append(Rule{double_quote_re, style::Types::Quoted});

    return rules;
}

QList<highlight::Rule> highlight::rules_cpp()
{
    return {};
}
