#
DESTDIR=
CFLAGS= -g -O2 -Wall -Wstrict-prototypes -Wpointer-arith -Wmissing-prototypes
CPPFLAGS=
LDFLAGS= -Wl,-z,defs
LDLIBFLAGS= -shared -Wl,-soname,$(TARGET)
TARGET=libnss_uidpool.so.2

all: $(TARGET)

$(TARGET): passwd.o
	$(CC) $(CFLAGS) $(LDLIBFLAGS) $(LDFLAGS) -o $@ $< -lc

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -c -o $@ $<

install:
	install -m755 -d $(DESTDIR)/lib/
	install -m644 $(TARGET) $(DESTDIR)/lib/$(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)

.PHONY: all install clean
