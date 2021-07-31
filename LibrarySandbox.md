# Using this tool to sandbox a library

In order to sandbox a library of your choice.

- Build the sources of your library along with the file `c_src/wasm2c_sandbox_wrapper.c` and passing the flag `-Wl,--export-all -Wl,--no-entry -Wl,--growable-table` to the linker using the wasi-clang compiler. This will produce a wasm module. The required wasi-clang compiler is available in the path `build/_deps/wasiclang-src/opt/wasi-sdk/bin/clang`.
For instance, to edit an existing `make` based build system, you can run the commmand.

   ```bash
   build/_deps/wasiclang-src/opt/wasi-sdk/bin/clang --sysroot build/_deps/wasiclang-src/opt/wasi-sdk/share/wasi-sysroot/ c_src/wasm2c_sandbox_wrapper.c -c -o c_src/wasm2c_sandbox_wrapper.o

   CC=build/_deps/wasiclang-src/opt/wasi-sdk/bin/clang                            \
   CXX=build/_deps/wasiclang-src/opt/wasi-sdk/bin/clang++                         \
   CFLAGS="--sysroot build/_deps/wasiclang-src/opt/wasi-sdk/share/wasi-sysroot/"  \
   LD=build/_deps/wasiclang-src/opt/wasi-sdk/bin/wasm-ld                          \
   LDLIBS=wasm2c_sandbox_wrapper.o                                                \
   LDFLAGS="-Wl,--export-all -Wl,--no-entry -Wl,--growable-table"                   \
   make
   ```

- Assuming the above command produced the wasm module `libFoo.wasm`, compile this to an C file using the modified wasm2c compiler as shown below.

   ```bash
   build/_deps/mod_wasm2c-src/bin/wasm2c -o libWasmFoo.c libFoo.wasm
   ```

- Compile this and wasm2c runtime to a shared library using your platform c compiler. On linux this may done as shown below. You can adjust this command as needed for Mac/Windows.

   ```bash
   gcc -shared -fPIC -O3 -o libWasmFoo.so libWasmFoo.c build/_deps/mod_wasm2c-src/wasm2c/wasm-rt-impl.c build/_deps/mod_wasm2c-src/wasm2c/wasm-rt-os-unix.c build/_deps/mod_wasm2c-src/wasm2c/wasm-rt-os-win.c build/_deps/mod_wasm2c-src/wasm2c/wasm-rt-wasi.c
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

- To compile the above example, you must include the rlbox header files in `build/_deps/rlbox-src/code/include`, the integration header files in `include/`. For instance, you can compile the above with

   ```bash
   g++ -std=c++17 example.cpp -o example -I build/_deps/rlbox-src/code/include -I include
   ```
