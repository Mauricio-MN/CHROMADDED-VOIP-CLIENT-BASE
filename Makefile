CC=g++

CFLAGS=-std=gnu++20 -fdiagnostics-color=always

SDIR = src

LDFLAGS += -LC:/msys64/mingw64\bin
LDFLAGS += -DSFML_STATIC
LIBS= -Wl,-Bstatic -static-libstdc++ -static-libgcc -lsfml-audio-s -lsfml-network-s -lsfml-system-s -lwinmm -lflac -lvorbisenc -lvorbisfile -lvorbis -logg -lssl -lcrypto -lws2_32 -lwsock32 -lshlwapi -lopus -lprotobuf -Wl,-Bdynamic -lopenal 

_SRCS=cript.cpp soundmanager.cpp player.cpp SoundCustomBufferRecorder.cpp smbPitchShift.cpp opusmanager.cpp socketUdp.cpp protoParse.cpp crmd.cpp
_SRCSCC=proto/protocol.pb.cc
INCLUDE_FILES=crmd.h

# Diretório dos binários
BINDIR_REAL = bin/release
ODIR = obj/release
ifeq ($(MODE),debug)
    BINDIR_REAL = bin/debug
	CFLAGS += -g
	ODIR = obj/debug
else ifeq ($(MODE),release)
	BINDIR_REAL = bin/release
	ODIR = obj/release
endif

_OBJS=$(subst .cpp,.o,$(_SRCS))
_OBJS_CC=$(subst .cc,.o,$(_SRCSCC))
OBJS_CPP=$(patsubst %,$(ODIR)/%,$(_OBJS))
OBJS_CC=$(patsubst %,$(ODIR)/%,$(_OBJS_CC))
SRCS=$(patsubst %,$(SDIR)/%,$(_SRCS))

_DEPFILES=$(subst .cpp,.d,$(_SRCS))
DEPFILES=$(patsubst %,$(ODIR)/%,$(_DEPFILES))

TYPELIB_FLAG=
ifeq ($(DYNAMIC),true)
	TYPELIB_FLAG= -shared
endif

.PHONY: all

all: $(BINDIR_REAL)/crmd.dll

ifeq ($(DYNAMIC),true)
$(BINDIR_REAL)/crmd.dll: $(OBJS_CPP) $(OBJS_CC)
	mkdir -p $(BINDIR_REAL)
	$(CC) $(CFLAGS) -fPIC $(TYPELIB_FLAG) $(LDFLAGS) $(OBJS_CPP) $(OBJS_CC) -o $@ $(LIBS) -Wl,--out-implib,$(BINDIR_REAL)/crmd.a
	@ldd $@ | grep "=> /" | grep -vE "/c/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | xargs -I '{}' cp -uf '{}' $(BINDIR_REAL)/ || true
else
$(BINDIR_REAL)/crmd.dll: $(OBJS_CPP) $(OBJS_CC)
	mkdir -p $(BINDIR_REAL)
	ar rcs -o $@ $(OBJS_CPP) $(OBJS_CC)
	@ldd $@ | grep "=> /" | grep -vE "/c/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | xargs -I '{}' cp -uf '{}' $(BINDIR_REAL)/ || true
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