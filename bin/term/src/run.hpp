#pragma once
#include "./task.hpp"
#include <elfio/elfio.hpp>

class RunTask : public Task {
  // Task interface
public:
  RunTask(const ELFIO::elfio &elf, QObject *parent);
  void run() override;
  void setCharOut(std::string fname);
  void setCharIn(std::string fname);
  void setMemDump(std::string fname);
  void setMaxSteps(quint64 maxSteps);

private:
  const ELFIO::elfio &_elf;
  std::string _charOut, _charIn, _memDump;
  quint64 _maxSteps;
};
