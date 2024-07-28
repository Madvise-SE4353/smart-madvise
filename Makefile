obj-m += smart-madvise.o pagecache_collector.o
smart-madvise-objs+= ./src/smart-madvise.o ./src/executor.o ./src/global_map.o ./src/pagecache_collector
# obj-m += smart-madvise-syscalltable.o
ccflags-y := -I${PWD}/include
KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) clean

# build-collector:
# 	@echo "Building the kernel modules..."
# 	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(shell pwd) modules

# clean-collector:
# 	@echo "Cleaning up build artifacts..."
# 	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(shell pwd) clean

install-collector:
	@echo "Installing the kernel modules..."
	sudo insmod src/pagecache_collector.ko

remove-collector:
	@echo "Removing the kernel modules..."
	sudo rmmod pagecache_collector

# .PHONY: build-collector clean-collector install-collector remove-collector
