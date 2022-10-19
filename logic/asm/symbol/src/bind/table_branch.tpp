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

#include "./symbol.hpp"

template<typename addr_size_t>
BranchTable<addr_size_t>::BranchTable(const Napi::CallbackInfo &info): Napi::ObjectWrap<BranchTable<addr_size_t>>(
    info) {
  Napi::Env env = info.Env();
  if (info.Length() > 1) {
    Napi::TypeError::New(env, "Expected at most one argument").ThrowAsJavaScriptException();
    return;
  }
  if (info.Length() == 1) {
    if (!info[0].IsExternal()) {
      Napi::TypeError::New(env, "First argument must be an external object").ThrowAsJavaScriptException();
      return;
    }
    try {
      // Treat argument 0 as a raw pointer to one of our shared pointers to our target table, then grab the pointer, and dereference it.
      // This allows me to pass raw C++ objects as javascript arguments, making the API easier to design.
      this->table = *info[0].As<Napi::External<std::shared_ptr<symbol::BranchTable<addr_size_t>>>>().Data();
    } catch (std::exception &e) {
      Napi::TypeError::New(env, "First argument must be a symbol table").ThrowAsJavaScriptException();
      return;
    }
  } else
    this->table = std::make_shared<symbol::BranchTable<addr_size_t>>();
}

template<typename addr_size_t>
Napi::Value BranchTable<addr_size_t>::add_leaf(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected at 0 arguments").ThrowAsJavaScriptException();
    return env.Null();
  }
  auto child = symbol::insert_leaf(this->table);
  auto retVal = LeafTable<addr_size_t>::GetClass(env, ADDR_SUFFIX).New(
      {Napi::External<std::shared_ptr<symbol::LeafTable<addr_size_t>>>::New(env, &child)});
  return retVal;
}

template<typename addr_size_t>
Napi::Value BranchTable<addr_size_t>::add_branch(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected at 0 arguments").ThrowAsJavaScriptException();
    return env.Null();
  }
  auto child = symbol::insert_branch(this->table);
  auto retVal = BranchTable<addr_size_t>::GetClass(env, ADDR_SUFFIX).New(
      {Napi::External<std::shared_ptr<symbol::BranchTable<addr_size_t>>>::New(env, &child)});
  return retVal;
}

template<typename addr_size_t>
Napi::Value BranchTable<addr_size_t>::table_index(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  return Napi::Number::New(env, (uint64_t) this->table.get());
}

template<typename addr_size_t>
Napi::Value BranchTable<addr_size_t>::parent(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  auto root = symbol::parent<addr_size_t>({this->table});
  if (std::holds_alternative<std::shared_ptr<symbol::LeafTable<addr_size_t>>>(root)) {
    auto as_leaf = std::get<std::shared_ptr<symbol::LeafTable<addr_size_t>>>(root);
    return LeafTable<addr_size_t>::GetClass(env, ADDR_SUFFIX).New(
        {Napi::External<std::shared_ptr<symbol::LeafTable<addr_size_t>>>::New(env, &as_leaf)});
  } else {
    auto as_branch = std::get<std::shared_ptr<symbol::BranchTable<addr_size_t>>>(root);
    return BranchTable<addr_size_t>::GetClass(env, ADDR_SUFFIX).New(
        {Napi::External<std::shared_ptr<symbol::BranchTable<addr_size_t>>>::New(env, &as_branch)});
  }
}

template<typename addr_size_t>
Napi::Value BranchTable<addr_size_t>::rootTable(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  auto root = symbol::root_table<addr_size_t>({this->table});
  if (std::holds_alternative<std::shared_ptr<symbol::LeafTable<addr_size_t>>>(root)) {
    auto as_leaf = std::get<std::shared_ptr<symbol::LeafTable<addr_size_t>>>(root);
    return LeafTable<addr_size_t>::GetClass(env, ADDR_SUFFIX).New(
        {Napi::External<std::shared_ptr<symbol::LeafTable<addr_size_t>>>::New(env, &as_leaf)});
  } else {
    auto as_branch = std::get<std::shared_ptr<symbol::BranchTable<addr_size_t>>>(root);
    return BranchTable<addr_size_t>::GetClass(env, ADDR_SUFFIX).New(
        {Napi::External<std::shared_ptr<symbol::BranchTable<addr_size_t>>>::New(env, &as_branch)});
  }
}

