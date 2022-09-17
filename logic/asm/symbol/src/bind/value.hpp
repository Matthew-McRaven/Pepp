#pragma once

/*
    The Pep/10 suite of applications (Pep10, Pep10CPU, Pep10Term) are
    simulators for the Pep/10 virtual machine, and allow users to
    create, simulate, and debug across various levels of abstraction.

    Copyright (C) 2022 J. Stanley Warford & Matthew McRaven, Pepperdine University

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

#include <napi.h>
#include <node_api.h>

#include "symbol/value.hpp"

template<typename addr_size_t>
class Value : public Napi::ObjectWrap<Value<addr_size_t>> {
public:
  explicit Value(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env, std::string suffix);

  Napi::Value value(const Napi::CallbackInfo &info);
  std::shared_ptr<symbol::abstract_value<addr_size_t>> raw_value();
  Napi::Value type(const Napi::CallbackInfo &info);
  Napi::Value size(const Napi::CallbackInfo &info);
  Napi::Value relocatable(const Napi::CallbackInfo &info);
  // If two symbol values share a symbolIndex, then they point to the same underlying symbol.
  Napi::Value symbol_index(const Napi::CallbackInfo &info);

  Napi::Value is_const(const Napi::CallbackInfo &);
  Napi::Value set_const(const Napi::CallbackInfo &info);

  Napi::Value is_addr(const Napi::CallbackInfo &);
  // Valid strings for arg 3 are "code" and "object"
  Napi::Value set_addr(const Napi::CallbackInfo &info);
  Napi::Value offset(const Napi::CallbackInfo &info);
  Napi::Value base(const Napi::CallbackInfo &info);

  Napi::Value is_symptr(const Napi::CallbackInfo &info);
  Napi::Value set_symptr(const Napi::CallbackInfo &info);

  Napi::Value is_empty(const Napi::CallbackInfo &info);
  Napi::Value set_empty(const Napi::CallbackInfo &info);
  Napi::Value is_deleted(const Napi::CallbackInfo &info);
  Napi::Value set_deleted(const Napi::CallbackInfo &info);
private:
  std::shared_ptr<symbol::abstract_value<addr_size_t>> _value;
};

#include "./value.tpp"