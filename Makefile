obj-m += smart-madvise-kprobe.o
# obj-m += smart-madvise-syscalltable.o
KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) clean
