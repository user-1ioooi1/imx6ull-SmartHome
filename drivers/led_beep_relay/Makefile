KERNELDIR := /home/lcj/linux/linux/my_linux/my_linux

CURRENT_PATH := $(shell pwd)

obj-m := gpio_io_all.o

build: kernel_modules  #默认执行kernel_modules命令

kernel_modules:
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNELDIR) M=$(CURRENT_PATH) clean
