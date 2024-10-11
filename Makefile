CC=g++

CFLAGS=-std=gnu++20 -fdiagnostics-color=always

SDIR = src

ifeq ($(i686),true)
	CC="C:/msys6432/mingw32/bin/g++"
	LDFLAGS += -LC:/msys6432/mingw32/bin
	LDFLAGS += -LC:/msys6432/mingw32/lib
	LDFLAGS += -IC:/msys6432/mingw32/include
else
	LDFLAGS += -LC:/msys64/mingw64/bin
endif

LDFLAGS += -DSFML_STATIC -DAPI_EXPORT
#LIBS= -Wl,-Bstatic -static-libstdc++ -static-libgcc -lsfml-audio-s -lsfml-network-s -lsfml-system-s -lwinmm -lflac -lvorbisenc -lvorbisfile -lvorbis -logg -lssl -lcrypto -lws2_32 -lwsock32 -lshlwapi -lopus -lprotobuf -Wl,-Bdynamic -lopenal 
LIBS= -lsfml-audio-s -lsfml-network-s -lsfml-system-s -lopenal -lprotobuf
LIBS+= -labsl_base -labsl_log_internal_message -labsl_log_internal_check_op
LIBS+= -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread 
LIBS+= -lflac -lvorbisenc -lvorbisfile -lvorbis -logg -lssl -lcrypto -lopus
LIBS+= $(pkg-config --libs --cflags --static -lflac -lvorbisenc -lvorbisfile -lvorbis -logg -lssl -lcrypto -lopus )
LIBS+= -lwinmm -lws2_32 -lwsock32 -lshlwapi -lcrypt32
LIBS+=-Wl,-Bdynamic

_SRCS=replayGain.cpp WinDump.cpp smbPitchShift.cpp data.cpp soundmanager.cpp soundmanagerRecorder.cpp player.cpp opusmanager.cpp socketUdp.cpp protoParse.cpp crmd.cpp
_SRCSCC=proto/protocol.pb.cc
INCLUDE_FILES=crmd.h

SHARED_FLAG = -shared -fPIC
EXTRA_EXPORT_FLAG=
EXPORT_EXT=dll
# Diretório dos binários
BINDIR_REAL = bin/release
ODIR = obj/release
ifeq ($(OPT),3)
	CFLAGS += -O3
endif

LDDUTIL=ldd

ifeq ($(NEEDDBG),true)
	LDFLAGS += -DNEED_DGB
	CFLAGS += -g
	LIBS += -lexchndl
endif

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
	ifeq ($(i686),true)
		BINDIR_REAL = bin/debug32
		ODIR = obj/debug32
		CFLAGS += -m32
		M_FLAG = -m32
		LDDUTIL=ntldd
	endif
else ifeq ($(MODE),release)
	BINDIR_REAL = bin/release
	ODIR = obj/release
	ifeq ($(i686),true)
		ODIR = obj/release32
		BINDIR_REAL = bin/release32
		CFLAGS += -m32
		M_FLAG = -m32
		LDDUTIL=ntldd
	endif
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
	@ $(LDDUTIL) $@ | grep "=> /" | grep -vE "/c/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | xargs -I '{}' cp -uf '{}' $(BINDIR_REAL)/ || true
else
$(BINDIR_REAL)/crmd.$(EXPORT_EXT): $(OBJS_CPP) $(OBJS_CC)
	mkdir -p $(BINDIR_REAL)
	$(CC) $(CFLAGS) $(SHARED_FLAG) $(LDFLAGS) $(OBJS_CPP) $(OBJS_CC) -o $@ $(LIBS) $(EXTRA_EXPORT_FLAG)
	$(LDDUTIL) ./$@ | sed 's,C:,/c,g' | sed 's,\\,/,g' | grep "=> " | grep -vE "/c/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | xargs -I '{}' cp -uf '{}' ./$(BINDIR_REAL)/crmd/ || true
	$(LDDUTIL) $(BINDIR_REAL)/crmd/*.dll | sed 's,C:,/c,g' | sed 's,\\,/,g' | grep "=> " | grep -vE "/c/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | xargs -I '{}' cp -uf '{}' $(BINDIR_REAL)/crmd/ || true
	cp -r $(SDIR)/crmd.h $(BINDIR_REAL)/
	cp -r $(SDIR)/crmd.def $(BINDIR_REAL)/
	cd $(BINDIR_REAL)
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