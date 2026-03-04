SAI_INC := /usr/include/sai
SAI_LIB := /usr/lib


PYTHON_VERSION := $(shell python3 -c 'import sys; print("%d.%d" % sys.version_info[:2])')
PYTHON_INC     := /usr/include/python$(PYTHON_VERSION)

CC      := gcc
CFLAGS  := -fPIC -I$(PYTHON_INC) -I$(SAI_INC) -I$(SAI_INC)/experimental

LDFLAGS := -L$(SAI_LIB) -lsai

all: _saiut.so

setup:
	rm -rf .inc
	mkdir -p .inc
	ln -sf $(SAI_INC)/saitypes.h .inc/
	ln -sf $(SAI_INC)/saistatus.h .inc/
	ln -sf $(SAI_INC)/saiport.h .inc/

saiut_wrap.c: setup saiut.i
	swig -python saiut.i

saiut_wrap.o: saiut_wrap.c
	$(CC) $(CFLAGS) -c $< -o $@

saiut.o: saiut.c
	$(CC) $(CFLAGS) -c $< -o $@

_saiut.so: saiut.o saiut_wrap.o
	$(CC) -shared $^ $(LDFLAGS) -o $@

clean:
	rm -rf *.so *.o *_wrap.c saiut.py saiut.pyc .inc/

