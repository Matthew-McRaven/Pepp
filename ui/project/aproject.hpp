#pragma once
#include <QQmlEngine>
#include <QStringListModel>
#include <deque>
#include <qabstractitemmodel.h>
#include "cpu/registermodel.hpp"
#include "cpu/statusbitmodel.hpp"
#include "memory/hexdump/rawmemory.hpp"
#include "opcodemodel.hpp"
#include "utils/constants.hpp"

namespace project {
// Additional options requested for a project.
// A particular (arch, level) tuple may only support a subset of features.
// TODO: Wrap in a Q_OBJECT to expose to QML.
enum class Features : int {
  None = 0,
  OneByte,
  TwoByte,
  NoOS,
};

// TODO: Expose values on AProject directly
struct Environment {
  utils::Architecture arch;
  utils::Abstraction level;
  Features features;
};
} // namespace project

// Dummy base class which provides functionality common to all projects.
class AProject : public QObject {
  Q_OBJECT
  Q_PROPERTY(project::Environment env READ env)
public:
  explicit AProject(project::Environment env) : _env(env) {}
  project::Environment env() const { return _env; }
  virtual ~AProject() = default;
  // virtual void *memoryModel() = 0;
  // virtual void *cpuModel() = 0;

private:
  const project::Environment _env;
};

// TODO: move to bits
uint64_t mask(uint8_t byteCount);

struct HexFormatter : public RegisterFormatter {
  explicit HexFormatter(std::function<uint64_t()> fn, uint16_t byteCount = 2)
      : _fn(fn), _bytes(byteCount), _mask(byteCount) {}
  ~HexFormatter() override = default;
  QString format() const override { return u"0x%1"_qs.arg(_mask & _fn(), _bytes * 2, 16, QChar('0')); }
  bool readOnly() const override { return false; }
  qsizetype length() const override { return 2 * _bytes + 2; }

private:
  uint16_t _bytes = 0;
  uint64_t _mask = 0;
  std::function<uint64_t()> _fn;
};

struct UnsignedDecFormatter : public RegisterFormatter {
  explicit UnsignedDecFormatter(std::function<uint64_t()> fn, uint16_t byteCount = 2)
      : _fn(fn), _bytes(byteCount), _len(std::floor(std::log10(byteCount))), _mask(byteCount) {}
  ~UnsignedDecFormatter() override = default;
  QString format() const override { return QString::number(_mask & _fn()); }
  bool readOnly() const override { return false; }
  qsizetype length() const override { return _len; }

private:
  uint16_t _bytes = 0, _len = 0;
  uint64_t _mask = 0;
  std::function<uint64_t()> _fn;
};

struct SignedDecFormatter : public RegisterFormatter {
  explicit SignedDecFormatter(std::function<int64_t()> fn, uint16_t byteCount = 2)
      : _fn(fn), _bytes(byteCount), _len(std::floor(std::log10(byteCount))), _mask(mask(byteCount)) {}
  ~SignedDecFormatter() override = default;
  QString format() const override {
    if (auto v = _fn(); v < 0)
      // Limit V to the range expressable with an N-bit unsigned int before restoring the sign.
      return QString::number(~(_mask & ~v));
    return QString::number(_mask & _fn());
  }
  bool readOnly() const override { return false; }
  qsizetype length() const override { return 1 + _len; }

private:
  uint16_t _bytes = 0, _len = 0;
  uint64_t _mask = 0;
  std::function<int64_t()> _fn;
};

struct BinaryFormatter : public RegisterFormatter {
  explicit BinaryFormatter(std::function<uint64_t()> fn, uint16_t byteCount = 1) : _fn(fn), _len(byteCount) {}
  ~BinaryFormatter() override = default;
  QString format() const override { return u"%1"_qs.arg(_mask & _fn(), length(), 2, QChar('0')); }
  bool readOnly() const override { return false; }
  qsizetype length() const override { return 8 * _len; }

private:
  uint16_t _len = 0;
  uint64_t _mask = 0;
  std::function<uint64_t()> _fn;
};

struct MnemonicFormatter : public RegisterFormatter {
  explicit MnemonicFormatter(std::function<QString()> fn) : _fn(fn) {}
  ~MnemonicFormatter() override = default;
  QString format() const override { return _fn(); }
  bool readOnly() const override { return false; }
  qsizetype length() const override { return 7 + 2 + 3; }

private:
  std::function<QString()> _fn;
};

class Pep10_ISA final : public QObject {
  Q_OBJECT
  Q_PROPERTY(project::Environment env READ env CONSTANT)
  Q_PROPERTY(utils::Architecture architecture READ architecture CONSTANT)
  Q_PROPERTY(utils::Abstraction abstraction READ abstraction CONSTANT)
  Q_PROPERTY(QVariant delegate MEMBER _delegate NOTIFY delegateChanged)
  Q_PROPERTY(QString objectCodeText READ objectCodeText WRITE setObjectCodeText NOTIFY objectCodeTextChanged);
  Q_PROPERTY(ARawMemory *memory READ memory CONSTANT)
  Q_PROPERTY(RegisterModel *registers MEMBER _registers CONSTANT)
  Q_PROPERTY(OpcodeModel *mnemonics READ mnemonics CONSTANT)
  Q_PROPERTY(FlagModel *flags MEMBER _flags CONSTANT)
public:
  explicit Pep10_ISA(QVariant delegate, QObject *parent = nullptr);
  project::Environment env() const;
  utils::Architecture architecture() const;
  utils::Abstraction abstraction() const;
  ARawMemory *memory() const;
  OpcodeModel *mnemonics() const;
  QString objectCodeText() const;
  void setObjectCodeText(const QString &objectCodeText);
  Q_INVOKABLE static QStringListModel *modes() {
    static QStringListModel ret({"Welcome", "Edit", "Debug", "Help"});
    QQmlEngine::setObjectOwnership(&ret, QQmlEngine::CppOwnership);
    return &ret;
  }
  // Actually utils::Abstraction, but QM passes it as an int.
  Q_INVOKABLE void set(int abstraction, QString value);

signals:
  void objectCodeTextChanged();
  void delegateChanged();

private:
  QString _objectCodeText = {};
  QVariant _delegate = {};
  // Use raw pointer to avoid double-free with parent'ed QObjects.
  ArrayRawMemory *_memory = nullptr;
  RegisterModel *_registers = nullptr;
  FlagModel *_flags = nullptr;
};

// Factory to ensure class invariants of project are maintained.
// Must be a singleton to call methods on it.
// Can't seem to call Q_INVOKABLE on an uncreatable type.
class ProjectModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ _rowCount NOTIFY rowCountChanged)
public:
  enum class Roles {
    ProjectRole = Qt::UserRole + 1,
  };
  Q_ENUM(Roles);
  // Q_INVOKABLE ISAProject *isa(utils::Architecture::Value arch, project::Features features);
  explicit ProjectModel(QObject *parent = nullptr) : QAbstractListModel(parent){};
  // Helper to expose rowCount as a property to QML.
  int _rowCount() const { return rowCount({}); }
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  Q_INVOKABLE Pep10_ISA *pep10ISA(QVariant delegate);
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent,
                int destinationChild) override;
  QHash<int, QByteArray> roleNames() const override;
signals:
  void rowCountChanged(int);

private:
  std::deque<Pep10_ISA *> _projects = {};
};
