obj-m += smart-madvise-kprobe.o 
smart-madvise-kprobe-objs+= ./src/smart-madvise-kprobe.o ./src/myprintk.o 
# obj-m += smart-madvise-syscalltable.o
ccflags-y := -I${PWD}/include
KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) clean
