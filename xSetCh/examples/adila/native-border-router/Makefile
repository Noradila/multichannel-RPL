CONTIKI_PROJECT=border-router
all: $(CONTIKI_PROJECT)
APPS += slip-cmd adilaGVtest

CONTIKI=../../..

WITH_UIP6=1
UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL -DUIP_CONF_IPV6 -DWITH_UIP6

#linker optimizations
SMALL=1

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += border-router-cmds.c tun-bridge.c border-router-rdc.c \
slip-config.c slip-dev.c

#/home/adila/Desktop/multichannel-RPL/xSetCh/examples/adila/slip-radio/slip-radio-cc2420.c
#../../slip-radio/slip-radio-cc2420.c

WITH_WEBSERVER=1
ifeq ($(WITH_WEBSERVER),1)
CFLAGS += -DWEBSERVER=1
#PROJECT_SOURCEFILES += httpd-simple.c
else ifneq ($(WITH_WEBSERVER), 0)
APPS += $(WITH_WEBSERVER)
CFLAGS += -DWEBSERVER=2
endif

include $(CONTIKI)/Makefile.include

connect-router:	border-router.native
	#sudo ./border-router.native -a 127.0.0.1 -v6 aaaa::1/64
	sudo ./border-router.native -a whymper.ee.ethz.ch -p 50101 aaaa::1/64
