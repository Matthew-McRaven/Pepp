#pragma once
#include "../task.hpp"

class AsmTask : public Task {
public:
  AsmTask(int ed, std::string userFname, std::string out,
          QObject *parent = nullptr);
  void setOsFname(std::string fname);
  void setOsListingFname(std::string fname);
  void emitElfTo(std::string fname);
  void run() override;

private:
  int ed;
  std::string userIn, pepoOut;
  std::optional<std::string> osIn, peplOut, elfOut, osListOut;
};
