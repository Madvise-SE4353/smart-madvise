obj-m += smart-madvise.o 
smart-madvise-objs += ./src/smart-madvise.o ./src/pagecache_collector.o ./src/executor.o ./src/global_map.o ./src/pagecache_collector.o

# obj-m += smart-madvise-syscalltable.o
ccflags-y := -I${PWD}/include
KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) clean

install:
	@echo "Installing the kernel modules..."
	if ! lsmod | grep -q smart_madvise; then \
		sudo insmod smart-madvise.ko; \
	else \
		echo "smart_madvise module is already loaded."; \
	fi

remove:
	@echo "Removing the kernel modules..."
	if lsmod | grep -q smart_madvise; then \
		sudo rmmod smart_madvise; \
	else \
		echo "smart_madvise module is not loaded."; \
	fi

deploy:
	$(MAKE) all
	$(MAKE) remove
	$(MAKE) install
