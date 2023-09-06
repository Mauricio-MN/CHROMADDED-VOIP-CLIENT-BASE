CC=g++

SDIR = src

CFLAGS=-std=gnu++20 -fdiagnostics-color=always

LDFLAGS += -LC:/msys64/mingw64\bin
LDFLAGS += -DSFML_STATIC -static-libstdc++ -static-libgcc
LIBS=-lssl -lcrypto -lsfml-system -lsfml-audio -lsfml-window -llibopus

_SRCS=autopcv.cpp ocrmng.cpp inputmng.cpp coordParser.cpp itens.cpp pathfind.cpp maps/MapManager.cpp playerwalk.cpp coreMu.cpp pickonspot.cpp macro.cpp guiconfig.cpp typetext.cpp openglUtil.cpp hardware.cpp


# Diretório dos binários
ifeq ($(MODE),debug)
    BINDIR_REAL = bin/debug
	CFLAGS += -g
	ODIR = obj/debug
else ifeq ($(MODE),release)
	BINDIR_REAL = bin/release
	ODIR = obj/release
else ifeq ($(MODE),test)
	BINDIR_REAL = bin/test
	CFLAGS += -g
	ODIR = obj/test
	_SRCS += main.cpp
	_SRCS := $(filter-out autopcv.cpp,$(_SRCS))
	LDFLAGS += -DTESTS=\"test\"
	LDFLAGS += -Deu=\"true\"
	ifeq ($(RES),720)
		BINDIR_REAL = bin/test720
		ODIR = obj/test720
		LDFLAGS += -DSCREEN_720=\"true\"
	else
		LDFLAGS += -DSCREEN_768=\"true\"
	endif
else
    BINDIR_REAL = bin/release
	ODIR = obj/release
	ifeq ($(RES),720)
		BINDIR_REAL = bin/release720
		ODIR = obj/release720
		LDFLAGS += -DSCREEN_720=\"true\"
	else
		LDFLAGS += -DSCREEN_768=\"true\"
	endif
endif

_RESOURCESOBJ=$(subst .rc,.res,$(_RESOURCES))
RESOURCESOBJ=$(patsubst %,$(ODIR)/%,$(_RESOURCESOBJ))

_OBJS=$(subst .cpp,.o,$(_SRCS))
OBJS=$(patsubst %,$(ODIR)/%,$(_OBJS))
OBJS+=$(RESOURCESOBJ)
SRCS=$(patsubst %,$(SDIR)/%,$(_SRCS))

_DEPFILES=$(subst .cpp,.d,$(_SRCS))
DEPFILES=$(patsubst %,$(ODIR)/%,$(_DEPFILES))

.PHONY: all

all: $(BINDIR_REAL)/autopcv

$(BINDIR_REAL)/autopcv: $(OBJS)
	mkdir -p $(BINDIR_REAL)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
	@ldd $@ | grep "=> /" | grep -vE "/c/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | xargs -I '{}' cp -uf '{}' $(BINDIR_REAL)/ || true
	cp -r $(RESOURCEFOLDER)/* $(BINDIR_REAL)/

$(ODIR)/%.o: $(SDIR)/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -MMD -c $< -o $@

-include $(ODIR)/*.d

$(ODIR)/%.res: $(SDIR)/rc/%.rc
	windres $< -O coff -o $@