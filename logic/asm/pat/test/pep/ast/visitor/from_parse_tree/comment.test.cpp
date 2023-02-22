#include <QObject>
#include <QTest>

#include "pat/pep/ast/visitor/ast_from_parse_tree.hpp"
#include "pat/pep/ast/visitor/to_ptr.hpp"
#include "pat/pep/isa/pep10.hpp"
#include "pat/pep/parse/rules_lines.hpp"
using namespace std::literals::string_literals;
class PatPepAstVisitor_FromParseTreeComment : public QObject {
  Q_OBJECT
private slots:
  void test1() {
    auto value = ";magic"s;
    std::vector<pat::pep::parse::LineType> result;
    parse(value.begin(), value.end(), pat::pep::parse::line, result);
    QCOMPARE(result.size(), 1);
    auto visit = pat::pep::ast::visitor::FromParseTree<Pep10ISA>();
    auto node = result[0].apply_visitor(visit);
    auto ptrVisit = pat::pep::ast::visitor::ToPtr();
    auto ptr = node.apply_visitor(ptrVisit);
    QCOMPARE_NE(ptr, nullptr);
    QCOMPARE_NE(qSharedPointerCast<pat::ast::node::Comment>(ptr), nullptr);
  };
};

#include "comment.test.moc"

QTEST_MAIN(PatPepAstVisitor_FromParseTreeComment);
