#!/bin/sh
set -xe
CFLAGS="-L./OpenCL/lib -I./OpenCL/include -DnPROFILING -Wall -Wextra -pedantic -O0 -ggdb -I ./include -DCL_TARGET_OPENCL_VERSION=300 -DCL_USE_DEPRECATED_OPENCL_1_2_APIS -Wno-overlength-Strings -Wno-override-init"
FILES="`find ./src -maxdepth 1 -type f -name "*.c"` ./src/platform_specific/render_windows.c"
CC="x86_64-w64-mingw32-gcc"
LIBS="-lm -lOpenCL -lkernel32 -luser32 -lgdi32"

$CC $CFLAGS create_kernel.c -o create_kernel.out

./create_kernel.out ./kernel_files/tyche_i.c ./include/constants.h ./include/v3d.h ./include/grid_primitive_types.h ./kernel_files/random.h ./kernel_files/simulation_funcs.h ./include/colors.h ./src/v3d.c ./kernel_files/random.c ./kernel_files/simulation_funcs.c ./src/colors.c ./kernel_files/kernel.c

rm ./create_kernel.out

$CC $CFLAGS -c $FILES $LIBS --static 

FILES_OBJ="`find -type f -name "*.o"`"
ar cr libatomistic.a $FILES_OBJ

rm *.o

$CC -L./ $CFLAGS main.c -o main -latomistic $LIBS
