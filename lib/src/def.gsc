# Author: Sven Vogel
# Edited: 25.05.2024
# License: GPL-2.0

# ,----------------------------------------.
# |       Standard Type definitions        |
# `----------------------------------------`

# Unsigned integrals

type unsigned half half int: u8
type unsigned half int: u16
type unsigned int: u32
type unsigned double int: u64
type unsigned double double int: u128

# Signed integrals

type signed u8: i8
type signed u16: i16
type signed u32: i32
type signed u64: i64
type signed u128: i128

# IEEE-754 floating point

type signed half float: f16
type signed float: f32
type signed double float: f64
type signed double double float: f128

# String constant

type ref u8: cstr

# C style void pointer replacement

type ref u8: ptr
