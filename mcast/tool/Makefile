#Comment this out to disable debugging output
#DEBUG = 1
#API_SOCK=1

APPLE_DARWIN = $(shell gcc -dumpmachine | grep -q 'apple-darwin' && echo "1" || echo "0")
CYGWIN = $(shell gcc -dumpmachine | grep -q 'cygwin' && echo "1" || echo "0")

DEFS=-DCLIENT -D_REENTRANT -D_GNU_SOURCE

ifeq ($(CYGWIN), 1)
WIN32=1
else
API_SOCK=1
endif

ifeq ($(APPLE_DARWIN), 1)
DEFS:=$(DEFS) -I../common/darwin/include/ -DAPPLE
APPLE=1
endif

LIBRARY_PATH=/usr/lib

XML_INC:=`xml2-config --cflags`
XML_LIB:=`xml2-config --libs`

CFLAGS:=-Os -Wall $(XML_INC) -I../dvbloop -I../common/ -I../client -Wall $(DEFS)
LDFLAGS:=$(XML_LIB) 
ifdef API_SHM
LDFLAGS:= $(LDFLAGS) -lrt
CFLAGS:= $(CFLAGS) -DAPI_SHM
endif
ifdef API_SOCK
CFLAGS:= $(CFLAGS) -DAPI_SOCK
endif

ifdef DEBUG
LDFLAGS:= $(LDFLAGS) -g
CFLAGS:= $(CFLAGS) -g -DDEBUG
endif

ifdef WIN32
CFLAGS:= $(CFLAGS) -DWIN32
endif


NETCVDIAG = netcvdiag
NETCVDIAG_OBJS = netcvdiag.o tools.o

NETCVUPDATE = netcvupdate
NETCVUPDATE_OBJS = netcvupdate.o

all: $(NETCVDIAG) $(NETCVUPDATE)

static: $(NETCVDIAG)-static $(NETCVUPDATE)-static

