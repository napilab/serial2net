obj-m += ./serial2net.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install: all
	@echo "Installing module..."
	sudo /sbin/rmmod serial2net 2>/dev/null || true
	sudo /sbin/insmod serial2net.ko
	@echo "Module installed."

uninstall:
	@echo "Removing module..."
	sudo /sbin/rmmod serial2net || true
	@echo "Module removed."
