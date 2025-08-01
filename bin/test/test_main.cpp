#include <QCoreApplication>
#include "config.hpp"
int main(int argc, char *argv[]) {
  QCoreApplication ap(argc, argv);
  return Catch::Session().run(argc, argv);
}
