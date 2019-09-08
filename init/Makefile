OBJS:=main.o

init: $(OBJS)
	$(CC) $(OBJS) -o init -Wl,-e_start -nostdlib -ffreestanding
all: init

install: init
	mkdir -p $(DESTDIR)/sbin
	cp init $(DESTDIR)/sbin

clean:
	rm -f $(OBJS)