SAI_INC := /usr/include/sai
SAI_LIB := /usr/lib

CC      := gcc
CFLAGS  := -fPIC -I/usr/include/python3.11 -I$(SAI_INC) -I$(SAI_INC)/experimental

LDFLAGS := -L$(SAI_LIB) -lsai

all: _saiut.so

setup:
	rm -rf .inc
	mkdir -p .inc
	ln -s $(SAI_INC)/saitypes.h .inc/
	ln -s $(SAI_INC)/saistatus.h .inc/
	ln -s $(SAI_INC)/saiport.h .inc/

saiut_wrap.c: setup saiut.i
	swig -python saiut.i

saiut_wrap.o: saiut_wrap.c

_saiut.so: saiut.o saiut_wrap.o
	gcc -shared $^ $(LDFLAGS) -o $@

clean:
	rm -rf *.so *.o *_wrap.c saiut.py saiut.pyc .inc/
