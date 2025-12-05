#include "./seekable.hpp"

#include <QRegularExpression>

pepp::tc::support::SeekableData::SeekableData(QString d, support::Location loc) : data(std::move(d)), _loc(loc) {}

QChar pepp::tc::support::SeekableData::peek() {
  if (!input_remains()) return '\0';
  return data[_end];
}

QStringView pepp::tc::support::SeekableData::select() const {
  auto count = _end - _start;
  auto ret = QStringView(data).sliced(_start, count);
  return ret;
}

QStringView pepp::tc::support::SeekableData::rest() const { return data.sliced(_end); }

bool pepp::tc::support::SeekableData::input_remains() const { return _end < data.size(); }

void pepp::tc::support::SeekableData::advance(size_t n) {
  _end += n;
  _loc.column += n;
}

void pepp::tc::support::SeekableData::skip(size_t n) {
  _start = _end += n;
  _loc.column += n;
}

void pepp::tc::support::SeekableData::newline() { _loc.row++, _loc.column = 0; }

QRegularExpressionMatch pepp::tc::support::SeekableData::matchView(const QRegularExpression &re) {
  static const auto NormalMatch = QRegularExpression::NormalMatch;
  static const auto Anchored = QRegularExpression::AnchorAtOffsetMatchOption;
  return re.matchView(data, _start, NormalMatch, Anchored);
}

pepp::tc::support::Location pepp::tc::support::SeekableData::location() const { return _loc; }
