//
// Created by servostar on 6/8/24.
//
// based on: https://github.com/llvm/llvm-project/blob/main/lld/unittests/AsLibAll/AllDrivers.cpp
//           https://github.com/numba/llvmlite/blob/main/ffi/linker.cpp

#include <lld/Common/Driver.h>
#include <llvm/Support/raw_ostream.h>

/*
 *  Gemstone supports Windows (COFF, MinGW) and GNU/Linux (ELF)
 */

LLD_HAS_DRIVER(coff)
LLD_HAS_DRIVER(elf)
LLD_HAS_DRIVER(mingw)
// LLD_HAS_DRIVER(macho)
// LLD_HAS_DRIVER(wasm)

#define LLD_COFF_ELF_MINGW_DRIVER { {lld::WinLink, &lld::coff::link}, {lld::Gnu, &lld::elf::link}, {lld::MinGW, &lld::mingw::link} }

extern "C" {

/**
 * @brief C-wrapper for lldMain which is written in C++
 * @param Argc
 * @param Argv
 * @param outstr
 * @return
 */
int lld_main(int Argc, const char **Argv, const char **outstr) {
    // StdOut
    std::string stdout;
    llvm::raw_string_ostream stdout_stream(stdout);

    // StdErr
    std::string stderr;
    llvm::raw_string_ostream stderr_stream(stderr);

    // convert arguments
    std::vector<const char *> Args(Argv, Argv + Argc);

    lld::Result result = lld::lldMain(Args, stdout_stream, stderr_stream, LLD_COFF_ELF_MINGW_DRIVER);

    *outstr = strdup(stdout.c_str());

    return !result.retCode && result.canRunAgain;
}

} // extern "C"
