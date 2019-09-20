OBJS:=main.o

CFLAGS:=-v

init: $(OBJS)
	$(CC) $(OBJS) -o init -static
all: init

install: init
	mkdir -p $(DESTDIR)/sbin
	cp init $(DESTDIR)/sbin

clean:
	rm -f $(OBJS)