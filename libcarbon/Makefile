BINS:= libcarbon.a libcarbon.so
OBJS:=src/syscalls.o
CFLAGS:= -fPIC -DPIC

libcarbon.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

libcarbon.so: $(OBJS)
	$(CC) -shared -fPIC -DPIC $(OBJS) -o $@

install: $(BINS)
	mkdir -p $(DESTDIR)/usr
	mkdir -p $(DESTDIR)/usr/include
	mkdir -p $(DESTDIR)/usr/lib
	cp include/syscalls.h $(DESTDIR)/usr/include/carbon/
	cp libcarbon.a $(DESTDIR)/usr/lib
	cp libcarbon.so $(DESTDIR)/usr/lib
clean:
	rm -f $(BINS) $(OBJS)