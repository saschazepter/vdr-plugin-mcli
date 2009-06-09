#Comment this out to disable debugging output
#DEBUG=1
#VERBOSE=1
#WIN32=1
#API_SOCK=1
#VERBOSE=1
#BACKTRACE=1

ifdef RBMINI
  ARMEL=1
endif

APPLE_DARWIN = $(shell gcc -dumpmachine | grep -q 'apple-darwin' && echo "1" || echo "0")
CYGWIN = $(shell gcc -dumpmachine | grep -q 'cygwin' && echo "1" || echo "0")

DEFS=-DCLIENT -DLIBRARY -D_REENTRANT -D_GNU_SOURCE

ifeq ($(CYGWIN), 1)
WIN32=1
else
API_SOCK=1
endif

ifeq ($(APPLE_DARWIN), 1)
DEFS:=$(DEFS) -I../common/darwin/include/ -DAPPLE
APPLE=1
endif

include ../../../../../Make.config

ifdef ARMEL
  XML_INC := -I/usr/arm-linux-gnueabi/include/libxml2
  XML_LIB := -lxml2
  CROSS = arm-linux-gnueabi-
else
XML_INC:=`xml2-config --cflags`
XML_LIB:=`xml2-config --libs`
LIBRARY_PATH=/usr/lib
endif

ifeq ($(APPLE_DARWIN), 1)
CFLAGS:= $(CFLAGS) -fno-common -Wall -I../common $(DEFS)
else
CFLAGS:= $(CFLAGS) -Wall -I../common $(DEFS)
endif

ifdef BACKTRACE
CFLAGS:= $(CFLAGS) -DBACKTRACE -g
endif

ifdef API_SHM
LDFLAGS:= $(LDFLAGS) -lrt
CFLAGS:= $(CFLAGS) -DAPI_SHM
endif
ifdef API_SOCK
CFLAGS:= $(CFLAGS) -DAPI_SOCK
endif

ifdef VERBOSE
CFLAGS:= $(CFLAGS) -DDEBUG
DEBUG=1
endif

ifdef WIN32
CFLAGS:= -Iwin32/include $(CFLAGS) -mno-cygwin -DWIN32
LDFLAGS:= -Lwin32/lib $(LDFLAGS) -mno-cygwin
LDLIBS:= -lpthreadGC2 -lxml2 -lz -lws2_32 -liphlpapi 
else
CFLAGS:= $(CFLAGS)  -I../dvbloop $(XML_INC)
LDFLAGS:=$(LDFLAGS)
LDLIBS:=$(XML_LIB) -lpthread
endif

ifdef DEBUG
LDFLAGS:= $(LDFLAGS) -g -rdynamic
CFLAGS:= $(CFLAGS) -g -O0
else
CFLAGS:= $(CFLAGS) -O3
endif

MCLI = mcli

MCLI_OBJS= mld_common.o mld_client.o mld_reporter.o mcast.o recv_tv.o recv_ccpp.o tools.o tca_handler.o tra_handler.o satlists.o interfaces.o api_server.o ciparser.o ci_handler.o mmi_handler.o
ifdef WIN32
MCLI_OBJS := $(MCLI_OBJS) inet_pton.o inet_ntop.o inet_aton.o
else
MCLI_OBJS := $(MCLI_OBJS)
endif

MCLI_SOBJS = main.o
ifdef WIN32
MCLI_SOBJS := $(MCLI_SOBJS) dummy_client.o
else
ifdef APPLE
MCLI_SOBJS := $(MCLI_SOBJS) dummy_client.o input.o
else
MCLI_SOBJS := $(MCLI_SOBJS) dvblo_handler.o input.o
endif 
endif

all: lib$(MCLI)

static: $(MCLI)-static

$(MCLI): $(MCLI_OBJS) $(MCLI_SOBJS)
	$(CC) $(LDFLAGS) -o $@ $(MCLI_OBJS) $(MCLI_SOBJS) $(LDLIBS)
ifndef DEBUG
ifndef WIN32
	strip $(MCLI)
endif
endif

