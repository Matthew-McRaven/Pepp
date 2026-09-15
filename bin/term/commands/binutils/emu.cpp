#include "emu.hpp"

PeppEmulator::PeppEmulator(Options &opts, QObject *parent) : Task(parent), _opts(opts) {}

void PeppEmulator::run() { return emit finished(0); }
