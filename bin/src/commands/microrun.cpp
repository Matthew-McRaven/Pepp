#include "microrun.hpp"
#include <QtCore>
#include "sim/api2/device.hpp"
#include "sim/api2/memory/address.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep9/mc2/cpu.hpp"

MicroRunTask::MicroRunTask(int ed, std::string fname, int busWidth, QObject *parent)
    : Task(parent), _ed(ed), _busWidth(busWidth), _pecpuIn(fname) {}

void MicroRunTask::setUnitTests(std::string fname) { _unitTest = fname; }

void MicroRunTask::setErrName(std::string fname) { _errName = fname; }

void MicroRunTask::run() {
  using namespace Qt::StringLiterals;
  using uarch1 = pepp::tc::arch::Pep9ByteBus;
  using uarch2 = pepp::tc::arch::Pep9WordBus;
  using regs = pepp::tc::arch::Pep9Registers;

  sim::api2::device::ID id = 0;
  auto nextID = [&id]() { return id++; };
  auto desc_mem =
      sim::api2::device::Descriptor{.id = nextID(), .compatible = nullptr, .baseName = "dev", .fullName = "/dev"};
  auto span = sim::api2::memory::AddressSpan<quint16>(0, 0xffff);
  auto desc_cpu =
      sim::api2::device::Descriptor{.id = nextID(), .compatible = nullptr, .baseName = "cpu", .fullName = "/cpu"};
  sim::memory::Dense<quint16> mem(desc_mem, span, 0);

  // Load pepcpu source code and unit tests
  QString source_text, unit_test_text;
  {
    QFile f(QString::fromStdString(_pecpuIn)); // auto-closes
    if (!f.exists()) {
      std::cerr << "Source file does not exist.\n";
      return emit finished(3);
    }
    f.open(QIODevice::ReadOnly | QIODevice::Text);
    source_text = f.readAll();
  }

  if (_unitTest) {
    QFile f(QString::fromStdString(*_unitTest)); // auto-closes
    if (!f.exists()) {
      std::cerr << "Test file does not exist.\n";
      return emit finished(3);
    }
    f.open(QIODevice::ReadOnly | QIODevice::Text);
    unit_test_text = f.readAll();
  } else unit_test_text = source_text;

  QStringList errors;
  if (_busWidth == 1) {
    auto cpu = targets::pep9::mc2::CPUByteBus(desc_cpu, nextID);
    cpu.setTarget(&mem, nullptr);
    cpu.setConstantRegisters();
    auto code_asm = pepp::tc::parse::MicroParser<uarch1, regs>(source_text).parse();
    auto test_asm = pepp::tc::parse::MicroParser<uarch1, regs>(unit_test_text).parse();
    if (!code_asm.errors.empty()) {
      std::cerr << "Assembly failed, check error log" << std::endl;
      errors.append("Source assembly failed with errors:");
      for (auto const &[line, message] : code_asm.errors) errors.append(u"%1: %2"_s.arg(line).arg(message.trimmed()));
    } else if (!test_asm.errors.empty()) {
      std::cerr << "Assembly failed, check error log" << std::endl;
      errors.append("Test assembly failed with errors:");
      for (auto const &[line, message] : test_asm.errors) errors.append(u"%1: %2"_s.arg(line).arg(message.trimmed()));
    } else {
      auto mc = pepp::tc::parse::microcodeFor<uarch1, regs>(code_asm);
      auto test = pepp::tc::parse::tests<uarch1, regs>(test_asm);
      cpu.setMicrocode(std::move(mc));
      cpu.applyPreconditions(test.pre);
      for (int cycle = 1; cpu.status() != targets::pep9::mc2::CPUByteBus::Status::Halted;) cpu.clock(cycle++);
      if (cpu.status() != targets::pep9::mc2::CPUByteBus::Status::Halted) {
        std::cerr << "CPU did not halt after running unit tests." << std::endl;
        errors.append("Step failed with error code: " + QString::number((int)cpu.status()));
      }
      auto success = cpu.testPostconditions(test.post);
      auto passed = std::all_of(success.cbegin(), success.cend(), [](const bool &test) { return test; });
      if (!passed) {
        std::cerr << "Postconditions violated, check error log" << std::endl;
        for (int it = 0; it < success.size(); it++)
          if (!success[it]) errors.append("Failed test: " + toString(test.post[it]));
      }
    }
  } else if (_busWidth == 2) {
    auto cpu = targets::pep9::mc2::CPUWordBus(desc_cpu, nextID);
    cpu.setTarget(&mem, nullptr);
    cpu.setConstantRegisters();
    auto code_asm = pepp::tc::parse::MicroParser<uarch2, regs>(source_text).parse();
    auto test_asm = pepp::tc::parse::MicroParser<uarch2, regs>(unit_test_text).parse();
    if (!code_asm.errors.empty()) {
      std::cerr << "Assembly failed, check error log" << std::endl;
      errors.append("Source assembly failed with errors:");
      for (auto const &[line, message] : code_asm.errors) errors.append(u"%1: %2"_s.arg(line).arg(message.trimmed()));
    } else if (!test_asm.errors.empty()) {
      std::cerr << "Assembly failed, check error log" << std::endl;
      errors.append("Test assembly failed with errors:");
      for (auto const &[line, message] : test_asm.errors) errors.append(u"%1: %2"_s.arg(line).arg(message.trimmed()));
    } else {
      auto mc = pepp::tc::parse::microcodeFor<uarch2, regs>(code_asm);
      auto test = pepp::tc::parse::tests<uarch2, regs>(test_asm);
      cpu.setMicrocode(std::move(mc));
      cpu.applyPreconditions(test.pre);
      for (int cycle = 1; cpu.status() != targets::pep9::mc2::CPUWordBus::Status::Halted;) cpu.clock(cycle++);
      if (cpu.status() != targets::pep9::mc2::CPUWordBus::Status::Halted) {
        std::cerr << "CPU did not halt after running unit tests." << std::endl;
        errors.append("Step failed with error code: " + QString::number((int)cpu.status()));
      }
      auto success = cpu.testPostconditions(test.post);
      auto passed = std::all_of(success.cbegin(), success.cend(), [](const bool &test) { return test; });
      if (!passed) {
        std::cerr << "Postconditions violated, check error log" << std::endl;
        for (int it = 0; it < success.size(); it++)
          if (!success[it]) errors.append("Failed test: " + toString(test.post[it]));
      }
    }
  } else {
    std::cerr << "Invalid bus width :" << _busWidth << std::endl;
    return emit finished(4);
  }

  if (!errors.empty()) {
    QString errFName;
    if (_errName) {
      errFName = QString::fromStdString(*_errName);
    } else {
      QFileInfo err(QString::fromStdString(this->_pecpuIn));
      errFName = err.path() + "/" + err.completeBaseName() + ".err.txt";
    }
    QFile errF(errFName);
    if (errF.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
      auto ts = QTextStream(&errF);
      for (auto const &message : errors) ts << message.trimmed() << "\n";
    } else {
      std::cerr << "Failed to open error log for writing: " << errFName.toStdString() << std::endl;
      for (auto const &message : errors) qWarning().noquote().nospace() << message.trimmed();
    }
    return emit finished(6);
  }

  return emit finished(0);
}
