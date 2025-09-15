#include "microasm.hpp"
#include "toolchain2/parser/ucode/pep_common.hpp"
#include "toolchain2/targets/pep/uarch.hpp"

MicroAsmTask::MicroAsmTask(int ed, std::string in, int busWidth, QObject *parent)
    : Task(parent), ed(ed), busWidth(busWidth), in(in) {}

void MicroAsmTask::setErrName(std::string fname) { errOut = fname; }

void MicroAsmTask::setOutCPUName(std::string fname) { pepcpuOut = fname; }

void MicroAsmTask::run() {
  using uarch1 = pepp::tc::arch::Pep9ByteBus;
  using uarch2 = pepp::tc::arch::Pep9WordBus;
  using regs = pepp::tc::arch::Pep9Registers;
  QString source;
  {
    QFile sIn(QString::fromStdString(this->in)); // auto-closes
    if (!sIn.exists()) {
      std::cerr << "Source file does not exist.\n";
      return emit finished(3);
    }
    sIn.open(QIODevice::ReadOnly | QIODevice::Text);
    source = sIn.readAll();
    sIn.close();
  }

  pepp::tc::parse::Errors errors;
  QStringList formatted;
  if (busWidth == 1) {
    auto result = pepp::tc::parse::MicroParser<uarch1, regs>(source).parse();
    if (!result.errors.empty()) errors = result.errors;
    else
      for (const auto &line : result.program) formatted.append(pepp::tc::ir::format<uarch1, regs>(line));
  } else if (busWidth == 2) {
    auto result = pepp::tc::parse::MicroParser<uarch2, regs>(source).parse();
    if (!result.errors.empty()) errors = result.errors;
    else
      for (const auto &line : result.program) formatted.append(pepp::tc::ir::format<uarch2, regs>(line));

  } else {
    std::cerr << "Invalid bus width :" << busWidth << std::endl;
    return emit finished(4);
  }

  if (!errors.empty()) {
    std::cerr << "Assembly failed, check error log" << std::endl;
    QString errFName;
    if (errOut) {
      errFName = QString::fromStdString(*errOut);
    } else {
      QFileInfo err(QString::fromStdString(this->in));
      errFName = err.path() + "/" + err.completeBaseName() + ".err.txt";
    }
    QFile errF(errFName);
    if (errF.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
      auto ts = QTextStream(&errF);
      for (auto const &[line, message] : errors) ts << line << ": " << message.trimmed() << "\n";
    } else {
      std::cerr << "Failed to open error log for writing: " << errFName.toStdString() << std::endl;
      for (auto const &[line, message] : errors) qWarning().noquote().nospace() << line << ": " << message.trimmed();
    }

    return emit finished(6);
  }

  QString pepcpuFName;
  if (pepcpuOut) {
    pepcpuFName = QString::fromStdString(*pepcpuOut);
    QFile pepcpuF(pepcpuFName);
    if (pepcpuF.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
      for (const auto &line : std::as_const(formatted)) QTextStream(&pepcpuF) << line << "\n";
    } else std::cerr << "Failed to open microcode for writing: " << pepcpuFName.toStdString() << std::endl;
  }

  return emit finished(0);
}