$(NETCVDIAG): $(NETCVDIAG_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(NETCVDIAG_OBJS) $(LDLIBS)
ifndef DEBUG
ifndef WIN32
	strip $(NETCVDIAG)
endif
endif

$(NETCVUPDATE): $(NETCVUPDATE_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(NETCVUPDATE_OBJS) $(LDLIBS)
ifndef DEBUG
ifndef WIN32
	strip $(NETCVUPDATE)
endif
endif

$(NETCVDIAG)-static: $(NETCVDIAG_OBJS)
	$(CC) $(LDFLAGS) -static -static-libgcc -o $@ $(NETCVDIAG_OBJS) $(LDLIBS) $(LIBRARY_PATH)/libm.a  $(LIBRARY_PATH)/libz.a
	strip $(NETCVDIAG)-static

$(NETCVUPDATE)-static: $(NETCVUPDATE_OBJS)
	$(CC) $(LDFLAGS) -static -static-libgcc -o $@ $(NETCVUPDATE_OBJS) $(LDLIBS) $(LIBRARY_PATH)/libm.a
	strip $(NETCVUPDATE)-static

install: $(NETCVDIAG) $(NETCVUPDATE)
	install -p $(NETCVDIAG) /usr/sbin/$(NETCVDIAG)
	install -p $(NETCVUPDATE) /usr/sbin/$(NETCVUPDATE)

depend:
	makedepend -Y -- $(CFLAGS)  -- *c >/dev/null 2>&1

clean:
	rm -f $(NETCVDIAG) $(NETCVUPDATE) *.elf *.gdb *.o *~

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

# DO NOT DELETE

netcvdiag.o: ../client/headers.h ../common/defs.h
netcvdiag.o: /usr/include/libxml2/libxml/encoding.h
netcvdiag.o: /usr/include/libxml2/libxml/xmlversion.h
netcvdiag.o: /usr/include/libxml2/libxml/xmlexports.h
netcvdiag.o: /usr/include/libxml2/libxml/tree.h
netcvdiag.o: /usr/include/libxml2/libxml/xmlstring.h
netcvdiag.o: /usr/include/libxml2/libxml/xmlregexp.h
netcvdiag.o: /usr/include/libxml2/libxml/dict.h
netcvdiag.o: /usr/include/libxml2/libxml/xmlmemory.h
netcvdiag.o: /usr/include/libxml2/libxml/threads.h
netcvdiag.o: /usr/include/libxml2/libxml/globals.h
netcvdiag.o: /usr/include/libxml2/libxml/parser.h
netcvdiag.o: /usr/include/libxml2/libxml/hash.h
netcvdiag.o: /usr/include/libxml2/libxml/valid.h
netcvdiag.o: /usr/include/libxml2/libxml/xmlerror.h
netcvdiag.o: /usr/include/libxml2/libxml/list.h
netcvdiag.o: /usr/include/libxml2/libxml/xmlautomata.h
netcvdiag.o: /usr/include/libxml2/libxml/entities.h
netcvdiag.o: /usr/include/libxml2/libxml/xmlIO.h
netcvdiag.o: /usr/include/libxml2/libxml/SAX.h
netcvdiag.o: /usr/include/libxml2/libxml/xlink.h
netcvdiag.o: /usr/include/libxml2/libxml/SAX2.h
netcvdiag.o: /usr/include/libxml2/libxml/xmlwriter.h
netcvdiag.o: /usr/include/libxml2/libxml/xpath.h ../common/version.h
netcvdiag.o: ../common/list.h ../common/satlists.h ../common/mcast.h
netcvdiag.o: ../common/input.h ../common/recv_ccpp.h ../client/recv_tv.h
netcvdiag.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
netcvdiag.o: ../client/api_server.h ../client/tca_handler.h
netcvdiag.o: ../client/tra_handler.h ../client/mld_reporter.h
netcvdiag.o: ../common/ciparser.h ../client/ci_handler.h
netcvdiag.o: ../client/mmi_handler.h
netcvupdate.o: ../client/headers.h ../common/defs.h
netcvupdate.o: /usr/include/libxml2/libxml/encoding.h
netcvupdate.o: /usr/include/libxml2/libxml/xmlversion.h
netcvupdate.o: /usr/include/libxml2/libxml/xmlexports.h
netcvupdate.o: /usr/include/libxml2/libxml/tree.h
netcvupdate.o: /usr/include/libxml2/libxml/xmlstring.h
netcvupdate.o: /usr/include/libxml2/libxml/xmlregexp.h
netcvupdate.o: /usr/include/libxml2/libxml/dict.h
netcvupdate.o: /usr/include/libxml2/libxml/xmlmemory.h
netcvupdate.o: /usr/include/libxml2/libxml/threads.h
netcvupdate.o: /usr/include/libxml2/libxml/globals.h
netcvupdate.o: /usr/include/libxml2/libxml/parser.h
netcvupdate.o: /usr/include/libxml2/libxml/hash.h
netcvupdate.o: /usr/include/libxml2/libxml/valid.h
netcvupdate.o: /usr/include/libxml2/libxml/xmlerror.h
netcvupdate.o: /usr/include/libxml2/libxml/list.h
netcvupdate.o: /usr/include/libxml2/libxml/xmlautomata.h
netcvupdate.o: /usr/include/libxml2/libxml/entities.h
netcvupdate.o: /usr/include/libxml2/libxml/xmlIO.h
netcvupdate.o: /usr/include/libxml2/libxml/SAX.h
netcvupdate.o: /usr/include/libxml2/libxml/xlink.h
netcvupdate.o: /usr/include/libxml2/libxml/SAX2.h
netcvupdate.o: /usr/include/libxml2/libxml/xmlwriter.h
netcvupdate.o: /usr/include/libxml2/libxml/xpath.h ../common/version.h
netcvupdate.o: ../common/list.h ../common/satlists.h ../common/mcast.h
netcvupdate.o: ../common/input.h ../common/recv_ccpp.h ../client/recv_tv.h
netcvupdate.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
netcvupdate.o: ../client/api_server.h ../client/tca_handler.h
netcvupdate.o: ../client/tra_handler.h ../client/mld_reporter.h
netcvupdate.o: ../common/ciparser.h ../client/ci_handler.h
netcvupdate.o: ../client/mmi_handler.h
tools.o: ../client/headers.h ../common/defs.h
tools.o: /usr/include/libxml2/libxml/encoding.h
tools.o: /usr/include/libxml2/libxml/xmlversion.h
tools.o: /usr/include/libxml2/libxml/xmlexports.h
tools.o: /usr/include/libxml2/libxml/tree.h
tools.o: /usr/include/libxml2/libxml/xmlstring.h
tools.o: /usr/include/libxml2/libxml/xmlregexp.h
tools.o: /usr/include/libxml2/libxml/dict.h
tools.o: /usr/include/libxml2/libxml/xmlmemory.h
tools.o: /usr/include/libxml2/libxml/threads.h
tools.o: /usr/include/libxml2/libxml/globals.h
tools.o: /usr/include/libxml2/libxml/parser.h
tools.o: /usr/include/libxml2/libxml/hash.h
tools.o: /usr/include/libxml2/libxml/valid.h
tools.o: /usr/include/libxml2/libxml/xmlerror.h
tools.o: /usr/include/libxml2/libxml/list.h
tools.o: /usr/include/libxml2/libxml/xmlautomata.h
tools.o: /usr/include/libxml2/libxml/entities.h
tools.o: /usr/include/libxml2/libxml/xmlIO.h
tools.o: /usr/include/libxml2/libxml/SAX.h
tools.o: /usr/include/libxml2/libxml/xlink.h
tools.o: /usr/include/libxml2/libxml/SAX2.h
tools.o: /usr/include/libxml2/libxml/xmlwriter.h
tools.o: /usr/include/libxml2/libxml/xpath.h ../common/version.h
tools.o: ../common/list.h ../common/satlists.h ../common/mcast.h
tools.o: ../common/input.h ../common/recv_ccpp.h ../client/recv_tv.h
tools.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
tools.o: ../client/api_server.h ../client/tca_handler.h
tools.o: ../client/tra_handler.h ../client/mld_reporter.h
tools.o: ../common/ciparser.h ../client/ci_handler.h ../client/mmi_handler.h
