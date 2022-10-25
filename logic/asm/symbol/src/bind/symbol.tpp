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

#include <boost/algorithm/string.hpp>
#include <magic_enum.hpp>

template<typename addr_size_t>
Symbol<addr_size_t>::Symbol(const Napi::CallbackInfo &info): Napi::ObjectWrap<Symbol<addr_size_t>>(info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
    return;
  }
  try {
    auto shared = info[0].As<Napi::External<std::shared_ptr<symbol::entry<addr_size_t>>>>();
    this->_symbol = *shared.Data();
    // Unlike native symbol API, we never want value to be nullptr, otherwise returning raw value could explode.
    if (this->_symbol->value == nullptr)
      this->_symbol->value = std::make_shared<symbol::value_empty<addr_size_t>>();
  }
  catch (std::exception &e) {
    Napi::TypeError::New(env, "Expected argument to be a C++ data pointer").ThrowAsJavaScriptException();
    return;
  }
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::name(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Null();
  }
  if (!_symbol)
    return env.Undefined();
  return Napi::String::New(env, _symbol->name);
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::definition_state(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Null();
  }
  if (!_symbol)
    return env.Undefined();
  switch (_symbol->state) {
  case symbol::definition_state::kUndefined: return Napi::Number::New(env, 0);
  case symbol::definition_state::kSingle: return Napi::Number::New(env, 1);
  case symbol::definition_state::kMultiple: return Napi::Number::New(env, 2);
  case symbol::definition_state::kExternalMultiple: return Napi::Number::New(env, 3);
  }
  Napi::TypeError::New(env, "Unexpected definition state").ThrowAsJavaScriptException();
  return env.Null();
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::binding(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Null();
    if (!_symbol)
      return env.Undefined();
  }
  switch (_symbol->binding) {
  case symbol::binding_t::kGlobal: return Napi::String::New(env, "global");
  case symbol::binding_t::kImported: return Napi::String::New(env, "weak");
  case symbol::binding_t::kLocal: return Napi::String::New(env, "local");
  }
  Napi::TypeError::New(env, "Unexpected binding").ThrowAsJavaScriptException();
  return env.Null();
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::value(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  switch (_symbol->value->type()) {
  case symbol::Type::kDeleted:
  case symbol::Type::kEmpty: return env.Undefined();
  default:return Napi::BigInt::New(env, static_cast<uint64_t>(_symbol->value->value()));
  }
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::type(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::String::New(env, magic_enum::enum_name(symbol::Type::kDeleted).data());
  }
  switch (_symbol->value->type()) {
  case symbol::Type::kDeleted: return Napi::String::New(env, "notype");
  case symbol::Type::kEmpty: return Napi::String::New(env, "notype");
  case symbol::Type::kObject: return Napi::String::New(env, "object");
  case symbol::Type::kCode: return Napi::String::New(env, "func");
  }
  Napi::TypeError::New(env, "Unexpected type").ThrowAsJavaScriptException();
  return env.Null();
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::size(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  switch (_symbol->value->type()) {
  case symbol::Type::kDeleted:
  case symbol::Type::kEmpty:return env.Undefined();
  default:return Napi::BigInt::New(env, static_cast<uint64_t>(_symbol->value->size()));
  }
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::set_section_index(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsBigInt()) {
    Napi::TypeError::New(env, "Argument 1 must be a bigint").ThrowAsJavaScriptException();
  } else {
    bool lossless;
    auto bi = info[0].As<Napi::BigInt>();
    auto value = bi.Uint64Value(&lossless);
    // Hex should be  2^32 +1 represented as a u64.
    if (!lossless || value >= 0x1'0000'0000ull)
      Napi::TypeError::New(env, "BigInt must be smaller than 2^32 -1").ThrowAsJavaScriptException();
    _symbol->section_index = static_cast<uint32_t>(value);
  }
  return env.Undefined();
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::section_index(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  return Napi::BigInt::New(env, static_cast<uint64_t>(_symbol->section_index));
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::relocatable(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env, _symbol->value->relocatable());
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::symbol_index(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  return Napi::Number::New(env, (uint64_t) _symbol->value.get());
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::is_const(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env, _symbol->value->type() == symbol::Type::kConstant);
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::set_const(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsBigInt()) {
    Napi::TypeError::New(env, "Argument 1 must be a bigint").ThrowAsJavaScriptException();
  } else {
    bool lossless;
    auto u64 = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, "Argument 1 must fit in a 64 bit int").ThrowAsJavaScriptException();
    _symbol->value = std::make_shared<symbol::value_const<addr_size_t>>(u64);
  }
  return env.Undefined();
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::is_addr(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env,
                            _symbol->value->type() == symbol::Type::kCode
                                || _symbol->value->type() == symbol::Type::kObject);
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::set_addr(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 3) {
    Napi::TypeError::New(env, "Expected 3 arguments").ThrowAsJavaScriptException();
  } else if (!info[0].IsBigInt()) {
    Napi::TypeError::New(env, "Argument 1 must be a biting").ThrowAsJavaScriptException();
  } else if (!info[1].IsBigInt()) {
    Napi::TypeError::New(env, "Argument 2 must be a bigint").ThrowAsJavaScriptException();
  } else if (!info[2].IsString()) {
    Napi::TypeError::New(env, "Argument 3 must be a string").ThrowAsJavaScriptException();
  } else {
    bool lossless;
    auto base = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, "Argument 1 must fit in a 64 bit int").ThrowAsJavaScriptException();
    auto offset = info[1].As<Napi::BigInt>().Uint64Value(&lossless);
    if (!lossless)
      Napi::TypeError::New(env, "Argument 2 must fit in a 64 bit int").ThrowAsJavaScriptException();
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

    _symbol->value = std::make_shared<symbol::value_location<addr_size_t>>(base, offset, type);
  }
  return env.Undefined();
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::is_symptr(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env, _symbol->value->type() == symbol::Type::kPtrToSym);
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::set_symptr(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
  } else if (!info[0].IsObject()) {
    Napi::TypeError::New(env, "Argument 1 must be a object that is a symbol").ThrowAsJavaScriptException();
  } else {
    try {
      auto unwraped_symbol = Napi::ObjectWrap<Symbol<addr_size_t>>::Unwrap(info[0].ToObject());
      auto symbol = unwraped_symbol->raw_symbol();
      if (symbol == _symbol) {
        Napi::TypeError::New(env, "Symbol pointer creation would create a self-loop").ThrowAsJavaScriptException();
        return env.Undefined();
      }
      _symbol->value = std::make_shared<symbol::value_pointer<addr_size_t>>(unwraped_symbol->raw_symbol());
    } catch (std::exception &e) {
      Napi::TypeError::New(env, "Argument 1 must be a object that is a symbol").ThrowAsJavaScriptException();
    }
  }
  return env.Undefined();
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::is_empty(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env, _symbol->value->type() == symbol::Type::kEmpty);
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::set_empty(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
  } else {
    _symbol->value = std::make_shared<symbol::value_empty<addr_size_t>>();
  }
  return env.Undefined();
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::is_deleted(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);
  }
  return Napi::Boolean::New(env, _symbol->value->type() == symbol::Type::kDeleted);
}

