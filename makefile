
ifdef SystemRoot
	RM = del /Q
	CP = copy
	FixPath = $(subst /,\,$1)
	PLATFLAGS = -lkernel32 libopenal/OpenAL32.dll -lmingw32
	LINKPATHS = -Llib
	COMPATHS = -Ilibopenal -Ilibogg-1.3.1/include -Ilibvorbis-1.3.3/include
	PLATPOST = $(CP) $(call FixPath,libopenal/OpenAL32.dll bin) & \
	           $(CP) $(call FixPath,libopenal/wrap_oal.dll bin) & \
	           $(CP) $(call FixPath,sgscript/bin/sgscript.dll bin)
	BINEXT=.exe
	LIBPFX=
	LIBEXT=.dll
	PICFLAG=
else
	RM = rm -f
	CP = cp
	FixPath = $1
	PLATFLAGS = -lopenal -ldl -Wl,-rpath,'$$ORIGIN' -Wl,-z,origin
	LINKPATHS = -Llib
	COMPATHS = -Ilibogg-1.3.1/include -Ilibvorbis-1.3.3/include
	PLATPOST = $(CP) $(call FixPath,sgscript/bin/libsgscript.so bin)
	BINEXT=
	LIBPFX=lib
	LIBEXT=.so
	PICFLAG= -fPIC
endif

SRCDIR=src
LIBDIR=lib
EXTDIR=ext
OUTDIR=bin
OBJDIR=obj

ifeq ($(arch),64)
	ARCHFLAGS= -m64
else
	ifeq ($(arch),32)
		ARCHFLAGS= -m32
	else
		ARCHFLAGS=
	endif
endif

ifeq ($(mode),release)
	CFLAGS = -O3 -Wall $(ARCHFLAGS)
else
	mode = debug
	CFLAGS = -D_DEBUG -g -Wall $(ARCHFLAGS)
endif



C2FLAGS = $(CFLAGS) -DBUILDING_SGS_AUDIO
LIBFLAGS = $(ARCHFLAGS) $(PICFLAG)

_DEPS = sa_sound.h
DEPS = $(patsubst %,$(SRCDIR)/%,$(_DEPS))

_OBJ = sa_main.o sa_sound.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))


$(OUTDIR)/sgs-audio$(LIBEXT): $(OBJ) $(LIBDIR)/libogg.a $(LIBDIR)/libvorbis.a
	$(MAKE) -C sgscript
	g++ -Wall -o $@ $(OBJ) $(C2FLAGS) -shared $(LINKPATHS) $(PLATFLAGS) -lvorbis -logg -lpthread -static-libgcc -static-libstdc++ $(PICFLAG) -Lsgscript/bin -lsgscript
	$(PLATPOST)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	g++ -c -o $@ $< $(COMPATHS) -Isgscript/src -Isgscript/ext/cpp $(C2FLAGS) $(PICFLAG)

.PHONY: clean
clean:
	$(MAKE) -C sgscript clean
	$(RM) $(call FixPath,$(OBJDIR)/*.o $(OUTDIR)/sgs*)




$(OBJDIR)/libogg_bitwise.o:
	gcc -c -o $@ libogg-1.3.1/src/bitwise.c -O3 -Ilibogg-1.3.1/include $(LIBFLAGS)
$(OBJDIR)/libogg_framing.o:
	gcc -c -o $@ libogg-1.3.1/src/framing.c -O3 -Ilibogg-1.3.1/include $(LIBFLAGS)
$(LIBDIR)/libogg.a: $(OBJDIR)/libogg_bitwise.o $(OBJDIR)/libogg_framing.o
	ar rcs $@ $?


_OBJ_VORBIS = analysis.o bitrate.o block.o codebook.o envelope.o \
floor0.o floor1.o info.o lookup.o lpc.o lsp.o mapping0.o mdct.o psy.o \
registry.o res0.o sharedbook.o smallft.o synthesis.o vorbisenc.o vorbisfile.o window.o
OBJ_VORBIS = $(patsubst %,$(OBJDIR)/%,$(_OBJ_VORBIS))

$(OBJDIR)/libvorbis_%.o: libvorbis-1.3.3/lib/%.c
	gcc -c -o $@ $< -Ilibvorbis-1.3.3/include -Ilibvorbis-1.3.3/lib -Ilibogg-1.3.1/include $(LIBFLAGS)
$(LIBDIR)/libvorbis.a: $(patsubst %,$(OBJDIR)/libvorbis_%,$(_OBJ_VORBIS))
	ar rcs $@ $?