lib$(MCLI): $(MCLI_OBJS)
ifdef WIN32
	$(CC) $(LDFLAGS) -shared -o $@.dll $(MCLI_OBJS) $(LDLIBS) -Wl,--out-implib,$@.a -Wl,--output-def,$@.def
	lib /def:$@.def /machine:x86 /out:..\\common\\win32\\lib\\$@.lib
#	dlltool -k --dllname $@.dll --output-lib win32/lib/$@.lib --def $@.def
	cp -a $@.dll win32/lib/
	cp -a $@.a win32/lib/
	cp -a $@.def win32/lib/
endif
ifdef APPLE
	$(CC) $(LDFLAGS) -dynamiclib -o $@.dylib $(MCLI_OBJS) $(LDLIBS)
	$(AR) $(ARFLAGS) $@.a $(MCLI_OBJS)
else
	$(CC) $(LDFLAGS) -shared -o $@.so $(MCLI_OBJS) $(LDLIBS)
	$(AR) $(ARFLAGS) $@.a $(MCLI_OBJS)
endif


$(MCLI)-shared: lib$(MCLI)
	$(CC) -c $(CFLAGS) -ULIBRARY -o main.o main.c
ifdef WIN32
	$(CC) -c $(CFLAGS) -ULIBRARY -o dummy_client.o dummy_client.c
else
ifdef APPLE
	$(CC) -c $(CFLAGS) -ULIBRARY -o input.o input.c
	$(CC) -c $(CFLAGS) -ULIBRARY -o dummy_client.o dummy_client.c
else 
	$(CC) -c $(CFLAGS) -ULIBRARY -o input.o input.c
	$(CC) -c $(CFLAGS) -ULIBRARY -o dvblo_handler.o dvblo_handler.c
endif
endif
	$(CC) $(LDFLAGS) -o $@ $(MCLI_SOBJS) $(LDLIBS) -L. -lmcli

$(MCLI)-static: $(MCLI_OBJS) $(MCLI_SOBJS)
	$(CC) $(LDFLAGS) -static -static-libgcc -o $@ $(MCLI_OBJS) $(MCLI_SOBJS) $(LDLIBS) $(LIBRARY_PATH)/libxml2.a $(LIBRARY_PATH)/libz.a $(LIBRARY_PATH)/libm.a $(LIBRARY_PATH)/libpthread.a
ifndef DEBUG 
ifndef WIN32
	strip $(MCLI)-static
endif
endif

api_shm_test.o: api_shm_test.c
	$(CC) -c $(CFLAGS) -DUSE_SHM_API -o $@ $<
	
api_sock_test.o:api_sock_test.c
	$(CC) -c $(CFLAGS) -DUSE_SOCK_API -o $@ $<
	
$(MCLI)-shmtest: api_shm_test.o
	$(CC) $(LDFLAGS) -o $@ api_shm_test.o $(LDLIBS) -lrt

$(MCLI)-socktest: api_sock_test.o
	$(CC) $(LDFLAGS) -o $@ api_sock_test.o 
	
install: mcli
	install -p $< /usr/sbin/$<

install-lib: libmcli.la
	libtool --mode=install install $< /usr/local/lib/	

install-shared: mcli-shared
	libtool --mode=install install $< /usr/local/bin

depend:
	makedepend -Y -- $(CFLAGS)  -- *c >/dev/null 2>&1

clean:
	rm -f $(MCLI) $(MCLI)-* *.elf *.gdb *.o *.lo *.la *~ *.so *.a *.def *.dll *.dylib out.ts

