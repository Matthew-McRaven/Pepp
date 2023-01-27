#pragma once

#include <QEnableSharedFromThis>

#include "entry.hpp"
namespace symbol {

/*
 * Some implementation ideas drawn from:
 * Design and Implementation of the Symbol Table for Object-Oriented Programming
 * Language, Yangsun Lee 2017, http://dx.doi.org/10.14257/ijdta.2017.10.7.03
 */

/*!
 * \brief A symbol table which does not contain any symbols, but instead
 * contains other symbol tables.
 *
 * This table forms the inner node of a hierarchical symbol table.
 * It contains no symbols of its own, to separate the functionality of
 * containing symbols from containing tables. Like the LeafTable, it is add
 * only. No symbols or tables are allowed to be deleted to prevent confusion
 * when dealing with externals' definitions changing.
 *
 * \tparam value_t An unsigned integral type that is large enough to contain the
 * largest address on the target system.
 */
class Table : public QEnableSharedFromThis<Table> {
public:
  //! Default constructor only used for Global BranchTable
  Table() = default;
  [[maybe_unused]] explicit Table(QSharedPointer<Table> parent);
  ~Table() = default;

  //	Copying and move OK
  Table(const Table &) = default;
  Table &operator=(const Table &) = default;
  Table(Table &&) noexcept = default;
  Table &operator=(Table &&) noexcept = default;

  /*!
   * \brief Register an existing symbol table as a child of this table.
   * Instead of calling this directly, it is usually best to use insert_table.
   * This method make sure that the added table has the proper parent, and the
   * parent has an owning reference to the child. \arg child A symbol table that
   * is whose parent is this. \sa symbol::insert_table.
   */
  void add_child(QSharedPointer<Table> child);
  /*!
   * \brief Fetch the list of all children under this table.
   * \returns A mutable list of all children under this table.
   * Please don't abuse the fact that children are non-const.
   */
  QList<Table> children() { return children_; }

  //! A pointer to the parent of this table. If the pointer is null, this table
  //! is the root of the tree.
  const QWeakPointer<Table> parent_ = QWeakPointer<Table>(nullptr);

  using entry_ptr_t = QSharedPointer<symbol::Entry>;
  using map_t = QMap<QString, entry_ptr_t>;
  using range = map_t::iterator;
  using const_range = map_t::const_iterator;

  /*!
   * \brief Unlike reference, get() will not create an entry in the table if the
   * symbol fails to be found. \returns Pointer to found symbol or nullopt if
   * not found.
   */
  std::optional<entry_ptr_t> get(const QString &name) const;

  /*!
   * \brief Create a symbol entry if it doesn't already exist. Do data
   * validations checks to see if symbol is already declared globally. \returns
   * Pointer to symbol.
   */
  entry_ptr_t reference(const QString &name);

  /*!
   * \brief Create a symbol entry if it doesn't already exist. Do data
   * validations checks to see if symbol is already declared globally. Sets
   * definition state of variable. \returns Pointer to symbol.
   */
  entry_ptr_t define(const QString &name);

  /*!
   * Once a symbol has been marked as global, there is no un-global'ing it.
   * This function handles walking the tree and pointing other local symbols to
   * this tables global instance.
   */
  void mark_global(const QString &name);
  //! Returns true if this table (not checking any other table in the hierarchy)
  //! contains a symbol with the matching name.
  bool exists(const QString &name) const;

  //! Return all symbols contained by the table.
  auto entries() const -> const_range;
  //! Return all symbols contained by the table. Mutable to allow
  //! transformations by visitors.
  auto entries() -> range;

private:
  QList<Table> children_;
  map_t name_to_entry_;
};
} // namespace symbol
