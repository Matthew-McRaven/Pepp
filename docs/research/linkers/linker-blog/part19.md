# Part 19: \_\_start and \_\_stop Symbols, Byte Swapping

I’ve pretty much run out of linker topics. Unless I think of something new, I’ll make tomorrow’s post be the last one, for a total of 20.

## \_\_start and \_\_stop Symbols

A quick note about another GNU linker extension. If the linker sees a section in the output file which can be part of a C variable name–the name contains only alphanumeric characters or underscore–the linker will automatically define symbols marking the start and stop of the section. Note that this is not true of most section names, as by convention most section names start with a period. But the name of a section can be any string; it doesn’t have to start with a period. And when that happens for section NAME, the GNU linker will define the symbols `__start_NAME` and `__stop_NAME` to the address of the beginning and the end of section, respectively.

This is convenient for collecting some information in several different object files, and then referring to it in the code. For example, the GNU C library uses this to keep a list of functions which may be called to free memory. The `__start` and `__stop` symbols are used to walk through the list.

In C code, these symbols should be declared as something like `extern char __start_NAME[]`. For an extern array the value of the symbol and the value of the variable are the same.

## Byte Swapping

The new linker I am working on, gold, is written in C++. One of the attractions was to use template specialization to do efficient byte swapping. Any linker which can be used in a cross-compiler needs to be able to swap bytes when writing them out, in order to generate code for a big-endian system while running on a little-endian system, or vice-versa. The GNU linker always stores data into memory a byte at a time, which is unnecessary for a native linker. Measurements from a few years ago showed that this took about 5% of the linker’s CPU time. Since the native linker is by far the most common case, it is worth avoiding this penalty.

In C++, this can be done using templates and template specialization. The idea is to write a template for writing out the data. Then provide two specializations of the template, one for a linker of the same endianness and one for a linker of the opposite endianness. Then pick the one to use at compile time. The code looks this; I’m only showing the 16-bit case for simplicity.

```cpp
// Endian simply indicates whether the host is big endian or not.
struct Endian { public: 
  // Used for template specializations.
  static const bool host_big_endian = __BYTE_ORDER == __BIG_ENDIAN;
};
// Valtype_base is a template based on size (8, 16, 32, 64) which
// defines the type Valtype as the unsigned integer of the specified size.
template struct Valtype_base;
template<> struct Valtype_base<16> { typedef uint16_t Valtype; };
// Convert_endian is a template based on size and on whether the host // and target have the same endianness. It defines the type Valtype // as Valtype_base does, and also defines a function convert_host // which takes an argument of type Valtype and returns the same value,
// but swapped if the host and target have different endianness.
template struct Convert_endian;
template struct Convert_endian { typedef typename Valtype_base::Valtype Valtype;
static inline Valtype convert_host(Valtype v) { return v; } };
template<> struct Convert_endian<16, false> { typedef Valtype_base<16>::Valtype Valtype;
static inline Valtype convert_host(Valtype v) { return bswap_16(v); } };
// Convert is a template based on size and on whether the target is
// big endian. It defines Valtype and convert_host like
// Convert_endian. That is, it is just like Convert_endian except in
// the meaning of the second template parameter.
template struct Convert { typedef typename Valtype_base::Valtype Valtype;
static inline Valtype convert_host(Valtype v) { return Convert_endian ::convert_host(v); } };
// Swap is a template based on size and on whether the target is big
// endian. It defines the type Valtype and the functions readval and
// writeval. The functions read and write values of the appropriate
// size out of buffers, swapping them if necessary.
template struct Swap { typedef typename Valtype_base::Valtype Valtype;
static inline Valtype readval(const Valtype* wv) { return Convert::convert_host(*wv); }
static inline void writeval(Valtype* wv, Valtype v) { *wv = Convert::convert_host(v); }
```

Now, for example, the linker reads a 16-bit big-endian value using `Swap<16,true>::readval`. This works because the linker always knows how much data to swap in, and it always knows whether it is reading big- or little-endian data.
