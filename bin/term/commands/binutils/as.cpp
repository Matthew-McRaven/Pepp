#include "as.hpp"
#include <iostream>

AsTask::AsTask(Options &opts, QObject *parent) : Task(parent), _opts(opts) {}

void AsTask::run() {
  std::cout << "No-operation!\n";
  return emit finished(0);
}
