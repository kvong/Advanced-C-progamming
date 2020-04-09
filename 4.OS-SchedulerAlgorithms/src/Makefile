IDIR=../include
CC=gcc
CFLAGS=-I$(IDIR) -w
LINK= # links here

ODIR=.obj
TEMP= Master.log# all temp files

_DEPS = prblock.h spawnclock.h msgqueue.h pqueue.h pnode.h# all .h files
DEPS = $(patsubst %, $(IDIR)/%, $(_DEPS))

#########################################
_OBJ = master.o spawnclock.o pqueue.o pnode.o# all .o files
OBJ = $(patsubst %, $(ODIR)/%, $(_OBJ))

TARGET = oss# executable name
#########################################
_OBJ2 = child.o spawnclock.o# all .o files
OBJ2 = $(patsubst %, $(ODIR)/%, $(_OBJ2))

TARGET2 = dostuff# executable name
#########################################

all: $(TARGET) $(TARGET2)

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(TARGET2): $(OBJ2)
	$(CC) -o $@ $^ $(CFLAGS)

$(TARGET3): $(OBJ3)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(TARGET) $(TARGET2) $(TEMP)
