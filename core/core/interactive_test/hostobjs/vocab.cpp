#include "./vocab.hpp"

void register_devicemgmt_words(Interpreter *interp) {
  register_value_words(interp);
  register_config_words(interp);
  register_system_words(interp);
}