mingw32:
	rm -rf mingw/*.c mingw/*.h mingw/win32
	cp *.c *.h mingw/
	mkdir mingw/win32
	cp -a win32/lib mingw/win32/
	cp -a win32/include mingw/win32/
	@echo "Created mingw directory - now ready to rumble... (call build.cmd)"
        
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.lo: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

# DO NOT DELETE

api_server.o: headers.h ../common/defs.h win32/include/pthread.h
api_server.o: win32/include/sched.h win32/include/zlib.h
api_server.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
api_server.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
api_server.o: win32/include/libxml/xmlversion.h
api_server.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
api_server.o: win32/include/libxml/xmlstring.h
api_server.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
api_server.o: win32/include/libxml/xmlmemory.h win32/include/libxml/threads.h
api_server.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
api_server.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
api_server.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
api_server.o: win32/include/libxml/xmlautomata.h
api_server.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
api_server.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
api_server.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
api_server.o: win32/include/libxml/xpath.h ../common/version.h
api_server.o: ../common/list.h ../common/satlists.h ../common/mcast.h
api_server.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
api_server.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
api_server.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
api_server.o: ../common/ciparser.h ci_handler.h
api_shm_test.o: headers.h ../common/defs.h win32/include/pthread.h
api_shm_test.o: win32/include/sched.h win32/include/zlib.h
api_shm_test.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
api_shm_test.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
api_shm_test.o: win32/include/libxml/xmlversion.h
api_shm_test.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
api_shm_test.o: win32/include/libxml/xmlstring.h
api_shm_test.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
api_shm_test.o: win32/include/libxml/xmlmemory.h
api_shm_test.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
api_shm_test.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
api_shm_test.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
api_shm_test.o: win32/include/libxml/list.h
api_shm_test.o: win32/include/libxml/xmlautomata.h
api_shm_test.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
api_shm_test.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
api_shm_test.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
api_shm_test.o: win32/include/libxml/xpath.h ../common/version.h
api_shm_test.o: ../common/list.h ../common/satlists.h ../common/mcast.h
api_shm_test.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
api_shm_test.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
api_shm_test.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
api_shm_test.o: ../common/ciparser.h ci_handler.h
api_sock_test.o: headers.h ../common/defs.h win32/include/pthread.h
api_sock_test.o: win32/include/sched.h win32/include/zlib.h
api_sock_test.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
api_sock_test.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
api_sock_test.o: win32/include/libxml/xmlversion.h
api_sock_test.o: win32/include/libxml/xmlexports.h
api_sock_test.o: win32/include/libxml/tree.h win32/include/libxml/xmlstring.h
api_sock_test.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
api_sock_test.o: win32/include/libxml/xmlmemory.h
api_sock_test.o: win32/include/libxml/threads.h
api_sock_test.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
api_sock_test.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
api_sock_test.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
api_sock_test.o: win32/include/libxml/xmlautomata.h
api_sock_test.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
api_sock_test.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
api_sock_test.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
api_sock_test.o: win32/include/libxml/xpath.h ../common/version.h
api_sock_test.o: ../common/list.h ../common/satlists.h ../common/mcast.h
api_sock_test.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
api_sock_test.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
api_sock_test.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
api_sock_test.o: ../common/ciparser.h ci_handler.h
api_test.o: headers.h ../common/defs.h win32/include/pthread.h
api_test.o: win32/include/sched.h win32/include/zlib.h win32/include/zconf.h
api_test.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
api_test.o: win32/include/libxml/encoding.h win32/include/libxml/xmlversion.h
api_test.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
api_test.o: win32/include/libxml/xmlstring.h win32/include/libxml/xmlregexp.h
api_test.o: win32/include/libxml/dict.h win32/include/libxml/xmlmemory.h
api_test.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
api_test.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
api_test.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
api_test.o: win32/include/libxml/list.h win32/include/libxml/xmlautomata.h
api_test.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
api_test.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
api_test.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
api_test.o: win32/include/libxml/xpath.h ../common/version.h ../common/list.h
api_test.o: ../common/satlists.h ../common/mcast.h ../common/input.h
api_test.o: ../common/recv_ccpp.h recv_tv.h ../common/tools.h
api_test.o: ../common/interfaces.h ../common/mld.h api_server.h tca_handler.h
api_test.o: tra_handler.h mld_reporter.h ../common/ciparser.h ci_handler.h
ci_handler.o: headers.h ../common/defs.h win32/include/pthread.h
ci_handler.o: win32/include/sched.h win32/include/zlib.h
ci_handler.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
ci_handler.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
ci_handler.o: win32/include/libxml/xmlversion.h
ci_handler.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
ci_handler.o: win32/include/libxml/xmlstring.h
ci_handler.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
ci_handler.o: win32/include/libxml/xmlmemory.h win32/include/libxml/threads.h
ci_handler.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
ci_handler.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
ci_handler.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
ci_handler.o: win32/include/libxml/xmlautomata.h
ci_handler.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
ci_handler.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
ci_handler.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
ci_handler.o: win32/include/libxml/xpath.h ../common/version.h
ci_handler.o: ../common/list.h ../common/satlists.h ../common/mcast.h
ci_handler.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
ci_handler.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
ci_handler.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
ci_handler.o: ../common/ciparser.h ci_handler.h
ciparser.o: headers.h ../common/defs.h win32/include/pthread.h
ciparser.o: win32/include/sched.h win32/include/zlib.h win32/include/zconf.h
ciparser.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
ciparser.o: win32/include/libxml/encoding.h win32/include/libxml/xmlversion.h
ciparser.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
ciparser.o: win32/include/libxml/xmlstring.h win32/include/libxml/xmlregexp.h
ciparser.o: win32/include/libxml/dict.h win32/include/libxml/xmlmemory.h
ciparser.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
ciparser.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
ciparser.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
ciparser.o: win32/include/libxml/list.h win32/include/libxml/xmlautomata.h
ciparser.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
ciparser.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
ciparser.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
ciparser.o: win32/include/libxml/xpath.h ../common/version.h ../common/list.h
ciparser.o: ../common/satlists.h ../common/mcast.h ../common/input.h
ciparser.o: ../common/recv_ccpp.h recv_tv.h ../common/tools.h
ciparser.o: ../common/interfaces.h ../common/mld.h api_server.h tca_handler.h
ciparser.o: tra_handler.h mld_reporter.h ../common/ciparser.h ci_handler.h
dummy_client.o: headers.h ../common/defs.h win32/include/pthread.h
dummy_client.o: win32/include/sched.h win32/include/zlib.h
dummy_client.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
dummy_client.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
dummy_client.o: win32/include/libxml/xmlversion.h
dummy_client.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
dummy_client.o: win32/include/libxml/xmlstring.h
dummy_client.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
dummy_client.o: win32/include/libxml/xmlmemory.h
dummy_client.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
dummy_client.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
dummy_client.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
dummy_client.o: win32/include/libxml/list.h
dummy_client.o: win32/include/libxml/xmlautomata.h
dummy_client.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
dummy_client.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
dummy_client.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
dummy_client.o: win32/include/libxml/xpath.h ../common/version.h
dummy_client.o: ../common/list.h ../common/satlists.h ../common/mcast.h
dummy_client.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
dummy_client.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
dummy_client.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
dummy_client.o: ../common/ciparser.h ci_handler.h
dvblo_handler.o: headers.h ../common/defs.h win32/include/pthread.h
dvblo_handler.o: win32/include/sched.h win32/include/zlib.h
dvblo_handler.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
dvblo_handler.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
dvblo_handler.o: win32/include/libxml/xmlversion.h
dvblo_handler.o: win32/include/libxml/xmlexports.h
dvblo_handler.o: win32/include/libxml/tree.h win32/include/libxml/xmlstring.h
dvblo_handler.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
dvblo_handler.o: win32/include/libxml/xmlmemory.h
dvblo_handler.o: win32/include/libxml/threads.h
dvblo_handler.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
dvblo_handler.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
dvblo_handler.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
dvblo_handler.o: win32/include/libxml/xmlautomata.h
dvblo_handler.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
dvblo_handler.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
dvblo_handler.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
dvblo_handler.o: win32/include/libxml/xpath.h ../common/version.h
dvblo_handler.o: ../common/list.h ../common/satlists.h ../common/mcast.h
dvblo_handler.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
dvblo_handler.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
dvblo_handler.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
dvblo_handler.o: ../common/ciparser.h ci_handler.h
inet_aton.o: ../common/defs.h win32/include/pthread.h win32/include/sched.h
inet_aton.o: win32/include/zlib.h win32/include/zconf.h
inet_aton.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
inet_aton.o: win32/include/libxml/encoding.h
inet_aton.o: win32/include/libxml/xmlversion.h
inet_aton.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
inet_aton.o: win32/include/libxml/xmlstring.h
inet_aton.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
inet_aton.o: win32/include/libxml/xmlmemory.h win32/include/libxml/threads.h
inet_aton.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
inet_aton.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
inet_aton.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
inet_aton.o: win32/include/libxml/xmlautomata.h
inet_aton.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
inet_aton.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
inet_aton.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
inet_aton.o: win32/include/libxml/xpath.h
inet_ntop.o: ../common/defs.h win32/include/pthread.h win32/include/sched.h
inet_ntop.o: win32/include/zlib.h win32/include/zconf.h
inet_ntop.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
inet_ntop.o: win32/include/libxml/encoding.h
inet_ntop.o: win32/include/libxml/xmlversion.h
inet_ntop.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
inet_ntop.o: win32/include/libxml/xmlstring.h
inet_ntop.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
inet_ntop.o: win32/include/libxml/xmlmemory.h win32/include/libxml/threads.h
inet_ntop.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
inet_ntop.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
inet_ntop.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
inet_ntop.o: win32/include/libxml/xmlautomata.h
inet_ntop.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
inet_ntop.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
inet_ntop.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
inet_ntop.o: win32/include/libxml/xpath.h
inet_pton.o: ../common/defs.h win32/include/pthread.h win32/include/sched.h
inet_pton.o: win32/include/zlib.h win32/include/zconf.h
inet_pton.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
inet_pton.o: win32/include/libxml/encoding.h
inet_pton.o: win32/include/libxml/xmlversion.h
inet_pton.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
inet_pton.o: win32/include/libxml/xmlstring.h
inet_pton.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
inet_pton.o: win32/include/libxml/xmlmemory.h win32/include/libxml/threads.h
inet_pton.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
inet_pton.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
inet_pton.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
inet_pton.o: win32/include/libxml/xmlautomata.h
inet_pton.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
inet_pton.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
inet_pton.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
inet_pton.o: win32/include/libxml/xpath.h
input.o: headers.h ../common/defs.h win32/include/pthread.h
input.o: win32/include/sched.h win32/include/zlib.h win32/include/zconf.h
input.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
input.o: win32/include/libxml/encoding.h win32/include/libxml/xmlversion.h
input.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
input.o: win32/include/libxml/xmlstring.h win32/include/libxml/xmlregexp.h
input.o: win32/include/libxml/dict.h win32/include/libxml/xmlmemory.h
input.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
input.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
input.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
input.o: win32/include/libxml/list.h win32/include/libxml/xmlautomata.h
input.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
input.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
input.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
input.o: win32/include/libxml/xpath.h ../common/version.h ../common/list.h
input.o: ../common/satlists.h ../common/mcast.h ../common/input.h
input.o: ../common/recv_ccpp.h recv_tv.h ../common/tools.h
input.o: ../common/interfaces.h ../common/mld.h api_server.h tca_handler.h
input.o: tra_handler.h mld_reporter.h ../common/ciparser.h ci_handler.h
interfaces.o: headers.h ../common/defs.h win32/include/pthread.h
interfaces.o: win32/include/sched.h win32/include/zlib.h
interfaces.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
interfaces.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
interfaces.o: win32/include/libxml/xmlversion.h
interfaces.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
interfaces.o: win32/include/libxml/xmlstring.h
interfaces.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
interfaces.o: win32/include/libxml/xmlmemory.h win32/include/libxml/threads.h
interfaces.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
interfaces.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
interfaces.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
interfaces.o: win32/include/libxml/xmlautomata.h
interfaces.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
interfaces.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
interfaces.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
interfaces.o: win32/include/libxml/xpath.h ../common/version.h
interfaces.o: ../common/list.h ../common/satlists.h ../common/mcast.h
interfaces.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
interfaces.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
interfaces.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
interfaces.o: ../common/ciparser.h ci_handler.h
main.o: headers.h ../common/defs.h win32/include/pthread.h
main.o: win32/include/sched.h win32/include/zlib.h win32/include/zconf.h
main.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
main.o: win32/include/libxml/encoding.h win32/include/libxml/xmlversion.h
main.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
main.o: win32/include/libxml/xmlstring.h win32/include/libxml/xmlregexp.h
main.o: win32/include/libxml/dict.h win32/include/libxml/xmlmemory.h
main.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
main.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
main.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
main.o: win32/include/libxml/list.h win32/include/libxml/xmlautomata.h
main.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
main.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
main.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
main.o: win32/include/libxml/xpath.h ../common/version.h ../common/list.h
main.o: ../common/satlists.h ../common/mcast.h ../common/input.h
main.o: ../common/recv_ccpp.h recv_tv.h ../common/tools.h
main.o: ../common/interfaces.h ../common/mld.h api_server.h tca_handler.h
main.o: tra_handler.h mld_reporter.h ../common/ciparser.h ci_handler.h
main.o: dummy_client.h
mcast.o: headers.h ../common/defs.h win32/include/pthread.h
mcast.o: win32/include/sched.h win32/include/zlib.h win32/include/zconf.h
mcast.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
mcast.o: win32/include/libxml/encoding.h win32/include/libxml/xmlversion.h
mcast.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
mcast.o: win32/include/libxml/xmlstring.h win32/include/libxml/xmlregexp.h
mcast.o: win32/include/libxml/dict.h win32/include/libxml/xmlmemory.h
mcast.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
mcast.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
mcast.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
mcast.o: win32/include/libxml/list.h win32/include/libxml/xmlautomata.h
mcast.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
mcast.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
mcast.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
mcast.o: win32/include/libxml/xpath.h ../common/version.h ../common/list.h
mcast.o: ../common/satlists.h ../common/mcast.h ../common/input.h
mcast.o: ../common/recv_ccpp.h recv_tv.h ../common/tools.h
mcast.o: ../common/interfaces.h ../common/mld.h api_server.h tca_handler.h
mcast.o: tra_handler.h mld_reporter.h ../common/ciparser.h ci_handler.h
mld_client.o: headers.h ../common/defs.h win32/include/pthread.h
mld_client.o: win32/include/sched.h win32/include/zlib.h
mld_client.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
mld_client.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
mld_client.o: win32/include/libxml/xmlversion.h
mld_client.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
mld_client.o: win32/include/libxml/xmlstring.h
mld_client.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
mld_client.o: win32/include/libxml/xmlmemory.h win32/include/libxml/threads.h
mld_client.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
mld_client.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
mld_client.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
mld_client.o: win32/include/libxml/xmlautomata.h
mld_client.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
mld_client.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
mld_client.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
mld_client.o: win32/include/libxml/xpath.h ../common/version.h
mld_client.o: ../common/list.h ../common/satlists.h ../common/mcast.h
mld_client.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
mld_client.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
mld_client.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
mld_client.o: ../common/ciparser.h ci_handler.h
mld_common.o: headers.h ../common/defs.h win32/include/pthread.h
mld_common.o: win32/include/sched.h win32/include/zlib.h
mld_common.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
mld_common.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
mld_common.o: win32/include/libxml/xmlversion.h
mld_common.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
mld_common.o: win32/include/libxml/xmlstring.h
mld_common.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
mld_common.o: win32/include/libxml/xmlmemory.h win32/include/libxml/threads.h
mld_common.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
mld_common.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
mld_common.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
mld_common.o: win32/include/libxml/xmlautomata.h
mld_common.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
mld_common.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
mld_common.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
mld_common.o: win32/include/libxml/xpath.h ../common/version.h
mld_common.o: ../common/list.h ../common/satlists.h ../common/mcast.h
mld_common.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
mld_common.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
mld_common.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
mld_common.o: ../common/ciparser.h ci_handler.h
mld_reporter.o: headers.h ../common/defs.h win32/include/pthread.h
mld_reporter.o: win32/include/sched.h win32/include/zlib.h
mld_reporter.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
mld_reporter.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
mld_reporter.o: win32/include/libxml/xmlversion.h
mld_reporter.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
mld_reporter.o: win32/include/libxml/xmlstring.h
mld_reporter.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
mld_reporter.o: win32/include/libxml/xmlmemory.h
mld_reporter.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
mld_reporter.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
mld_reporter.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
mld_reporter.o: win32/include/libxml/list.h
mld_reporter.o: win32/include/libxml/xmlautomata.h
mld_reporter.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
mld_reporter.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
mld_reporter.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
mld_reporter.o: win32/include/libxml/xpath.h ../common/version.h
mld_reporter.o: ../common/list.h ../common/satlists.h ../common/mcast.h
mld_reporter.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
mld_reporter.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
mld_reporter.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
mld_reporter.o: ../common/ciparser.h ci_handler.h
recv_ccpp.o: headers.h ../common/defs.h win32/include/pthread.h
recv_ccpp.o: win32/include/sched.h win32/include/zlib.h win32/include/zconf.h
recv_ccpp.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
recv_ccpp.o: win32/include/libxml/encoding.h
recv_ccpp.o: win32/include/libxml/xmlversion.h
recv_ccpp.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
recv_ccpp.o: win32/include/libxml/xmlstring.h
recv_ccpp.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
recv_ccpp.o: win32/include/libxml/xmlmemory.h win32/include/libxml/threads.h
recv_ccpp.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
recv_ccpp.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
recv_ccpp.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
recv_ccpp.o: win32/include/libxml/xmlautomata.h
recv_ccpp.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
recv_ccpp.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
recv_ccpp.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
recv_ccpp.o: win32/include/libxml/xpath.h ../common/version.h
recv_ccpp.o: ../common/list.h ../common/satlists.h ../common/mcast.h
recv_ccpp.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
recv_ccpp.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
recv_ccpp.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
recv_ccpp.o: ../common/ciparser.h ci_handler.h
recv_tv.o: headers.h ../common/defs.h win32/include/pthread.h
recv_tv.o: win32/include/sched.h win32/include/zlib.h win32/include/zconf.h
recv_tv.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
recv_tv.o: win32/include/libxml/encoding.h win32/include/libxml/xmlversion.h
recv_tv.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
recv_tv.o: win32/include/libxml/xmlstring.h win32/include/libxml/xmlregexp.h
recv_tv.o: win32/include/libxml/dict.h win32/include/libxml/xmlmemory.h
recv_tv.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
recv_tv.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
recv_tv.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
recv_tv.o: win32/include/libxml/list.h win32/include/libxml/xmlautomata.h
recv_tv.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
recv_tv.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
recv_tv.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
recv_tv.o: win32/include/libxml/xpath.h ../common/version.h ../common/list.h
recv_tv.o: ../common/satlists.h ../common/mcast.h ../common/input.h
recv_tv.o: ../common/recv_ccpp.h recv_tv.h ../common/tools.h
recv_tv.o: ../common/interfaces.h ../common/mld.h api_server.h tca_handler.h
recv_tv.o: tra_handler.h mld_reporter.h ../common/ciparser.h ci_handler.h
satlists.o: headers.h ../common/defs.h win32/include/pthread.h
satlists.o: win32/include/sched.h win32/include/zlib.h win32/include/zconf.h
satlists.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
satlists.o: win32/include/libxml/encoding.h win32/include/libxml/xmlversion.h
satlists.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
satlists.o: win32/include/libxml/xmlstring.h win32/include/libxml/xmlregexp.h
satlists.o: win32/include/libxml/dict.h win32/include/libxml/xmlmemory.h
satlists.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
satlists.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
satlists.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
satlists.o: win32/include/libxml/list.h win32/include/libxml/xmlautomata.h
satlists.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
satlists.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
satlists.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
satlists.o: win32/include/libxml/xpath.h ../common/version.h ../common/list.h
satlists.o: ../common/satlists.h ../common/mcast.h ../common/input.h
satlists.o: ../common/recv_ccpp.h recv_tv.h ../common/tools.h
satlists.o: ../common/interfaces.h ../common/mld.h api_server.h tca_handler.h
satlists.o: tra_handler.h mld_reporter.h ../common/ciparser.h ci_handler.h
sock_test.o: headers.h ../common/defs.h win32/include/pthread.h
sock_test.o: win32/include/sched.h win32/include/zlib.h win32/include/zconf.h
sock_test.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
sock_test.o: win32/include/libxml/encoding.h
sock_test.o: win32/include/libxml/xmlversion.h
sock_test.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
sock_test.o: win32/include/libxml/xmlstring.h
sock_test.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
sock_test.o: win32/include/libxml/xmlmemory.h win32/include/libxml/threads.h
sock_test.o: win32/include/libxml/globals.h win32/include/libxml/parser.h
sock_test.o: win32/include/libxml/hash.h win32/include/libxml/valid.h
sock_test.o: win32/include/libxml/xmlerror.h win32/include/libxml/list.h
sock_test.o: win32/include/libxml/xmlautomata.h
sock_test.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
sock_test.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
sock_test.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
sock_test.o: win32/include/libxml/xpath.h ../common/version.h
sock_test.o: ../common/list.h ../common/satlists.h ../common/mcast.h
sock_test.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
sock_test.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
sock_test.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
sock_test.o: ../common/ciparser.h ci_handler.h
tca_handler.o: headers.h ../common/defs.h win32/include/pthread.h
tca_handler.o: win32/include/sched.h win32/include/zlib.h
tca_handler.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
tca_handler.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
tca_handler.o: win32/include/libxml/xmlversion.h
tca_handler.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
tca_handler.o: win32/include/libxml/xmlstring.h
tca_handler.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
tca_handler.o: win32/include/libxml/xmlmemory.h
tca_handler.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
tca_handler.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
tca_handler.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
tca_handler.o: win32/include/libxml/list.h win32/include/libxml/xmlautomata.h
tca_handler.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
tca_handler.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
tca_handler.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
tca_handler.o: win32/include/libxml/xpath.h ../common/version.h
tca_handler.o: ../common/list.h ../common/satlists.h ../common/mcast.h
tca_handler.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
tca_handler.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
tca_handler.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
tca_handler.o: ../common/ciparser.h ci_handler.h
tools.o: headers.h ../common/defs.h win32/include/pthread.h
tools.o: win32/include/sched.h win32/include/zlib.h win32/include/zconf.h
tools.o: win32/include/linux/dvb/frontend.h win32/include/linux/dvb/ca.h
tools.o: win32/include/libxml/encoding.h win32/include/libxml/xmlversion.h
tools.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
tools.o: win32/include/libxml/xmlstring.h win32/include/libxml/xmlregexp.h
tools.o: win32/include/libxml/dict.h win32/include/libxml/xmlmemory.h
tools.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
tools.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
tools.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
tools.o: win32/include/libxml/list.h win32/include/libxml/xmlautomata.h
tools.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
tools.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
tools.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
tools.o: win32/include/libxml/xpath.h ../common/version.h ../common/list.h
tools.o: ../common/satlists.h ../common/mcast.h ../common/input.h
tools.o: ../common/recv_ccpp.h recv_tv.h ../common/tools.h
tools.o: ../common/interfaces.h ../common/mld.h api_server.h tca_handler.h
tools.o: tra_handler.h mld_reporter.h ../common/ciparser.h ci_handler.h
tra_handler.o: headers.h ../common/defs.h win32/include/pthread.h
tra_handler.o: win32/include/sched.h win32/include/zlib.h
tra_handler.o: win32/include/zconf.h win32/include/linux/dvb/frontend.h
tra_handler.o: win32/include/linux/dvb/ca.h win32/include/libxml/encoding.h
tra_handler.o: win32/include/libxml/xmlversion.h
tra_handler.o: win32/include/libxml/xmlexports.h win32/include/libxml/tree.h
tra_handler.o: win32/include/libxml/xmlstring.h
tra_handler.o: win32/include/libxml/xmlregexp.h win32/include/libxml/dict.h
tra_handler.o: win32/include/libxml/xmlmemory.h
tra_handler.o: win32/include/libxml/threads.h win32/include/libxml/globals.h
tra_handler.o: win32/include/libxml/parser.h win32/include/libxml/hash.h
tra_handler.o: win32/include/libxml/valid.h win32/include/libxml/xmlerror.h
tra_handler.o: win32/include/libxml/list.h win32/include/libxml/xmlautomata.h
tra_handler.o: win32/include/libxml/entities.h win32/include/libxml/xmlIO.h
tra_handler.o: win32/include/libxml/SAX.h win32/include/libxml/xlink.h
tra_handler.o: win32/include/libxml/SAX2.h win32/include/libxml/xmlwriter.h
tra_handler.o: win32/include/libxml/xpath.h ../common/version.h
tra_handler.o: ../common/list.h ../common/satlists.h ../common/mcast.h
tra_handler.o: ../common/input.h ../common/recv_ccpp.h recv_tv.h
tra_handler.o: ../common/tools.h ../common/interfaces.h ../common/mld.h
tra_handler.o: api_server.h tca_handler.h tra_handler.h mld_reporter.h
tra_handler.o: ../common/ciparser.h ci_handler.h
