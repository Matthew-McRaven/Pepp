#pragma once
#include "../task.hpp"

class AsmTask : public Task {
public:
  AsmTask(int ed, std::string userFname, QObject *parent = nullptr);
  void setOsFname(std::string fname);
  void setErrName(std::string fname);
  void setPepoName(std::string fname);
  void setOsListingFname(std::string fname);
  void emitElfTo(std::string fname);
  void run() override;

private:
  int ed;
  std::string userIn;
  std::optional<std::string> osIn, peplOut, elfOut, osListOut, errOut, pepoOut;
};
