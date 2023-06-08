#include "./about.hpp"
#include "about/version.hpp"
#include <iostream>

AboutTask::AboutTask(QObject *parent) : Task(parent) {}

void AboutTask::run() {
  std::cout << "placeholder\n";
  return emit finished(0);
}
