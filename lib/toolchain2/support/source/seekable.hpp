#pragma once
#include <QString>
#include "./location.hpp"
namespace pepp::tc::support {
// A structure to support sequential reading of a 1d string and provide a row/column abstraction on top of it.
struct SeekableData {
  SeekableData() = default;
  explicit SeekableData(QString d, support::Location loc = {0, 0});
  // View the next character without adjusting counters.
  QChar peek();
  // Get the text between _start and _end, inclusive;
  QStringView select() const;
  // View the remaing text from _end to the end of data.
  QStringView rest() const;
  // Returns true if there is more input to read.
  bool input_remains() const;

  // Advance the end and row cursors by n characters.
  void advance(size_t n);
  // Advance the start, end, and row cursors by n characters.
  void skip(size_t n);
  // Advance the row cursor by one, and reset the column cursor to 0.
  void newline();
  // Perform a AnchorAtOffsetMatchOption match at the current start position.
  // You must check the result for a match and call advance() if you want to move forward.
  QRegularExpressionMatch matchView(const QRegularExpression &re);
  support::Location location() const;

private:
  QString data = "\n";
  // Offsets into data
  size_t _start = 0, _end = 0;
  support::Location _loc;
};
} // namespace pepp::tc::support
