#pragma once
#include "./task.hpp"

class ListTask : public Task {
public:
  ListTask(int ed, QObject *parent = nullptr);
  void run() override;

private:
  int ed;
};

