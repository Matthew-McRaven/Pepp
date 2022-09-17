#pragma once

/*
    The Pep/10 suite of applications (Pep10, Pep10CPU, Pep10Term) are
    simulators for the Pep/10 virtual machine, and allow users to
    create, simulate, and debug across various levels of abstraction.

    Copyright (C) 2021 J. Stanley Warford & Matthew McRaven, Pepperdine University

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "magic_enum.hpp"
#include <boost/algorithm/string.hpp>

#include "./symbol.hpp"

template<typename addr_size_t>
Value<addr_size_t>::Value(const Napi::CallbackInfo &info): Napi::ObjectWrap<Value<addr_size_t>>(info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
    return;
  }
  try {
    auto shared = info[0].As<Napi::External<std::shared_ptr<symbol::abstract_value<addr_size_t>>>>();
    this->_value = *shared.Data();
  }
  catch (std::exception &e) {
    Napi::TypeError::New(env, "Expected argument to be a C++ data pointer").ThrowAsJavaScriptException();
    return;
  }
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::value(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  switch (_value->type()) {
  case symbol::Type::kDeleted:
  case symbol::Type::kEmpty:return env.Undefined();
  default:return Napi::Number::New(env, _value->value());
  }
}
template<typename addr_size_t>
std::shared_ptr<symbol::abstract_value<addr_size_t>> Value<addr_size_t>::raw_value() {
  return _value;
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::type(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::String::New(env, magic_enum::enum_name(symbol::Type::kDeleted).data());
  }
  return Napi::String::New(env, magic_enum::enum_name(_value->type()).data());
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::size(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  switch (_value->type()) {
  case symbol::Type::kDeleted:
  case symbol::Type::kEmpty:return env.Undefined();
  default:return Napi::Number::New(env, _value->size());
  }
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::relocatable(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env, _value->relocatable());
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::symbol_index(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  return Napi::Number::New(env, (uint64_t) _value.get());
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::is_const(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env, _value->type() == symbol::Type::kConstant);
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::set_const(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "Argument 1 must be a number").ThrowAsJavaScriptException();
  } else {
    _value =
        std::make_shared<symbol::value_const<addr_size_t>>(static_cast<addr_size_t>(info[0].ToNumber().Int64Value()));
  }
  return env.Undefined();
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::is_addr(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env, _value->type() == symbol::Type::kCode || _value->type() == symbol::Type::kObject);
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::set_addr(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 2) {
    Napi::TypeError::New(env, "Expected [2,3] argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsNumber()) {
    Napi::TypeError::New(env, "Argument 1 must be a number").ThrowAsJavaScriptException();
  } else if (!info[1].IsNumber()) {
    Napi::TypeError::New(env, "Argument 2 must be a number").ThrowAsJavaScriptException();
  } else if (!info[2].IsString()) {
    Napi::TypeError::New(env, "Argument 3 must be a string").ThrowAsJavaScriptException();
  } else {
    auto base = static_cast<addr_size_t>(info[0].ToNumber().Int64Value());
    auto offset = static_cast<addr_size_t>(info[1].ToNumber().Int64Value());
    auto type_name = info[2].ToString();

    auto type = symbol::Type::kDeleted;
    if (boost::iequals(type_name.Utf8Value(), "object"))
      type = symbol::Type::kObject;
    else if (boost::iequals(type_name.Utf8Value(), "code"))
      type = symbol::Type::kCode;
    else {
      Napi::TypeError::New(env, "Argument 3 must be a string").ThrowAsJavaScriptException();
      return env.Undefined();
    }

    _value = std::make_shared<symbol::value_location<addr_size_t>>(base, offset, type);
  }
  return env.Undefined();
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::is_symptr(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env, _value->type() == symbol::Type::kPtrToSym);
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::set_symptr(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsObject()) {
    Napi::TypeError::New(env, "Argument 1 must be a object that is a symbol").ThrowAsJavaScriptException();
  } else {
    try {
      auto unwraped_symbol = Napi::ObjectWrap<Symbol<addr_size_t>>::Unwrap(info[0].ToObject());
      _value = std::make_shared<symbol::value_pointer<addr_size_t>>(unwraped_symbol->raw_symbol());
    } catch (std::exception &e) {
      Napi::TypeError::New(env, "Argument 1 must be a object that is a symbol").ThrowAsJavaScriptException();
    }
  }
  return env.Undefined();
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::is_empty(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env, _value->type() == symbol::Type::kEmpty);
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::set_empty(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
  } else {
    _value = std::make_shared<symbol::value_empty<addr_size_t>>();
  }
  return env.Undefined();
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::is_deleted(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env, _value->type() == symbol::Type::kDeleted);
}

template<typename addr_size_t>
Napi::Value Value<addr_size_t>::set_deleted(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
  } else {
    _value = std::make_shared<symbol::value_deleted<addr_size_t>>();
  }
  return env.Undefined();
}

template<typename addr_size_t>
Napi::Function Value<addr_size_t>::GetClass(Napi::Env env, std::string suffix) {
  return Value<addr_size_t>::DefineClass(env, ("Value." + suffix).c_str(), {
      Value::InstanceMethod("value", &Value::value),
      Value::InstanceMethod("type", &Value::type),
      Value::InstanceMethod("size", &Value::size),
      Value::InstanceMethod("relocatable", &Value::relocatable),
      Value::InstanceMethod("symbolIndex", &Value::symbol_index),
      Value::InstanceMethod("isConst", &Value::is_const),
      Value::InstanceMethod("setConst", &Value::set_const),
      Value::InstanceMethod("isAddr", &Value::is_addr),
      Value::InstanceMethod("setAddr", &Value::set_addr),
      Value::InstanceMethod("isSymPtr", &Value::is_symptr),
      Value::InstanceMethod("setSymPtr", &Value::set_symptr),
      Value::InstanceMethod("isEmpty", &Value::is_empty),
      Value::InstanceMethod("setEmpty", &Value::set_empty),
      Value::InstanceMethod("isDeleted", &Value::is_deleted),
      Value::InstanceMethod("setDeleted", &Value::set_deleted),

  });
}