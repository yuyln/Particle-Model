#!/bin/sh

set -xe

LIBS="-lm `pkg-config --static --libs x11 xext`"
COMMON_CFLAGS="-DnPROFILING -O0 -I ./include"
FILES="`find ./src -maxdepth 1 -type f -name "*.c"` ./src/platform_specific/render_linux_x11.c"
CC="gcc"

if [ "$1" = "install" ]; then
    $CC $CFLAGS -c $FILES $LIBS

    FILES_OBJ="`find -type f -name "*.o"`"
    ar cr libatomistic.a $FILES_OBJ

    rm $FILES_OBJ
    mkdir --parents ~/.local/lib/particle/include
    cp ./libparticle.a ~/.local/lib/particle/
    cp -r ./include ~/.local/lib/particle/include
else
    set -xe
    CFLAGS="$COMMON_CFLAGS -Wall -Wextra -pedantic -ggdb -fsanitize=address -fsanitize=undefined -DDEBUG"

    $CC $CFLAGS -c $FILES $LIBS

    FILES_OBJ="`find -type f -name "*.o"`"
    ar cr libparticle.a $FILES_OBJ

    rm $FILES_OBJ

    $CC -L./ $CFLAGS main.c -o main -lparticle $LIBS
fi
