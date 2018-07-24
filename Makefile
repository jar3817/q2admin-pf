-include .config

ifndef CPU
    CPU := $(shell uname -m | sed -e s/i.86/i386/ -e s/amd64/x86_64/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/ -e s/alpha/axp/)
endif

ifndef REV
    REV := $(shell git rev-list HEAD | wc -l)
endif

ifndef VER
    VER := $(REV)~$(shell git rev-parse --short HEAD)
endif

GLIB_CFLAGS := $(shell pkg-config --cflags glib-2.0)
GLIB_LDFLAGS := $(shell pkg-config --libs glib-2.0)

CC ?= gcc
LD ?= ld
WINDRES ?= windres
STRIP ?= strip
RM ?= rm -f

CFLAGS ?= -O3 $(GLIB_CFLAGS)
LDFLAGS ?= -S -shared $(GLIB_LDFLAGS)
LIBS ?= -lcurl -lm -ldl

ifdef CONFIG_WINDOWS
    LDFLAGS += -mconsole
    LDFLAGS += -Wl,--nxcompat,--dynamicbase
else
    CFLAGS += -fPIC -ffast-math -w
    LDFLAGS += 
endif

CFLAGS += -DLINUX -DQ2A_VERSION='"$(VER)"' -DQ2A_REVISION=$(REV) 
RCFLAGS += -DQ2A_VERSION='\"$(VER)\"' -DQ2A_REVISION=$(REV)

HEADERS := game.h g_file.h g_local.h g_remote.h q_shared.h regex.h

OBJS := g_anticheat.o \
		g_ban.o \
		g_base64.o \
		g_checkvar.o \
		g_cmd.o \
		g_disable.o \
		g_file.o \
		g_flood.o \
		g_hashlist.o \
		g_init.o \
		g_libc.o \
		g_log.o \
		g_lrcon.o \
		g_main.o \
		g_mdfour.o \
		g_queue.o \
		g_remote.o \
		g_spawn.o \
		g_util.o \
		g_vote.o \
		g_zbot.o \
		g_zbotcheck.o \
		regex.o

ifdef CONFIG_SQLITE
    SQLITE_CFLAGS ?=
    SQLITE_LIBS ?= -lsqlite3
    CFLAGS += -DUSE_SQLITE=1 $(SQLITE_CFLAGS)
    LIBS += $(SQLITE_LIBS)
    OBJS += g_sqlite.o
endif

ifdef CONFIG_WINDOWS
    OBJS += openffa.o
    TARGET := game$(CPU).dll
else
    LIBS += -lm
    TARGET := game$(CPU).so
endif

all: $(TARGET)

default: all

.PHONY: all default clean strip

# Define V=1 to show command line.
ifdef V
    Q :=
    E := @true
else
    Q := @
    E := @echo
endif

-include $(OBJS:.o=.d)

%.o: %.c $(HEADERS)
	$(E) [CC] $@
	$(Q)$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.rc
	$(E) [RC] $@
	$(Q)$(WINDRES) $(RCFLAGS) -o $@ $<

$(TARGET): $(OBJS)
	$(E) [LD] $@
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(E) [CLEAN]
	$(Q)$(RM) *.o *.d $(TARGET)

strip: $(TARGET)
	$(E) [STRIP]
	$(Q)$(STRIP) $(TARGET)
