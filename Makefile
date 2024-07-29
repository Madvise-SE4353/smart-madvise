obj-m += smart-madvise.o 
smart-madvise-objs+= ./src/smart-madvise.o ./src/executor.o ./src/global_map.o ./src/daemon.o
# obj-m += smart-madvise-syscalltable.o
ccflags-y := -I${PWD}/include
KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) clean
