#pragma once
#include "../task.hpp"

class GetFigTask : public Task {
public:
  GetFigTask(int ed, std::string ch, std::string fig, std::string type,
             QObject *parent = nullptr);
  void run() override;

private:
  int ed;
  std::string ch, fig, type;
};
