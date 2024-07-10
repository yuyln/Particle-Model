#!/bin/sh

set -xe

LIBS="-lm `pkg-config --static --libs x11` -fopenmp"
COMMON_CFLAGS="-O3 -I ./include -mavx2"
FILES="`find ./src -maxdepth 1 -type f -name "*.c"` ./src/platform_specific/render_linux_x11.c"
CC="gcc"

if [ "`pkg-config --libs xext`" > /dev/null ]; then
    LIBS="$LIBS `pkg-config --static --libs xext`"
    COMMON_CFLAGS="$COMMON_CFLAGS -DUSE_XEXT"
fi

if test -f ./libparticle.a; then
    rm ./libparticle.a
fi

if [ "$1" = "install" ]; then
    $CC $COMMON_CFLAGS $CFLAGS -c $FILES $LIBS

    if test -f ~/.local/lib/particle; then
        rm -r ~/.local/lib/particle
    fi
    mkdir --parents ~/.local/lib/particle

    FILES_OBJ="`find -type f -name "*.o"`"
    ar cr libparticle.a $FILES_OBJ

    rm $FILES_OBJ
    mkdir --parents ~/.local/lib/particle/include
    cp ./libparticle.a ~/.local/lib/particle/
    cp -r ./include/* ~/.local/lib/particle/include
else
    set -xe
    CFLAGS="$COMMON_CFLAGS -Wall -Wextra -pedantic -ggdb -DDEBUG"

    $CC $CFLAGS -c $FILES $LIBS

    FILES_OBJ="`find -type f -name "*.o"`"
    ar cr libparticle.a $FILES_OBJ

    rm $FILES_OBJ

    $CC -L./ $CFLAGS main.c -o main -lparticle $LIBS
fi
