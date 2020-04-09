IDIR=../include
CC=gcc
CFLAGS=-I$(IDIR) -w
LINK=-pthread # links here

ODIR=.obj
TEMP=oss.log # all temp files

_DEPS = msgqueue.h shmry.h rnode.h rqueue.h resources.h shmsg.h spawnclock.h# all .h files
DEPS = $(patsubst %, $(IDIR)/%, $(_DEPS))

#########################################
_OBJ = master.o spawnclock.o rqueue.o rnode.o# all .o files
OBJ = $(patsubst %, $(ODIR)/%, $(_OBJ))

TARGET = oss # executable name
#########################################
_OB = child.o rqueue.o rnode.o# all .o files
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
