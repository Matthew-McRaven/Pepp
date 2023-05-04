#pragma once

#include <napi.h>
#include <node_api.h>

namespace bind {
Napi::Value unary_mnemonic_to_opcode(const Napi::CallbackInfo &info);
Napi::Value nonunary_mnemonic_to_opcode(const Napi::CallbackInfo &info);
} // end namespace bind;