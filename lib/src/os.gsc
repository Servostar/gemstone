# Author: Sven Vogel
# Edited: 03.06.2024
# License: GPL-2.0

# ,----------------------------------------.
# |           Operating System             |
# `----------------------------------------`

include "def.gsc"

# Return a hard coded C string identifying the underlying operating system
# Will return one of the following:
#  - "windows" (for Windows 7, Windows 10 and Windows 11)
#  - "unix" (for GNU/Linux and BSD)
fun getPlatformName(out cstr: name)

# Return a C string to the value of an environment varible named
# after the C string name.
fun getEnvVar(in cstr: name)(out cstr: value)

# Set the value of an environment variable with name to value.
fun setEnvVar(in cstr: name, in cstr: value)

# Unset a specific environment variable
fun unsetEnvVar(in cstr: name)