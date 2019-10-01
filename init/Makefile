OBJS:=main.o

init: $(OBJS)
	$(CC) $(OBJS) -o init -static -lcarbon
all: init

install: init
	mkdir -p $(DESTDIR)/sbin
	cp init $(DESTDIR)/sbin

clean:
	rm -f $(OBJS)