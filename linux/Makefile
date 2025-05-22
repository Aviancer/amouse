# Anachro Mouse, a usb to serial mouse adapter. Copyright (C) 2021 Aviancer <oss+amouse@skyvian.me>
#
# This library is free software; you can redistribute it and/or modify it under the terms of the 
# GNU Lesser General Public License as published by the Free Software Foundation; either version 
# 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without 
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with this library; 
# if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

.DEFAULT_GOAL  := all
SRC_DIR        := src
SHARED_DIR     := ../shared
BIN_DIR        := bin
C_SOURCES      := $(shell find $(SRC_DIR) -name '*.c')
C_SHARED       := $(shell find $(SHARED_DIR) -name '*.c')

CC = gcc
CFLAGS = -g -Wall
INCLUDES = -levdev -I/usr/include/libevdev-1.0/libevdev -I./include

TARGET = amouse

all: serial.o utils.o mouse.o storage.o ${TARGET}

${TARGET}: ${SRC_DIR}/${TARGET}.c
	${CC} ${CFLAGS} -o ${BIN_DIR}/${TARGET} ${C_SOURCES} ${C_SHARED} ${INCLUDES} 

utils.o: ${SRC_DIR}/../../shared/utils.c ${SRC_DIR}/../../shared/utils.h
	${CC} ${CFLAGS} -c ${SRC_DIR}/../../shared/utils.c -o ${SRC_DIR}/include/utils.o

mouse.o: ${SRC_DIR}/../../shared/mouse.c ${SRC_DIR}/../../shared/mouse.h
	${CC} ${CFLAGS} -c ${SRC_DIR}/../../shared/mouse.c -o ${SRC_DIR}/include/mouse.o

serial.o: ${SRC_DIR}/include/serial.c ${SRC_DIR}/include/serial.h
	${CC} ${CFLAGS} -c ${SRC_DIR}/include/serial.c -o ${SRC_DIR}/include/serial.o

storage.o: ${SRC_DIR}/include/storage.c ${SRC_DIR}/include/storage.h
	${CC} ${CFLAGS} -c ${SRC_DIR}/include/storage.c -o ${SRC_DIR}/include/storage.o

clean:
	${RM} ${BIN_DIR}/${TARGET}
	${RM} ${SRC_DIR}/include/*.o

# PREFIX is environment variable, but if not set, use default value
ifeq ($(PREFIX),)
        PREFIX := /usr/local
endif

install: amouse
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 bin/amouse $(DESTDIR)$(PREFIX)/bin/
