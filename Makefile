.POSIX:
CROSS   =
CC      = i686-w64-mingw32-gcc
STRIP = i686-w64-mingw32-strip
CFLAGS  = -m32 -Wl,--subsystem=windows:3.10 -DCURL_STATICLIB -DWIN32_LEAN_AND_MEAN -march=i486 -mtune=i486 -DAPI_KEY=\"$(shell echo $$APIKEYVAR)\"
CFLAGS	+= -Wl,--dynamicbase -s -DMAGICKCORE_HDRI_ENABLE=1 -DMAGICKCORE_QUANTUM_DEPTH=16 -D_LIB -D_MT -DMAGICKCORE_CHANNEL_MASK_DEPTH=32
CFLAGS  += -Os -std=gnu99 -fno-PIC -fno-ident -fno-stack-protector -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables -mpreferred-stack-boundary=2 -falign-functions=1 -falign-jumps=1 -falign-loops=1

LDFLAGS = -static -Wl,--start-group -lpthread -luser32 -lgdi32 -lkernel32 -no-pie -lshell32 -lcomdlg32 -lole32 -lssl -lcurl -lwsock32 -lws2_32 -lmincore -lcrypt32 -lMagick++ -lMagickCore -lMagickWand -llcms2 -lturbojpeg -lurlmon -lssp -lwolfssl -lpng -lz -Wl,--end-group

WINDRES = i686-w64-mingw32-windres

EXECUTABLE = pc98launch_edit.exe
ICO = myicon.ico

C_FILES = main.c curl.c downscale.c
OBJ_C	= $(notdir $(patsubst %.c, %.o, $(C_FILES)))
# icon.o

all: $(OBJ_C) $(EXECUTABLE)

# Rules to make executable
$(EXECUTABLE): $(OBJ_C)  
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $^ $(LDFLAGS)

$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<


all: $(EXECUTABLE)

#icon.o: $(ICO)
#	echo '1 ICON "$(ICO)"' | $(WINDRES) -o $@

clean:
	rm -f $(EXECUTABLE) *.o
#icon.o
