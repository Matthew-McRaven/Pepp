#pragma once
#include "../task.hpp"

class GetFigTask : public Task {
public:
  GetFigTask(int ed, std::string ch, std::string fig, std::string type,
             bool isFigure, /*1 is figure, 0 is problem*/
             QObject *parent = nullptr);
  void run() override;

private:
  int ed;
  bool isFigure;
  std::string ch, fig, type;
};
