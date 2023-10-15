CC=g++

CFLAGS=-std=gnu++20 -fdiagnostics-color=always

SDIR = src

LDFLAGS += -LC:/msys64/mingw64\bin
LDFLAGS += -DSFML_STATIC
#LIBS= -Wl,-Bstatic -static-libstdc++ -static-libgcc -lsfml-audio-s -lsfml-network-s -lsfml-system-s -lwinmm -lflac -lvorbisenc -lvorbisfile -lvorbis -logg -lssl -lcrypto -lws2_32 -lwsock32 -lshlwapi -lopus -lprotobuf -Wl,-Bdynamic -lopenal 
LIBS= -lsfml-audio-s -lsfml-network-s -lsfml-system-s -lwinmm -lflac -lvorbisenc -lvorbisfile -lvorbis -logg -lssl -lcrypto -lws2_32 -lwsock32 -lshlwapi -lopus -lprotobuf -lopenal

_SRCS=data.cpp soundmanager.cpp soundmanagerRecorder.cpp player.cpp smbPitchShift.cpp opusmanager.cpp socketUdp.cpp protoParse.cpp crmd.cpp
_SRCSCC=proto/protocol.pb.cc
INCLUDE_FILES=crmd.h

SHARED_FLAG = -shared -fPIC
EXTRA_EXPORT_FLAG=
EXPORT_EXT=dll
# Diretório dos binários
BINDIR_REAL = bin/release
ODIR = obj/release
ifeq ($(MODE),debug)
    BINDIR_REAL = bin/debug
	CFLAGS += -g
	ODIR = obj/debug
	_SRCS += test.cpp
	SHARED_FLAG=
	STATIC=false
	LIBS += -lsfml-window
	EXTRA_EXPORT_FLAG =
	EXPORT_EXT=exe
else ifeq ($(MODE),release)
	BINDIR_REAL = bin/release
	ODIR = obj/release
	EXTRA_EXPORT_FLAG = -Wl,--out-implib,$(BINDIR_REAL)/crmd.a $(SDIR)/crmd.def
	EXPORT_EXT=dll
endif

_OBJS=$(subst .cpp,.o,$(_SRCS))
_OBJS_CC=$(subst .cc,.o,$(_SRCSCC))
OBJS_CPP=$(patsubst %,$(ODIR)/%,$(_OBJS))
OBJS_CC=$(patsubst %,$(ODIR)/%,$(_OBJS_CC))
SRCS=$(patsubst %,$(SDIR)/%,$(_SRCS))

_DEPFILES=$(subst .cpp,.d,$(_SRCS))
DEPFILES=$(patsubst %,$(ODIR)/%,$(_DEPFILES))

.PHONY: all

all: $(BINDIR_REAL)/crmd.$(EXPORT_EXT)

ifeq ($(STATIC),true)
	$(BINDIR_REAL)/crmd.$(EXPORT_EXT): $(OBJS_CPP) $(OBJS_CC)
	mkdir -p $(BINDIR_REAL)
	ar rcs -o $@ $(OBJS_CPP) $(OBJS_CC)
	@ldd $@ | grep "=> /" | grep -vE "/c/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | xargs -I '{}' cp -uf '{}' $(BINDIR_REAL)/ || true
else
$(BINDIR_REAL)/crmd.$(EXPORT_EXT): $(OBJS_CPP) $(OBJS_CC)
	mkdir -p $(BINDIR_REAL)
	$(CC) $(CFLAGS) $(SHARED_FLAG) $(LDFLAGS) $(OBJS_CPP) $(OBJS_CC) -o $@ $(LIBS) $(EXTRA_EXPORT_FLAG)
	@ldd $@ | grep "=> /" | grep -vE "/c/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | xargs -I '{}' cp -uf '{}' $(BINDIR_REAL)/ || true
	dlltool -d $(SDIR)/crmd.def -l $(BINDIR_REAL)/crmd.lib -D crmd.$(EXPORT_EXT)
endif

$(ODIR)/%.o: $(SDIR)/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -MMD -c $< -o $@ $(LIBS)

$(ODIR)/%.o: $(SDIR)/%.cc
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -MMD -c $< -o $@ $(LIBS)

#-include $(ODIR)/*.d
-include $(OBJS_CPP:.o=.d)
-include $(OBJS_CC:.o=.d)