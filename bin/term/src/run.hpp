#pragma once
#include "./task.hpp"
#include <elfio/elfio.hpp>

class RunTask : public Task {
  // Task interface
public:
  RunTask(int ed, std::string fname, QObject *parent);
  bool loadToElf();
  void run() override;
  void setCharOut(std::string fname);
  void setCharIn(std::string fname);
  void setMemDump(std::string fname);
  void setMaxSteps(quint64 maxSteps);
  void setOsIn(std::string fname);

private:
  int _ed;
  std::string _objIn;
  QSharedPointer<ELFIO::elfio> _elf = nullptr;
  std::string _charOut, _charIn, _memDump;
  quint64 _maxSteps;
  std::optional<std::string> _osIn;
};
