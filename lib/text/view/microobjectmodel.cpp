#include "microobjectmodel.hpp"
#include "core/arch/pep/uarch/pep.hpp"
#include "core/langs/ucode/pep_parser.hpp"
#include "help/builtins/book.hpp"
#include "help/builtins/figure.hpp"
#include "help/builtins/registry.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"

Microcode::Microcode(pepp::MicrocodeChoice mc, pepp::Line2Address l2a, QObject *parent)
    : QObject(parent), choice(mc), line2addr(l2a) {}

pepp::MicroObjectModel::MicroObjectModel(QObject *parent) : QAbstractTableModel(parent) {}

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

namespace {
struct Decomposer {
  const pepp::Line2Address &lines;
  QStringList &_headers;
  QList<QList<int>> &_values;
  void operator()(const std::monostate &) {}
  void operator()(const pepp::OneByteMC9 &mc) {
    const auto line_count = mc.size();
    const auto size = pepp::tc::arch::Pep9ByteBus::signal_to_string().size();
    _headers.append("Line Number");
    for (unsigned int it = 0; it < size; it++) {
      _headers.append(QString::fromStdString(
          pepp::tc::arch::Pep9ByteBus::signal_to_string().at(static_cast<pepp::tc::arch::Pep9ByteBus::Signals>(it))));
    }
    for (u32 addr = 0; addr < line_count; addr++) {
      const auto &line = mc[addr];
      QList<int> temp;
      temp.append(lines.line(addr).value_or(-2) + 1);
      for (unsigned int it = 0; it < size; it++) {
        if (auto s = static_cast<pepp::tc::arch::Pep9ByteBus::Signals>(it); line.enabled(s)) temp.append(line.get(s));
        else temp.append(-1);
      }
      _values.append(std::move(temp));
    }
  }
  void operator()(const pepp::TwoByteMC9 &mc) {
    const auto line_count = mc.size();
    const auto size = pepp::tc::arch::Pep9WordBus::signal_to_string().size();
    _headers.append("Line Number");
    for (unsigned int it = 0; it < size; it++) {
      _headers.append(QString::fromStdString(
          pepp::tc::arch::Pep9WordBus::signal_to_string().at(static_cast<pepp::tc::arch::Pep9WordBus::Signals>(it))));
    }
    for (u32 addr = 0; addr < line_count; addr++) {
      const auto &line = mc[addr];
      QList<int> temp;
      temp.append(lines.line(addr).value_or(-2) + 1);
      for (unsigned int it = 0; it < size; it++) {
        if (auto s = static_cast<pepp::tc::arch::Pep9WordBus::Signals>(it); line.enabled(s)) temp.append(line.get(s));
        else temp.append(-1);
      }
      _values.append(std::move(temp));
    }
  }
};
} // namespace
void pepp::MicroObjectModel::setMicrocode(Microcode *microcode) {
  beginResetModel();
  _headers = {};
  _values = {};
  if (microcode != nullptr) {
    Decomposer decompose{.lines = microcode->line2addr, ._headers = _headers, ._values = _values};
    std::visit(decompose, microcode->choice);
  }
  endResetModel();
}
