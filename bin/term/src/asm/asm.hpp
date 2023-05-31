#pragma once
#include "../task.hpp"

class AsmTask : public Task {
public:
  AsmTask(int ed, std::string userFname, QObject *parent = nullptr);
  void setBm(bool forceBm);
  void setOsFname(std::string fname);
  void setErrName(std::string fname);
  void setPepoName(std::string fname);
  void setOsListingFname(std::string fname);
  void emitElfTo(std::string fname);
  void setMacroDirs(std::list<std::string> dirs);
  void run() override;

private:
  int ed;
  std::string userIn;
  bool forceBm = false;
  std::optional<std::string> osIn, peplOut, elfOut, osListOut, errOut, pepoOut;
  std::list<std::string> macroDirs;
};
