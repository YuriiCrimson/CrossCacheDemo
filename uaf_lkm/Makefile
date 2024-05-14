obj-m += uaf_lkm.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

test:
	sudo dmesg -C
	sudo insmod uaf_lkm.ko
	sudo rmmod uaf_lkm.ko
	dmesg

