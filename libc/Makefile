ALL_OBJS:=$(patsubst %.c,%.o,$(wildcard */*.c)) \
$(patsubst %.cpp,%.o,$(wildcard */*.cpp))

CFLAGS+=-mno-red-zone -mno-sse -mno-sse2 -fPIC -DPIC

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
%.o: %.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) -fno-exceptions -fno-rtti -c $< -o $@

install-headers:
	cp -r include/*	$(DESTDIR)/usr/include

install: libk.a
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp libk.a $(DESTDIR)$(LIBDIR)
libk.a: $(ALL_OBJS)
	$(AR) rcs $@ $(ALL_OBJS)

clean:
	rm -f $(ALL_OBJS)
	rm -f libk.a
