CC = gcc
CFLAGS = -g
INCS = 
LDS = 

SHLD = $(CC) $(CFLAGS)

OBJ = kgrep

all: $(OBJ)

$(OBJ): kgrep.o kutils.o
	$(SHLD) -o $(OBJ) kgrep.o kutils.o

%.o:%.c
	$(SHLD) -c $<

clean:
	rm -f *.o
	rm -f $(OBJ)

install:
	cp $(OBJ) /usr/bin/$(OBJ)

uninstall:
	rm -f /usr/bin/$(OBJ)
