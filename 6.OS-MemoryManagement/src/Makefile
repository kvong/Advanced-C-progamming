IDIR=../include
CC=gcc
CFLAGS=-I$(IDIR) -w
LINK=-pthread -lm # links here

ODIR=.obj
TEMP=oss.log # all temp files

_DEPS = queue.h node.h pnode.h pnode.h shmry.h shmsg.h# all .h files
DEPS = $(patsubst %, $(IDIR)/%, $(_DEPS))

#########################################
_OBJ = queue.o node.o master.o pqueue.o pnode.o spawnclock.o # all .o files
OBJ = $(patsubst %, $(ODIR)/%, $(_OBJ))

TARGET = oss # executable name
#########################################
_OB = child.o # all .o files
OB = $(patsubst %, $(ODIR)/%, $(_OB))

TARGE = dostuff

# To do multiple executable simply add the
# part above with name change

all: $(TARGET) $(TARGE)

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LINK)

$(TARGE): $(OB)
	$(CC) -o $@ $^ $(CFLAGS) $(LINK)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(TARGET) $(TARGE) $(TEMP)
