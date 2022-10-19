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

template<typename addr_size_t>
LeafTable<addr_size_t>::LeafTable(const Napi::CallbackInfo &info): Napi::ObjectWrap<LeafTable<addr_size_t>>(info) {
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
      this->table = *info[0].As<Napi::External<std::shared_ptr<symbol::LeafTable<addr_size_t>>>>().Data();
    } catch (std::exception &e) {
      Napi::TypeError::New(env, "First argument must be a symbol table").ThrowAsJavaScriptException();
      return;
    }
  } else
    this->table = std::make_shared<symbol::LeafTable<addr_size_t>>();
}

template<typename addr_size_t>
Napi::Value LeafTable<addr_size_t>::get(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Perform type checking on arguments, and coerce to C++ string type.
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
    return env.Null();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::String name = info[0].As<Napi::String>();
  std::string name_as_str = name.Utf8Value();

  // Perform operation, and stuff native C++ object into wrapped object.
  auto sym = table->get(name_as_str);
  if (!sym)
    return env.Null();
  auto x = Symbol<addr_size_t>::GetClass(env, ADDR_SUFFIX).New(
      {Napi::External<std::shared_ptr<symbol::entry<addr_size_t>>>::New(env, &*sym)});
  return x;
}

template<typename addr_size_t>
Napi::Value LeafTable<addr_size_t>::reference(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Perform type checking on arguments, and coerce to C++ string type.
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
    return env.Null();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::String name = info[0].As<Napi::String>();
  std::string name_as_str = name.Utf8Value();

  // Perform operation, and stuff native C++ object into wrapped object.
  auto sym = table->reference(name_as_str);
  auto x = Symbol<addr_size_t>::GetClass(env, ADDR_SUFFIX).New(
      {Napi::External<std::shared_ptr<symbol::entry<addr_size_t>>>::New(env, &sym)});
  return x;
}

template<typename addr_size_t>
Napi::Value LeafTable<addr_size_t>::define(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Perform type checking on arguments, and coerce to C++ string type.
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
    return env.Null();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::String name = info[0].As<Napi::String>();
  std::string name_as_str = name.Utf8Value();

  // Perform operation, and stuff native C++ object into wrapped object.
  auto sym = table->define(name_as_str);
  auto x = Symbol<addr_size_t>::GetClass(env, ADDR_SUFFIX).New(
      {Napi::External<std::shared_ptr<symbol::entry<addr_size_t>>>::New(env, &sym)});
  return x;
}

template<typename addr_size_t>
Napi::Value LeafTable<addr_size_t>::mark_global(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Perform type checking on arguments, and coerce to C++ string type.
  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Expected 1 argument").ThrowAsJavaScriptException();
    return env.Null();
  } else if (!info[0].IsString()) {
    Napi::TypeError::New(env, "First argument must be a string").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::String name = info[0].As<Napi::String>();
  std::string name_as_str = name.Utf8Value();

  table->mark_global(name_as_str);

  return env.Null();
}
template<typename addr_size_t>
Napi::Value LeafTable<addr_size_t>::table_index(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Expected 0 arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  return Napi::Number::New(env, (uint64_t) this->table.get());
}

template<typename addr_size_t>
Napi::Value LeafTable<addr_size_t>::parent(const Napi::CallbackInfo &info) {
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
Napi::Value LeafTable<addr_size_t>::rootTable(const Napi::CallbackInfo &info) {
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
Napi::Value LeafTable<addr_size_t>::exists(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1 || info.Length() > 2) {
    Napi::TypeError::New(env, "Expected at [1-2] arguments").ThrowAsJavaScriptException();
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
Napi::Value LeafTable<addr_size_t>::enumerate_symbols(const Napi::CallbackInfo &info) {
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
Napi::Value LeafTable<addr_size_t>::listing(const Napi::CallbackInfo &info) {
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
Napi::Value LeafTable<addr_size_t>::find(const Napi::CallbackInfo &info) {
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
Napi::Function LeafTable<addr_size_t>::GetClass(Napi::Env env, std::string suffix) {
  return LeafTable<addr_size_t>::DefineClass(env, ("LeafTable" + suffix).c_str(), {
      LeafTable::InstanceMethod("get", &LeafTable::get),
      LeafTable::InstanceMethod("reference", &LeafTable::reference),
      LeafTable::InstanceMethod("define", &LeafTable::define),
      LeafTable::InstanceMethod("markGlobal", &LeafTable::mark_global),
      //LeafTable::InstanceMethod("definitionState", &LeafTable::definition_state),
      //LeafTable::InstanceMethod("binding", &LeafTable::binding),
      //LeafTable::InstanceMethod("value", &LeafTable::value),
      LeafTable::InstanceMethod("tableIndex", &LeafTable::table_index),
      LeafTable::InstanceMethod("parent", &LeafTable::parent),
      LeafTable::InstanceMethod("rootTable", &LeafTable::rootTable),
      LeafTable::InstanceMethod("exists", &LeafTable::exists),
      LeafTable::InstanceMethod("enumerateSymbols", &LeafTable::enumerate_symbols),
      LeafTable::InstanceMethod("listing", &LeafTable::listing),
      LeafTable::InstanceMethod("find", &LeafTable::find),
  });
}