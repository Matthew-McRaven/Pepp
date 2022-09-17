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

#include <napi.h>
#include <node_api.h>

#include "symbol/table.hpp"

template<typename addr_size_t>
class BranchTable : public Napi::ObjectWrap<BranchTable<addr_size_t>> {
public:
  explicit BranchTable(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env, std::string suffix);
  Napi::Value add_leaf(const Napi::CallbackInfo &info);
  Napi::Value add_branch(const Napi::CallbackInfo &info);
  Napi::Array children(const Napi::CallbackInfo &info);

  // Helpers
  Napi::Value table_index(const Napi::CallbackInfo &info);
  Napi::Value parent(const Napi::CallbackInfo &info);

  // Visitors
  // name, policy.
  Napi::Value rootTable(const Napi::CallbackInfo &info);
  // name, policy.
  Napi::Value exists(const Napi::CallbackInfo &info);
  // policy.
  Napi::Value enumerate_symbols(const Napi::CallbackInfo &info);
  // policy.
  Napi::Value listing(const Napi::CallbackInfo &info);

private:
  std::shared_ptr<symbol::BranchTable<addr_size_t>> table;
};

template<typename addr_size_t>
class LeafTable : public Napi::ObjectWrap<LeafTable<addr_size_t>> {
public:
  explicit LeafTable(const Napi::CallbackInfo &info);
  static Napi::Function GetClass(Napi::Env env, std::string suffix);

  // Accessors
  Napi::Value get(const Napi::CallbackInfo &info);
  Napi::Value reference(const Napi::CallbackInfo &info);
  Napi::Value define(const Napi::CallbackInfo &info);
  Napi::Value mark_global(const Napi::CallbackInfo &info);

  // Helpers
  Napi::Value table_index(const Napi::CallbackInfo &info);
  Napi::Value parent(const Napi::CallbackInfo &info);
  Napi::Value entries(const Napi::CallbackInfo &info);

  // Visitors
  // name, policy.
  Napi::Value rootTable(const Napi::CallbackInfo &info);
  // name, policy.
  Napi::Value exists(const Napi::CallbackInfo &info);
  // policy.
  Napi::Value enumerate_symbols(const Napi::CallbackInfo &info);
  // policy.
  Napi::Value listing(const Napi::CallbackInfo &info);

private:
  std::shared_ptr<symbol::LeafTable<addr_size_t>> table;
};

#include "./table_branch.tpp"
#include "./table_leaf.tpp"