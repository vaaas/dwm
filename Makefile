# dwm - dynamic window manager
# See LICENSE file for copyright and license details.
# dwm version
VERSION = 6.2

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

XINERAMALIBS  = -lXinerama
XINERAMAFLAGS = -DXINERAMA

FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC = /usr/include/freetype2

# includes and libs
INCS = -I${X11INC} -I${FREETYPEINC}
LIBS = -L${X11LIB} -lX11 ${XINERAMALIBS} ${FREETYPELIBS}

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=2 -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS}
CFLAGS   = -std=c17 -pedantic -Wall -Wno-deprecated-declarations -O2 ${INCS} ${CPPFLAGS}
LDFLAGS  = ${LIBS}

CC = gcc
SRC = src/dwm.c

all: options bin/dwm

options:
	@echo dwm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

bin/:
	mkdir -p $@

bin/dwm: bin/
	${CC} ${SRC} -o $@ ${LDFLAGS} ${CFLAGS}

install: all
	cp bin/dwm ~/Binaries/dwm

.PHONY: all options install
