# Makefile for the "aesdsocket" application

# Compiler and flags
CC := $(CROSS_COMPILE)gcc
CFLAGS := -g -Wall -Werror
CUR_PATH=$(dir $(abspath $(MAKEFILE_LIST)))

# libraries to link
LDLIBS= -lm -lpthread -lrt -lthreading -lmanipulate_file -lsignal_func
OBJS=(filter-out aesdsocket.o $(patsubst %.c, %.o, $(wildcard *.c)))
LIBS_TYPE=""

.PHONY: all aesdsocket clean

all: aesdsocket

# link libraries even dynamically or statically 
ifneq ($(STATIC_LIBS), true)
LIBS_TYPE="so"
CFLAGS+= -Wl,-rpath=$(CUR_PATH)
libmanipulate_file.$(LIBS_TYPE): manipulate_file
	$(CC) $(CFLAGS) -shared -o $@ $<.o

libsignal_func.$(LIBS_TYPE): signal_func
	$(CC) $(CFLAGS) -shared -o $@ $<.o

libthreading.$(LIBS_TYPE): threading
	$(CC) $(CFLAGS) -shared -o $@ $<.o

else 
LIBS_TYPE="a"
libmanipulate_file.$(LIBS_TYPE): manipulate_file
	$(AR) -rc $@ $<.o

libsignal_func.$(LIBS_TYPE): signal_func
	$(AR) -rc $@ $<.o

libthreading.$(LIBS_TYPE): threading
	$(AR) -rc $@ $<.o
endif

# build 

aesdsocket: libthreading.$(LIBS_TYPE) libmanipulate_file.$(LIBS_TYPE) libsignal_func.$(LIBS_TYPE)
	$(CC) $(CFLAGS) -o $@ $(@).c -L$(CUR_PATH) $(LDLIBS)

manipulate_file:
	$(CC) $(CFLAGS) -fPIC -c $@.c $(LDLIBS)

signal_func:
	$(CC) $(CFLAGS) -fPIC -c $@.c $(LDLIBS)

threading:
	$(CC) $(CFLAGS) -fPIC -c $@.c $(LDLIBS)

clean:	
	$(RM) aesdsocket $(wildcard *.o) $(wildcard *.so) $(wildcard *.a)