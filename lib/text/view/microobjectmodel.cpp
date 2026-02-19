#include "microobjectmodel.hpp"
#include "core/arch/pep/uarch/pep.hpp"
#include "core/langs/ucode/pep_parser.hpp"
#include "help/builtins/book.hpp"
#include "help/builtins/figure.hpp"
#include "help/builtins/registry.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"

pepp::MicroObjectModel::MicroObjectModel(QObject *parent) : QAbstractTableModel(parent) {
  auto bookReg = builtins::Registry();
  auto book6 = helpers::book(6, &bookReg);
  using regs = pepp::tc::arch::Pep9Registers;
  auto source = book6->findFigure("12", "12")->typesafeNamedFragments().value("pepcpu");
  auto mc = pepp::tc::parse::MicroParser<pepp::tc::arch::Pep9ByteBus, regs>(source->contents().toStdString()).parse();
  auto microcode = pepp::tc::parse::microcodeEnableFor<pepp::tc::arch::Pep9ByteBus, regs>(mc);
  const auto size = pepp::tc::arch::Pep9ByteBus::signal_to_string().size();
  for (unsigned int it = 0; it < size; it++) {
    _headers.append(QString::fromStdString(
        pepp::tc::arch::Pep9ByteBus::signal_to_string().at(static_cast<pepp::tc::arch::Pep9ByteBus::Signals>(it))));
  }
  for (const auto &line : microcode) {
    QList<int> temp;
    for (unsigned int it = 0; it < size; it++) {
      if (auto s = static_cast<pepp::tc::arch::Pep9ByteBus::Signals>(it); line.enabled(s)) temp.append(line.get(s));
      else temp.append(-1);
    }
    _values.append(std::move(temp));
  }
}

int pepp::MicroObjectModel::rowCount(const QModelIndex &) const { return _values.size(); }

int pepp::MicroObjectModel::columnCount(const QModelIndex &) const { return _headers.size(); }

QVariant pepp::MicroObjectModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) return _headers[section];
  else return {};
}

QVariant pepp::MicroObjectModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return {};
  if (role == Qt::DisplayRole && index.row() < _values.size() && index.column() < _headers.size()) {
    int value = _values[index.row()][index.column()];
    if (value == -1) return "";
    else return QString::number(value);
  } else return {};
}
