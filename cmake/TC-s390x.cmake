# cmake -S . -B ./build_s390x -DCMAKE_TOOLCHAIN_FILE="./cmake/TC-s390x.cmake" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CROSSCOMPILING_EMULATOR="qemu-s390x -L /usr/s390x-linux-gnu/"

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR s390x)

# specify the cross compiler
set(CMAKE_C_COMPILER /usr/bin/s390x-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/s390x-linux-gnu-g++)

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
