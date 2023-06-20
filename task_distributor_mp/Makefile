CC = gcc
CFLAGS = -g

INCS = -I/usr/include -I./confparser
LDS = -L/usr/lib64 -luuid

SHLD = $(CC) $(CFLAGS)

OBJ = task_master
OBJ_CHILD = task_process

all: $(OBJ) $(OBJ_CHILD) 

$(OBJ): task_master.o klog.o kutils.o kconf.o koffset.o kfd.o \
		confparser/libconfparser.a
	$(SHLD) -o $(OBJ) task_master.o klog.o kutils.o kconf.o koffset.o kfd.o \
		confparser/libconfparser.a $(LDS)

$(OBJ_CHILD): task_process.o klog.o kutils.o kconf.o my_process.o \
		confparser/libconfparser.a
	$(SHLD) -o $(OBJ_CHILD) task_process.o klog.o kutils.o kconf.o my_process.o \
		confparser/libconfparser.a $(LDS)


%.o:%.c
	$(SHLD) -c $< $(INCS)

confparser/libconfparser.a:
	cd confparser; make -f linux.mk; cd -

clean:
	cd confparser; make -f linux.mk clean; cd -
	rm -rf *.o
	rm -rf $(OBJ)
	rm -rf $(OBJ_CHILD)