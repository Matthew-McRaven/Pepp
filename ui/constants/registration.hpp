#pragma once
#include <QObject>
namespace constants {

class Abstraction : public QObject {
  Q_GADGET
public:
  Abstraction(QObject *parent = nullptr) : QObject(parent) {}
  enum Value {
    // LG1 = 1,
    MC2 = 2,
    ISA3 = 3,
    OS4 = 4,
    ASMB5 = 5,
    // HOL6 = 6,
    // APP7 = 7,
  };
  Q_ENUM(Value);
};

class Architecture : public QObject {
  Q_GADGET
public:
  Architecture(QObject *parent = nullptr) : QObject(parent) {}
  enum Value {
    // Pep8 = 8,
    // Pep9 = 9,
    Pep10 = 10,
    RISCV32I = 128,
  };
  Q_ENUM(Value);
};

void registerTypes(const char *uri);
}; // namespace constants
