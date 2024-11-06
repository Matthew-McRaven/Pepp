#include <QCoreApplication>
#include <catch.hpp>
int main(int argc, char *argv[]) {
  QCoreApplication ap(argc, argv);
  return Catch::Session().run(argc, argv);
}
