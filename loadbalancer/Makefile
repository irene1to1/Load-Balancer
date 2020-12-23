# Makefile

FLAGS   = -std=gnu11 -g -Wall -Wextra -Wpedantic -Wshadow
SOURCES = loadbalancer.c
OBJECTS = loadbalancer.o
EXEBIN  = loadbalancer

all: $(EXEBIN)

$(EXEBIN) : $(OBJECTS)
	gcc -pthread -o $(EXEBIN) $(OBJECTS)

$(OBJECTS) : $(SOURCES)
	gcc -c $(FLAGS) $(SOURCES)

clean :
	rm -f $(OBJECTS)


