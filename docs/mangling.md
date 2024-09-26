# Mangling

The following document gives an overview of the topic of mangling in compiler design and describe the mangling implementation used by the Gemstone compiler.

**Table of Contents**

<!-- TOC -->
* [Mangling](#mangling)
  * [Abstract](#abstract)
  * [Available characters for symbol names](#available-characters-for-symbol-names)
  * [Specification](#specification)
    * [Common Prefix](#common-prefix)
    * [Functions](#functions)
    * [Global variables](#global-variables)
    * [References](#references)
<!-- TOC -->

---

## Abstract

According to Wikipedia \[1] mangling refers to the following:

> In compiler construction, name mangling (also called name decoration) is a technique used to solve various problems caused by the need to resolve unique names for programming entities in many modern programming languages.
> 
> It provides means to encode added information in the name of a function, structure, class or another data type, to pass more semantic information from the compiler to the linker.

Mangling changes the names of symbols such as functions or variables so that symbols of the same name but different implementation or semantic (like variables in different modules) can be used in the same object file.
The linker will complain about multiple symbols with the same name. Names alone are not enough to uniquely identify certain symbols.
Thus encoding additional information into the symbols name solved the problem.

A simple example on how basic mangling can be achieved for functions with the same name which are located in different modules:

```rust
mod A  {
    fn gee() { }
}

mod B  {
    fn gee() { }
}
```

A simple solution for mangling would be to prefix any functions name with the module separated by an underscore. The first `gee` function would get the name `A_gee` whereas the second function would become `B_gee` avoiding a name clash.

Many such schemes exist in modern compilers such as the [Itanium C++ ABI](https://refspecs.linuxbase.org/cxxabi-1.86.html#mangling) used by C++,
[RFC 2603](https://github.com/rust-lang/rfcs/blob/master/text/2603-rust-symbol-name-mangling-v0.md) by Rust \[2]\[3].

## Available characters for symbol names

Taking into account both the GNU/Linux linker `ld` and Microsofts the following list of symbol classes can be used for symbols across at least Windows and GNU/Linux \[4, p 84]\[5]:

| Class      | Symbols                                                |
|------------|--------------------------------------------------------|
| letters    | `abcdefghijklmnopqrstuvwxyzABCDEFGHJIKLMNOPQRSTUVWXYZ` |
| underscore | `_`                                                    |
| period     | `.`                                                    |
| hypen      | `-`                                                    |
| digits     | `0123456789`                                           |

---

## Specification

### Common Prefix

Every mangled name is prefixed with `gsc` to denote the "Gemstone Compiler name mangling convention".

### Functions

Data required for mangling functions:
- Function name
- Parameter name
- Parameter type
- Return type
- Parent modules

### Global variables

Data required for mangling global variables:
- Name
- Type
- Parent modules

### References

\[1]: https://en.wikipedia.org/wiki/Name_mangling.

\[2]: https://github.com/rust-lang/rfcs/blob/master/text/2603-rust-symbol-name-mangling-v0.md

\[3]: https://refspecs.linuxbase.org/cxxabi-1.86.html#mangling

\[4]: https://sourceware.org/binutils/docs-2.37/ld.pdf

\[5]: https://learn.microsoft.com/en-us/cpp/build/reference/decorated-names?view=msvc-170