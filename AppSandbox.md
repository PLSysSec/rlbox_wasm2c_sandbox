# Using this tool to sandbox a library

In order to sandbox a full program of your choice.

- Build the sources of your program passing the flag `-Wl,--export-all -Wl,--growable-table` to the linker using the wasi-clang compiler. This will produce a wasm module. The required wasi-clang compiler is available in the path `build/_deps/wasiclang-src/opt/wasi-sdk/bin/clang`.
For instance, to edit an existing `make` based build system, you can run the commmand.

   ```bash
   CC=build/_deps/wasiclang-src/opt/wasi-sdk/bin/clang                            \
   CXX=build/_deps/wasiclang-src/opt/wasi-sdk/bin/clang++                         \
   CFLAGS="--sysroot build/_deps/wasiclang-src/opt/wasi-sdk/share/wasi-sysroot/"  \
   LD=build/_deps/wasiclang-src/opt/wasi-sdk/bin/wasm-ld                          \
   LDFLAGS="-Wl,--export-all -Wl,--growable-table"                                  \
   make
   ```

- Assuming the above command produced the wasm module `libFoo.wasm`, compile this to an C file using the modified wasm2c compiler as shown below.

   ```bash
   build/_deps/mod_wasm2c-src/bin/wasm2c -o WasmFoo.c Foo.wasm
   ```

- Compile this and wasm2c runtime to a shared library using your platform c compiler. On linux this may done as shown below. You can adjust this command as needed for Mac/Windows.

   ```bash
   gcc -shared -fPIC -O3 -o WasmFoo.so WasmFoo.c build/_deps/mod_wasm2c-src/wasm2c/wasm-rt-impl.c build/_deps/mod_wasm2c-src/wasm2c/wasm-rt-os-unix.c build/_deps/mod_wasm2c-src/wasm2c/wasm-rt-os-win.c build/_deps/mod_wasm2c-src/wasm2c/wasm-rt-wasi.c
   ```

- Finally you can run the program with wasm2c-runner tool as shown

   ```bash
   build/_deps/mod_wasm2c-src/bin/wasm2c-runner WasmFoo.so
   ```
