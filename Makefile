TARGETS = snmp-monitor
TESTS = snmp-get rrd-test

# CFLAGS += -O0 -g

all: $(TARGETS)

tests: $(TESTS)

clean:
	rm -f *.o $(TARGETS) $(TESTS)

PREFIX ?= /usr/local

install: $(TARGETS)
	install -D -d $(DESTDIR)/$(PREFIX)/bin
	install -s -m 0755 $^ $(DESTDIR)/$(PREFIX)/bin

snmp-get: LDFLAGS += -lnetsnmp

rrd-test: rrdb.o
rrd-test: LDFLAGS += -lrrd

snmp-monitor: rrdb.o
snmp-monitor: CFLAGS  += `pkg-config --cflags glib-2.0`
snmp-monitor: LDFLAGS += `pkg-config --libs   glib-2.0`
snmp-monitor: LDFLAGS += -lnetsnmp -lrrd
