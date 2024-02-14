# Gemstone

Gemstone is a programming language compiler written in C with lex and yacc.

## Development with VSCode/Codium

Recommended extensions for getting a decent experience are the following:
- https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd
- https://marketplace.visualstudio.com/items?itemName=daohong-emilio.yash
- https://marketplace.visualstudio.com/items?itemName=cschlosser.doxdocgen
- https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools
- https://marketplace.visualstudio.com/items?itemName=twxs.cmake

In order to remove false error messages from clangd CMake has to be run once in order generate compile_commands.json.

## Build
The build pipeline is configured with CMake in the file CMakeLists.txt.
In order to avoid dependency and configuration issues the recommended way to build is by using the provided docker containers.
All tools required for building (`cmake`, `make`, `gcc`, `lex`, `yacc`) are installed inside the SDK container (see Dockerfile sdk/Dockerfile).
For creating the build pipeline build the Dockerfile in the root folder of this repository. This takes the current SDK and copies the source files into the home of the build user.
Then the make targets are generated. Running `make release` will build gemstone from source in release mode.
The generated binaries can be found either in `bin/release/gsc` or `bin/debug/gsc` depending on the chosen target.
The following graph visualizes the build pipeline:
```
                 SDK (environment)
                  │
                  │ configure build environment
                  │  cmake, make, gcc, yacc, lex
                  │
                  ▼
                 Devkit (pipeline)
                  │
                  │ create build pipeline
                  │  create make targets
                  ▼
                 Pipeline
     

yacc (generate files)    GCC (compile)   Extra Source Files (src/*.c)
│                             │                     │
├─ parser.tab.h ─────────────►│◄────────────────────┘
│                             │
└─ parser.tab.c ─────────────►│
                              │
lex (generate file)           │
│                             │
└─ lexer.ll.c  ──────────────►│
                              │
                              ▼
                             gsc
```

## Docker images
Currently, the SDK is based on Almalinux 9.3, an open source distro binary compatible to RHEL 9.3.

The following images can be found in the offical repository at [Docker Hub](https://hub.docker.com/r/servostar/gemstone):
- SDK
- Devkit