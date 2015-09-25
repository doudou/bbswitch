bbswitch-objs := bbswitch_proc.o bbswitch_nv.o

obj-m += bbswitch.o

ifdef DEBUG
CFLAGS_$(obj-m) := -DDEBUG
endif

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
PWD := "$$(pwd)"

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) O=$(PWD) -C $(KDIR) M=$(PWD) clean
