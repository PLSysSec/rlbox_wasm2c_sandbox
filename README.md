# RLBOX Wasm2c Sandbox Integration

**This is a prototype. Not yet ready for production use.**

Integration with RLBox sandboxing API to leverage the sandboxing in WASM modules compiled with a modified wasm2c compiler.

For details about the RLBox sandboxing APIs, see [here](https://github.com/PLSysSec/rlbox_api_cpp17).

This code has been tested on 64-bit versions of Ubuntu and Mac OSX.

## Reporting security bugs

If you find a security bug, please do not create a public issue. Instead, file a security bug on bugzilla using the [following template link](https://bugzilla.mozilla.org/enter_bug.cgi?cc=tom%40mozilla.com&cc=nfroyd%40mozilla.com&cc=deian%40cs.ucsd.edu&cc=shravanrn%40gmail.com&component=Security%3A%20Process%20Sandboxing&defined_groups=1&groups=core-security&product=Core&bug_type=defect).

## Building/Running the tests

You can build and run the tests using cmake with the following commands.

```bash
cmake -S . -B ./build
cmake --build ./build --target all
cmake --build ./build --target test
```

On Arch Linux you'll need to install [ncurses5-compat-libs](https://aur.archlinux.org/packages/ncurses5-compat-libs/).

## Using this library

First, build the rlbox_wasm2c_sandbox repo with

```bash
cmake -S . -B ./build
cmake --build ./build --target all
```
(Note: The parallel build is currently broken for first build. Incremental parallel build works fine.)

This wasm2c/wasm integration with RLBox depends on 3 external tools/libraries that are pulled in **automatically** to run the tests included in this repo.

1. [A clang compiler with support for WASM/WASI backend, and the WASI sysroot](https://github.com/CraneStation/wasi-sdk). This allows you to compile C/C++ code to WASM modules usable outside of web browsers (in desktop applications).
2. [The **modified** wasm2c compiler](https://github.com/PLSysSec/wasm2c_sandbox_compiler/) that compiles the produced WASM/WASI module to C code that you can compile into a shared library with a c compiler.
3.  [The RLBox APIs]((https://github.com/PLSysSec/rlbox_api_cpp17)) - A set of APIs that allow easy use of sandboxed libraries.

In the below steps, you can either use the automatically pulled in versions as described below, or download the tools yourself.

In order to sandbox a library of your choice.

- Build the sources of your library along with the file `c_src/wasm2c_sandbox_wrapper.c` and passing the flag `-Wl,--export-all -Wl,--no-entry -Wl,--growable-table` to the linker using the clang compiler described above. This will produce a wasm module. The required clang compiler is available in the path `build/_deps/wasiclang-src/opt/wasi-sdk/bin/clang`.
For instance, to edit an existing `make` based build system, you can run the commmand.

```bash
build/_deps/wasiclang-src/opt/wasi-sdk/bin/clang --sysroot build/_deps/wasiclang-src/opt/wasi-sdk/share/wasi-sysroot/ c_src/wasm2c_sandbox_wrapper.c -c -o c_src/wasm2c_sandbox_wrapper.o

CC=build/_deps/wasiclang-src/opt/wasi-sdk/bin/clang                            \
CXX=build/_deps/wasiclang-src/opt/wasi-sdk/bin/clang++                         \
CFLAGS="--sysroot build/_deps/wasiclang-src/opt/wasi-sdk/share/wasi-sysroot/"  \
LD=build/_deps/wasiclang-src/opt/wasi-sdk/bin/wasm-ld                          \
LDLIBS=wasm2c_sandbox_wrapper.o                                                \
LDFLAGS=-Wl,--export-all -Wl,--no-entry -Wl,--growable-table                   \
make
```

- Assuming the above command produced the wasm module `libFoo.wasm`, compile this to an ELF shared library using the modified wasm2cc compiler as shown below.

```bash
build/cargo/release/wasm2cc                                        \
    -o libWasmFoo.so                                             \
    libFoo.wasm
```
- Finally you can write sandboxed code, just as you would with any other RLBox sandbox, such as in the short example below. For more detailed examples, please refer to the tutorial in the [RLBox Repo]((https://github.com/PLSysSec/rlbox_api_cpp17)).


```c++
#include "rlbox_wasm2c_sandbox.hpp"
#include "rlbox.hpp"

int main()
{
    rlbox_sandbox<rlbox_wasm2c_sandbox> sandbox;
    sandbox.create_sandbox("libWasmFoo.so");
    // Invoke function bar with parameter 1
    sandbox.invoke_sandbox_function(bar, 1);
    sandbox.destroy_sandbox();
    return 0;
}
```

- To compile the above example, you must include the rlbox header files in `build/_deps/rlbox-src/code/include`, the integration header files in `include/` and the wasm2c_sandbox library in `build/cargo/{debug or release}/librlbox_wasm2c_sandbox.a` (make sure to use the whole archive and the rdynamic linker options). For instance, you can compile the above with

```bash
g++ -std=c++17 example.cpp -o example -I build/_deps/rlbox-src/code/include -I include -Wl,--whole-archive -l:build/cargo/debug/librlbox_wasm2c_sandbox.a -Wl,--no-whole-archive -Wl,-rdynamic
```

## Contributing Code

1. To contribute code, it is recommended you install clang-tidy which the build
uses if available. Install using:

   On Ubuntu:
```bash
sudo apt install clang-tidy
```
   On Arch Linux:
```bash
sudo pacman -S clang-tidy
```

2. It is recommended you use the dev mode for building during development. This
treat warnings as errors, enables clang-tidy checks, runs address sanitizer etc.
Also, you probably want to use the debug build. To do this, adjust your build
settings as shown below

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DDEV=ON -S . -B ./build
```

3. After making changes to the source, add any new required tests and run all
tests as described earlier.

4. To make sure all code/docs are formatted with, we use clang-format.
Install using:

   On Ubuntu:
```bash
sudo apt install clang-format
```
   On Arch Linux:
```bash
sudo pacman -S clang-format
```

5. Format code with the format-source target:
```bash
cmake --build ./build --target format-source
```

6. Submit the pull request.
