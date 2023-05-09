#pragma once
#include "../task.hpp"

class GetMacroTask : public Task {
public:
  GetMacroTask(int ed, std::string name, QObject *parent = nullptr);
  void run() override;

private:
  int ed;
  std::string name;
};
