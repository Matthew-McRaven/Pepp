#pragma once
#include <QQmlEngine>
#include <QStringListModel>
#include <deque>
#include <qabstractitemmodel.h>
#include <targets/pep10/isa3/system.hpp>
#include "cpu/registermodel.hpp"
#include "cpu/statusbitmodel.hpp"
#include "memory/hexdump/rawmemory.hpp"
#include "utils/constants.hpp"
#include "utils/opcodemodel.hpp"

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

class DebugEnableFlags : public QObject {
  Q_OBJECT
public:
  explicit DebugEnableFlags(QObject *parent = nullptr);
  enum Value {
    Start = 1,
    Continue = 2,
    Pause = 4,
    Stop = 8,
    LoadObject = 16,
    Execute = 32,
  };
  Q_ENUM(Value);
};

class StepEnableFlags : public QObject {
  Q_OBJECT
public:
  explicit StepEnableFlags(QObject *parent = nullptr);
  enum Value {
    Step = 1,
    StepOver = 2,
    StepInto = 4,
    StepOut = 8,
    StepBack = 16,
    StepBackOver = 32,
    StepBackInto = 64,
    StepBackOut = 128,
  };
  Q_ENUM(Value);
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
      : _fn(fn), _bytes(byteCount), _mask(mask(byteCount)) {}
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
  explicit UnsignedDecFormatter(std::function<std::uint64_t()> fn, uint16_t byteCount = 2)
      : _fn(fn), _bytes(byteCount), _mask(mask(byteCount)) {
    auto maxNum = std::pow(2, 8 * byteCount);
    auto digits = std::ceil(std::log10(maxNum));
    _len = digits;
  }
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
      : _fn(fn), _bytes(byteCount), _mask(mask(byteCount)) {
    auto maxNum = std::pow(2, 8 * byteCount);
    auto digits = std::ceil(std::log10(maxNum));
    _len = digits + 1;
  }
  ~SignedDecFormatter() override = default;
  QString format() const override {
    if (auto v = _fn(); v < 0)
      // Limit V to the range expressable with an N-bit unsigned int before restoring the sign.
      return QString::number(~(_mask & ~v));
    return QString::number(_mask & _fn());
  }
  bool readOnly() const override { return false; }
  qsizetype length() const override { return _len; }

private:
  uint16_t _bytes = 0, _len = 0;
  uint64_t _mask = 0;
  std::function<int64_t()> _fn;
};

struct BinaryFormatter : public RegisterFormatter {
  explicit BinaryFormatter(std::function<uint64_t()> fn, uint16_t byteCount = 1)
      : _fn(fn), _len(byteCount), _mask(mask(byteCount)) {}
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
  bool readOnly() const override { return true; }
  qsizetype length() const override { return 7 + 2 + 3; }

private:
  std::function<QString()> _fn;
};

struct OptionalFormatter : public RegisterFormatter {
  explicit OptionalFormatter(QSharedPointer<RegisterFormatter> fmt, std::function<bool()> valid)
      : _fmt(fmt), _valid(valid) {}
  ~OptionalFormatter() override = default;
  QString format() const override { return _valid() ? _fmt->format() : ""; }
  bool readOnly() const override { return _fmt->readOnly(); }
  qsizetype length() const override { return _fmt->length(); }

private:
  QSharedPointer<RegisterFormatter> _fmt;
  std::function<bool()> _valid;
};
class Pep10_ISA final : public QObject {
  Q_OBJECT
  Q_PROPERTY(project::Environment env READ env CONSTANT)
  Q_PROPERTY(utils::Architecture architecture READ architecture CONSTANT)
  Q_PROPERTY(utils::Abstraction abstraction READ abstraction CONSTANT)
  Q_PROPERTY(QVariant delegate MEMBER _delegate NOTIFY delegateChanged)
  Q_PROPERTY(QString objectCodeText READ objectCodeText WRITE setObjectCodeText NOTIFY objectCodeTextChanged);
  Q_PROPERTY(ARawMemory *memory READ memory CONSTANT)
  // Preserve the current address in the memory dump pane on tab-switch.
  Q_PROPERTY(quint16 currentAddress MEMBER _currentAddress NOTIFY currentAddressChanged)
  Q_PROPERTY(RegisterModel *registers MEMBER _registers CONSTANT)
  Q_PROPERTY(OpcodeModel *mnemonics READ mnemonics CONSTANT)
  Q_PROPERTY(FlagModel *flags MEMBER _flags CONSTANT)
  Q_PROPERTY(int allowedDebugging READ allowedDebugging NOTIFY allowedDebuggingChanged)
  Q_PROPERTY(int allowedSteps READ allowedSteps NOTIFY allowedStepsChanged)
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
  Q_INVOKABLE int allowedDebugging() const;
  Q_INVOKABLE int allowedSteps() const;
public slots:
  bool onSaveCurrent();
  bool onLoadObject();
  bool onExecute();
  bool onDebuggingStart();
  bool onDebuggingContinue();
  bool onDebuggingPause();
  bool onDebuggingStop();
  bool onISARemoveAllBreakpoints();
  bool onISAStep();
  bool onISAStepOver();
  bool onISAStepInto();
  bool onISAStepOut();

  bool onClearCPU();
  bool onClearMemory();

signals:
  void objectCodeTextChanged();
  void delegateChanged();
  void currentAddressChanged();
  void allowedDebuggingChanged();
  void allowedStepsChanged();

  void beginResetModel();
  void endResetModel();

private:
  QString _objectCodeText = {};
  QVariant _delegate = {};
  QSharedPointer<targets::pep10::isa::System> _system = {};
  QSharedPointer<ELFIO::elfio> _elf = {};
  // Use raw pointer to avoid double-free with parent'ed QObjects.
  ArrayRawMemory *_memory = nullptr;
  RegisterModel *_registers = nullptr;
  FlagModel *_flags = nullptr;
  qint16 _currentAddress = 0;
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
