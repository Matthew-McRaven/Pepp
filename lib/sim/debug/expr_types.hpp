#pragma once
#include <QString>
#include <compare>
#include <cstdint>
#include <memory>
#include "expr_cache.hpp"

namespace pepp::debug::types {
Q_NAMESPACE
enum class Primitives : uint8_t { i8, u8, i16, u16, i32, u32 };
Q_ENUM_NS(Primitives);
class Type;

// Used to ensure that no two equal types have different pointers.
using TypeCache = pepp::debug::Cache<Type>;

class NamedTypes {
public:
  explicit NamedTypes(std::shared_ptr<TypeCache> cache);
  NamedTypes(const NamedTypes &other) = default;
  ~NamedTypes() = default;

  std::shared_ptr<Type> get(const QString &name);
  std::shared_ptr<const Type> get(const QString &name) const;
  void add(const QString &name, std::shared_ptr<Type> type);

private:
  std::shared_ptr<TypeCache> _cache;
  std::map<QString, std::shared_ptr<Type>> _types;
};

class Type : public std::enable_shared_from_this<Type> {
public:
  enum class NodeType { Never, Primitive, Pointer, Array, Struct };
  virtual ~Type() = 0;
  virtual NodeType node_type() const = 0;
  virtual bool is_unsigned() const = 0;
  virtual uint32_t bitness() const = 0;
  virtual QString to_string() const = 0;
  virtual std::shared_ptr<Type> common_type(const Type &other, TypeCache &cache) const = 0;
  virtual std::strong_ordering operator<=>(const Type &rhs) const = 0;
};

class Never final : public Type {
public:
  NodeType node_type() const override;
  ~Never() override = default;
  bool is_unsigned() const override;
  ;
  uint32_t bitness() const override;
  ;
  QString to_string() const override;
  std::shared_ptr<Type> common_type(const Type &other, TypeCache &cache) const override;
  std::strong_ordering operator<=>(const Type &rhs) const override;
  std::strong_ordering operator<=>(const Never &rhs) const;
};

class Primitive final : public Type {
public:
  explicit Primitive(Primitives value);
  Primitive(const Primitive &other) = default;
  ~Primitive() override = default;
  NodeType node_type() const override;
  bool is_unsigned() const override;
  uint32_t bitness() const override;
  QString to_string() const override;
  std::shared_ptr<Type> common_type(const Type &other, TypeCache &cache) const override;
  std::shared_ptr<Type> common_type(const Primitive &rhs, TypeCache &cache) const;
  std::strong_ordering operator<=>(const Type &rhs) const override;
  std::strong_ordering operator<=>(const Primitive &rhs) const;
  const Primitives value;
};

class Pointer final : public Type {
public:
  explicit Pointer(std::shared_ptr<Type> value);
  Pointer(const Pointer &other) = default;
  ~Pointer() override = default;
  NodeType node_type() const override;
  bool is_unsigned() const override;
  uint32_t bitness() const override;
  QString to_string() const override;
  std::shared_ptr<Type> common_type(const Type &other, TypeCache &cache) const override;
  std::strong_ordering operator<=>(const Type &rhs) const override;
  std::strong_ordering operator<=>(const Pointer &rhs) const;

  const std::shared_ptr<Type> value;
};

class Array final : public Type {
public:
  Array(std::shared_ptr<Type> type, quint16 size);
  Array(const Array &other) = default;
  ~Array() override = default;
  NodeType node_type() const override;
  bool is_unsigned() const override;
  uint32_t bitness() const override;
  QString to_string() const override;
  std::shared_ptr<Type> common_type(const Type &other, TypeCache &cache) const override;
  std::shared_ptr<Type> common_type(const Array &other, TypeCache &cache) const;
  std::strong_ordering operator<=>(const Type &rhs) const override;
  std::strong_ordering operator<=>(const Array &rhs) const;

  const quint16 size = 0;
  const std::shared_ptr<Type> value = nullptr;
};

class Struct final : public Type {
public:
  Struct(std::vector<std::shared_ptr<const Type>> members);
  Struct(const Struct &other) = default;
  ~Struct() override = default;
  NodeType node_type() const override;
  bool is_unsigned() const override { return false; }
  uint32_t bitness() const override;
  QString to_string() const override;
  std::shared_ptr<Type> common_type(const Type &other, TypeCache &cache) const override;
  std::shared_ptr<Type> common_type(const Struct &other, TypeCache &cache) const;
  std::strong_ordering operator<=>(const Type &rhs) const override;
  std::strong_ordering operator<=>(const Struct &rhs) const;
  const std::vector<std::shared_ptr<const Type>> members;
};

} // namespace pepp::debug::types
