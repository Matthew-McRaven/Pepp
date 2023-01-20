#pragma once

#include <napi.h>
#include <node_api.h>
#include "sim/trace/ztb.hpp"
namespace bind {

class TestWrapper : public Napi::ObjectWrap<TestWrapper> {
public:
  // Takes 2 args, 1st is std::shared_ptr<ELFIO::elf>, 2nd is a Section.
  TestWrapper(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env);

  Napi::Value foo(const Napi::CallbackInfo &info);
private:
  sim::trace::ZTB _test;
};
}
