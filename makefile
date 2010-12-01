NAME=mapper

current: pd_darwin

# ----------------------- NT -----------------------

pd_nt: $(NAME).dll

.SUFFIXES: .dll

PDNTCFLAGS = /W3 /WX /DNT /DPD /nologo
# VC="C:\Program Files\Microsoft Visual Studio\Vc98"
VC = "C:\Program Files\Microsoft Visual Studio 9.0\VC"
VSTK = "C:\Program Files\Microsoft SDKs\Windows\v6.0A"

PDNTINCLUDE = /I. /I..\..\src /I$(VC)\include

PDNTLDIR = $(VC)\lib
PDNTLIB = /NODEFAULTLIB:libcmt /NODEFAULTLIB:oldnames /NODEFAULTLIB:kernel32 \
	$(PDNTLDIR)\libcmt.lib $(PDNTLDIR)\oldnames.lib \
        $(VSTK)\lib\kernel32.lib \
	 ..\..\bin\pd.lib 

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /nologo /dll /export:$(CSYM)_setup $*.obj $(PDNTLIB)

# ----------------------- IRIX 5.x -----------------------

pd_irix5: $(NAME).pd_irix5

.SUFFIXES: .pd_irix5

SGICFLAGS5 = -o32 -DPD -DUNIX -DIRIX -O2

SGIINCLUDE =  -I../../src

.c.pd_irix5:
	$(CC) $(SGICFLAGS5) $(SGIINCLUDE) -o $*.o -c $*.c
	ld -elf -shared -rdata_shared -o $*.pd_irix5 $*.o
	rm $*.o

# ----------------------- IRIX 6.x -----------------------

pd_irix6: $(NAME).pd_irix6

.SUFFIXES: .pd_irix6

SGICFLAGS6 = -n32 -DPD -DUNIX -DIRIX -DN32 -woff 1080,1064,1185 \
	-OPT:roundoff=3 -OPT:IEEE_arithmetic=3 -OPT:cray_ivdep=true \
	-Ofast=ip32

.c.pd_irix6:
	$(CC) $(SGICFLAGS6) $(SGIINCLUDE) -o $*.o -c $*.c
	ld -n32 -IPA -shared -rdata_shared -o $*.pd_irix6 $*.o
	rm $*.o

# ----------------------- LINUX i386 -----------------------

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch $(CFLAGS)

# Override this for m_pd.h location
PDINCLUDE = -I$(HOME)/.local/include

LIBMAPPER_CFLAGS = $(shell pkg-config --cflags libmapper-0)
LIBMAPPER_LIBS = $(shell pkg-config --libs libmapper-0)

LINUXINCLUDE = $(PDINCLUDE) $(LIBMAPPER_CFLAGS)
LINUXLIBS = $(LIBMAPPER_LIBS)

.c.pd_linux:
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	$(CC) -shared -o $*.pd_linux $*.o $(LINUXLIBS)
	strip --strip-unneeded $*.pd_linux
	rm -f $*.o

# ----------------------- Mac OSX -----------------------

pd_darwin: $(NAME).pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch $(OPT_CFLAGS)

LIBMAPPER_CFLAGS = $(shell pkg-config --cflags libmapper-0)
LIBMAPPER_LIBS = $(shell pkg-config --libs libmapper-0)

.c.pd_darwin:
	$(CC) -arch i386 $(DARWINCFLAGS) $(LINUXINCLUDE) -I /Applications/Pd-extended.app/Contents/Resources/include -o $*.o -c $*.c $(LIBMAPPER_CFLAGS)
	$(CC) -arch i386 -bundle -undefined suppress -flat_namespace \
	    -o $*.pd_darwin $*.o $(LIBMAPPER_LIBS)
	rm -f $*.o

# ----------------------------------------------------------

clean:
	rm -f *.o *.pd_* so_locations