template<typename addr_size_t>
Napi::Value Symbol<addr_size_t>::set_deleted(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
  } else {
    _symbol->value = std::make_shared<symbol::value_deleted<addr_size_t>>();
  }
  return env.Undefined();
}

template<typename addr_size_t>
Napi::Function Symbol<addr_size_t>::GetClass(Napi::Env env, std::string suffix) {
  return Symbol<addr_size_t>::DefineClass(env, ("Symbol." + suffix).c_str(), {
      Symbol::InstanceMethod("name", &Symbol::name),
      Symbol::InstanceMethod("definitionState", &Symbol::definition_state),
      Symbol::InstanceMethod("binding", &Symbol::binding),
      Symbol::InstanceMethod("value", &Symbol::value),
      Symbol::InstanceMethod("type", &Symbol::type),
      Symbol::InstanceMethod("size", &Symbol::size),
      Symbol::InstanceMethod("setSectionIndex", &Symbol::set_section_index),
      Symbol::InstanceMethod("sectionIndex", &Symbol::section_index),
      Symbol::InstanceMethod("relocatable", &Symbol::relocatable),
      Symbol::InstanceMethod("symbolIndex", &Symbol::symbol_index),
      Symbol::InstanceMethod("isConst", &Symbol::is_const),
      Symbol::InstanceMethod("setConst", &Symbol::set_const),
      Symbol::InstanceMethod("isAddr", &Symbol::is_addr),
      Symbol::InstanceMethod("setAddr", &Symbol::set_addr),
      Symbol::InstanceMethod("isSymPtr", &Symbol::is_symptr),
      Symbol::InstanceMethod("setSymPtr", &Symbol::set_symptr),
      Symbol::InstanceMethod("isEmpty", &Symbol::is_empty),
      Symbol::InstanceMethod("setEmpty", &Symbol::set_empty),
      Symbol::InstanceMethod("isDeleted", &Symbol::is_deleted),
      Symbol::InstanceMethod("setDeleted", &Symbol::set_deleted),
  });
}