template<typename addr_size_t>
Napi::Value BranchTable<addr_size_t>::exists(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || info.Length() > 2) {
    Napi::TypeError::New(env, "Expected  [1-2] arguments").ThrowAsJavaScriptException();
    return env.Null();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
    return env.Null();
  } else if (info.Length() == 2 && !info[1].IsNumber()) {
    Napi::TypeError::New(env,
                         "Second argument must be a number (see TraversalPolicy)").ThrowAsJavaScriptException();
    return env.Null();
  }

  std::string name = info[0].As<Napi::String>().Utf8Value();
  // No bounds checking on traversal policies, that's the algorithm's job.
  auto policy = symbol::TraversalPolicy::kChildren;
  if (info.Length() == 2)
    policy = static_cast<symbol::TraversalPolicy>(info[1].ToNumber().Int32Value());
  return Napi::Boolean::New(env, symbol::exists<addr_size_t>({this->table}, name, policy));
}

template<typename addr_size_t>
Napi::Value BranchTable<addr_size_t>::enumerate_symbols(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() > 1) {
    Napi::TypeError::New(env, "Expected [0,1] arguments").ThrowAsJavaScriptException();
    return env.Null();
  } else if (info.Length() == 1 && !info[0].IsNumber()) {
    Napi::TypeError::New(env, "First argument must be a number").ThrowAsJavaScriptException();
    return env.Null();
  }

  // No bounds checking on traversal policies, that's the algorithm's job.
  auto policy = symbol::TraversalPolicy::kChildren;
  if (info.Length() == 1)
    policy = static_cast<symbol::TraversalPolicy>(info[0].ToNumber().Int32Value());

  auto list = symbol::enumerate_symbols<addr_size_t>(this->table, policy);
  auto array = Napi::Array::New(env, list.size());
  uint64_t idx = 0;
  for (auto &item : list) {
    auto sym = Symbol<addr_size_t>::GetClass(env, ADDR_SUFFIX).New(
        {Napi::External<std::shared_ptr<symbol::entry<addr_size_t>>>::New(env, &item)});
    array[idx++] = sym;
  }
  return array;
}

template<typename addr_size_t>
Napi::Value BranchTable<addr_size_t>::listing(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() > 1) {
    Napi::TypeError::New(env, "Expected [0,1] arguments").ThrowAsJavaScriptException();
    return env.Null();
  } else if (info.Length() == 1 && !info[0].IsNumber()) {
    Napi::TypeError::New(env, "First argument must be a number").ThrowAsJavaScriptException();
    return env.Null();
  }

  // No bounds checking on traversal policies, that's the algorithm's job.
  auto policy = symbol::TraversalPolicy::kChildren;
  if (info.Length() == 1)
    policy = static_cast<symbol::TraversalPolicy>(info[0].ToNumber().Int32Value());

  auto listing = symbol::symbol_table_listing<addr_size_t>(this->table, policy);
  return Napi::String::New(env, listing);
}

template<typename addr_size_t>
Napi::Value BranchTable<addr_size_t>::find(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || info.Length() > 2) {
    Napi::TypeError::New(env, "Expected at [1,2] arguments").ThrowAsJavaScriptException();
    return env.Null();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
    return env.Null();
  } else if (info.Length() == 2 && !info[1].IsNumber()) {
    Napi::TypeError::New(env,
                         "Second argument must be a number (see TraversalPolicy)").ThrowAsJavaScriptException();
    return env.Null();
  }

  // No bounds checking on traversal policies, that's the algorithm's job.
  auto policy = symbol::TraversalPolicy::kChildren;
  if (info.Length() == 1)
    policy = static_cast<symbol::TraversalPolicy>(info[1].ToNumber().Int32Value());

  auto symbols = symbol::select_by_name<addr_size_t>(this->table, info[0].ToString().Utf8Value(), policy);
  auto array = Napi::Array::New(env, symbols.size());
  uint64_t idx = 0;
  for (auto &item : symbols) {
    auto sym = Symbol<addr_size_t>::GetClass(env, ADDR_SUFFIX).New(
        {Napi::External<std::shared_ptr<symbol::entry<addr_size_t>>>::New(env, &item)});
    array[idx++] = sym;
  }
  return array;
}

template<typename addr_size_t>
Napi::Function BranchTable<addr_size_t>::GetClass(Napi::Env env, std::string suffix) {
  return BranchTable<addr_size_t>::DefineClass(env, ("BranchTable" + suffix).c_str(), {
      BranchTable::InstanceMethod("addBranch", &BranchTable::add_branch),
      BranchTable::InstanceMethod("addLeaf", &BranchTable::add_leaf),
      BranchTable::InstanceMethod("tableIndex", &BranchTable::table_index),
      BranchTable::InstanceMethod("parent", &BranchTable::parent),
      BranchTable::InstanceMethod("rootTable", &BranchTable::rootTable),
      BranchTable::InstanceMethod("exists", &BranchTable::exists),
      BranchTable::InstanceMethod("enumerateSymbols", &BranchTable::enumerate_symbols),
      BranchTable::InstanceMethod("listing", &BranchTable::listing),
      BranchTable::InstanceMethod("find", &BranchTable::find),
  });
}