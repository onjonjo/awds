#ARCH	:= um

obj-m    := awds.o
awds-objs := awds_module.o

#KDIR default currently running kernel
KDIR := /lib/modules/$(shell uname -r)/build

#KDIR Laptop
#KDIR     := /home/johannes/uml/src/linux

#KDIR euk-lab81
#KDIR     := /scratch/jpfeiffe/uml/src/linux

#KDIR alix meshnodes euk-lab81
#KDIR     := /scratch/jpfeiffe/alix/linux

SRC      := $(shell pwd)

EXTRA_CFLAGS := -O -g -DDEBUG -Winline

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(SRC) modules

clean:
	rm -f *.*~
	rm -f *.o
	rm -f *.ko
	rm -rf .awds*
	rm -rf *.temp*
