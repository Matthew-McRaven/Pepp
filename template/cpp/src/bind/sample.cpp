#include "./sample.hpp"

namespace bind {

  TestWrapper::TestWrapper(const Napi::CallbackInfo &info): Napi::ObjectWrap<TestWrapper>(info) {

  }
  Napi::Function TestWrapper::GetClass(Napi::Env env) {
    return bind::TestWrapper::DefineClass(env, "Test", {
        TestWrapper::InstanceMethod("foo", &TestWrapper::foo),
    });
  }

  Napi::Value TestWrapper::foo(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    return Napi::Number::New(env, this->_test.foo());
  }
}
