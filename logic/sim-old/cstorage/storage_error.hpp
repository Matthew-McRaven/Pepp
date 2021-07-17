/*
 * Implementation derived from:
 * https://ned14.github.io/outcome/motivation/plug_error_code/
 */
#include <iostream>
#include <string>        // for string printing
#include <system_error>  // bring in std::error_code et al

// This is the custom error code enum
enum class StorageErrc
{
  Success     = 0, // 0 should not represent an error
  NoMMInput, // Storage operation failed because there was no memory-mapped input available. This is recoverable.
  ResizeError, // Attempted to resize a component which may not be resized. This is recoverable by creating additional instances of the class.
  Unwritable, // Write failed because the device doesn't support writing. The value written was ignored. This is recoverable.
  NoAvailableHistory, // Attempted to read a storage device whose history has been exhausted. This is recoverable.
  HistoryDisabled // Attempted to read a storage device which has history disabled. This is recoverable.
};

namespace std
{
  // Tell the C++ 11 STL metaprogramming that enum StorageErrc
  // is registered with the standard error code system
  template <> struct is_error_code_enum<StorageErrc> : true_type
  {
  };
}

namespace detail
{
  // Define a custom error code category derived from std::error_category
  class StorageErrc_category : public std::error_category
  {
  public:
    // Return a short descriptive name for the category
    virtual const char *name() const noexcept override final { return "StorageError"; }
    // Return what each enum means in text
    virtual std::string message(int c) const override final
    {
      switch (static_cast<StorageErrc>(c))
      {
      case StorageErrc::Success:
        return "Storage access successful";
      case StorageErrc::NoMMInput:
        return "No memory-mapped input available";
      case StorageErrc::ResizeError:
        return "Storage resize failed";
      default:
        return "unknown";
      }
    }
    // OPTIONAL: Allow generic error conditions to be compared to me
    virtual std::error_condition default_error_condition(int c) const noexcept override final
    {
      switch (static_cast<StorageErrc>(c))
      {
      case StorageErrc::NoMMInput:
        return make_error_condition(std::errc::io_error);
      case StorageErrc::ResizeError:
        return make_error_condition(std::errc::not_enough_memory);
      default:
        // I have no mapping for this code
        return std::error_condition(c, *this);
      }
    }
  };
}

// Declare a global function returning a static instance of the custom category
extern inline const detail::StorageErrc_category &StorageErrc_category()
{
  static detail::StorageErrc_category c;
  return c;
}

// Overload the global make_error_code() free function with our
// custom enum. It will be found via ADL by the compiler if needed.
inline std::error_code make_error_code(StorageErrc e)
{
  return {static_cast<int>(e), StorageErrc_category()};
}
