CC = gcc
CFLAGS = -g
INCS =
LDS =

SHLD = $(CC) $(CFLAGS)

OBJ = kcat
UNGZ = kungz

all: $(OBJ) $(UNGZ)

$(OBJ): kcat.o kutils.o
	$(SHLD) -o $(OBJ) kcat.o kutils.o

$(UNGZ): kungz.o
	$(SHLD) -o $(UNGZ) kungz.o -lz

%.o:%.c
	$(SHLD) -c $<

clean:
	rm -f *.o
	rm -f $(OBJ)
	rm -f $(UNGZ)

install:
	cp $(OBJ) /usr/bin/$(OBJ)
	cp $(UNGZ) /usr/bin/$(UNGZ)

uninstall:
	rm -f /usr/bin/$(OBJ)
	rm -f /usr/bin/$(UNGZ)