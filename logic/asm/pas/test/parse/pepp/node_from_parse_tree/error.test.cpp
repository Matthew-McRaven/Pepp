#include "pas/ast/generic/attr_error.hpp"
#include "pas/ast/generic/attr_location.hpp"
#include "pas/errors.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/parse/pepp/node_from_parse_tree.hpp"
#include <QObject>
#include <QTest>
#include <pas/operations/generic/errors.hpp>

using SourceLocation = pas::ast::generic::SourceLocation;
using Message = pas::ast::generic::Message;
using Severity = pas::ast::generic::Message::Severity;
using Error = QPair<SourceLocation, Message>;

auto makeFatal = [](qsizetype line, QString msg) {
  return QPair<SourceLocation, Message>{
      SourceLocation{.value = {.line = line, .valid = true}},
      Message{.severity = Severity::Fatal, .message = msg}};
};

class PasOpsPepp_NodeFromParseTree_Error : public QObject {
  Q_OBJECT
private slots:
  void fail() {
    QFETCH(QString, input);
    QFETCH(QList<Error>, errors);
    auto asStd = input.toStdString();

    // Convert input string to parsed lines.
    std::vector<pas::parse::pepp::LineType> result;
    bool success = true;
    QVERIFY_THROWS_NO_EXCEPTION([&]() {
      success =
          parse(asStd.begin(), asStd.end(), pas::parse::pepp::line, result);
    }());
    QVERIFY2(success, "Failed to parse");

    auto root = pas::parse::pepp::toAST<pas::isa::Pep10ISA>(result);
    auto visit = pas::ops::generic::collectErrors();
    pas::ast::apply_recurse<void>(*root, visit);
    auto actualErrors = visit.errors;
    for (int it = 0; it < qMin(errors.size(), actualErrors.size()); it++) {
      QCOMPARE(errors[it].first, actualErrors[it].first);
      QCOMPARE(errors[it].second, actualErrors[it].second);
    }
    QCOMPARE(errors.size(), actualErrors.size());
  }
  void fail_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QList<Error>>("errors");
    namespace E = pas::errors::pepp;
    // Unary Instructions
    QTest::addRow("unary: invalid mnemonic")
        << u"rat"_qs << QList<Error>{makeFatal(0, E::invalidMnemonic)};
    // NonUnary Instructions -- BR/Call family
    // NonUnary Instructions -- Not stores
    // NonUnary Instructions -- Stores
    /*
     * Directives
     */
    // ALIGN
    // - only decimal powers of 2 args
    // - exactly 1 arg
    // ASCII
    // - no chars
    // - no numeric (unsigned/signed decimal / hex)
    // - exactly 1 arg
    // BLOCK
    // - no signed
    // - no chars
    // - no strings (long or short)
    // - exactly 1 arg
    // - arg must fit in 16 bits (no big strs / hex / signed / unsigned)
    // BURN -- no symbol, only allows hex
    // - no decimal
    // - no text (chars/longstr/shortstr)
    // - no symbol
    // - exactly 1 arg
    // - arg must fit in 16 bits
    // Byte
    // - exactly 1 arg
    // - arg must fit in 8 bits
    // End
    // - no symbol
    // - exactly 0 args
    // Equate
    // - requires symbol
    // - exactly 1 argument
    // - arg fits in 16 bits
    // Export / Import / Input / Output / SCall / USCall
    // - no symbol
    // - exactly 1 arg
    // - arg must be symbolic
    // - arg must not be non-symbolic
    // Section
    // - exactly 1 arg (an identifier)
    // - no symbol
    // Word
    // - exactly 1 arg
    // - arg must fit in 16 bits
  }
};

#include "error.test.moc"

QTEST_MAIN(PasOpsPepp_NodeFromParseTree_Error)
