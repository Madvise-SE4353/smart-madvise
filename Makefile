obj-m += smart-madvise.o 
smart-madvise-objs += ./src/smart-madvise.o ./src/pagecache_collector.o ./src/executor.o ./src/global_map.o ./src/pagecache_collector.o ./src/daemon.o

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

install:
	@echo "Installing the kernel modules..."
	sudo insmod smart-madvise.ko

remove:
	@echo "Removing the kernel modules..."
	sudo rmmod smart_madvise

deploy:
	$(MAKE) all
	$(MAKE) remove
	$(MAKE) install

# .PHONY: build-collector clean-collector install-collector remove-collector
