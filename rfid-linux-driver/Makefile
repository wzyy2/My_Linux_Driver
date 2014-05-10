ifneq ($(KERNELRELEASE),)
obj-m :=RFID-RC522.o
RFID-RC522-objs:=rc522.o rc522_api.o
else
KDIR :=/home/chen/friendlyarm
all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers
endif
