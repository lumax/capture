PROJECT_NAME=v4l_capture
DEFS+=-D_GNU_SOURCE=1 -D_REENTRANT

INCLUDES+=-I. -I$(ELDK_FS)/usr/include -I$(ELDK_FS)/usr/include/SDL
#-I/usr/include/SDL
LIBS+=-lSDL -lSDL_image
#-L/usr/lib

CFLAGS+= 
LDFLAGS=-L.\
	-L$(ELDK_FS)/lib \
	-L$(ELDK_FS)/usr/lib \
	-L$(ELDK_FS)/usr/local/lib \
	-L$(STAGE)/lib \
	-L$(STAGE)/usr/local/lib \
	-L/usr/lib/i386-linux-gnu \
	-Wl,-rpath-link -Wl,$(STAGE)/usr/local/lib \
	-Wl,-rpath -Wl,$(ELDK_FS)/usr/local/lib \
	-Wl,-rpath-link -Wl,/usr/lib/i386-linux-gnu \
	-Wl,-rpath -Wl,/usr/lib/i386-linux-gnu \


CFLAGS+=-g -c -Wall

# Use defines required by DSP/BIOS Link, leveraging file created by Link's build

#CFLAGS+=-DCOMPILE_DATE=\"$(shell date +%d.%m.%Y_%H:%M:%S)\"
#CFLAGS+=-DVERSIONSNUMMER=$(VERSIONSNUMMER)


LDFLAGS+=

OBJS+= main.o v4l_capture.o

ifdef MAKE_EUMAX01_MJPEG_SUPPORT

ifdef CROSS_COMPILE
LIBS+=libc6_dsp_jpeg.lib
CFLAGS+= -DC6COMPILE
else
LDFLAGS+=-ldsp_jpeg
endif

endif

include $(MAKE_DIR)/global.mak


public:
	cp $(PROJECT_NAME) $(ELDK_FS)/usr/work/capture/$(PROJECT_NAME)
	cp capture.c $(ELDK_FS)/usr/work/capture/$(PROJECT_NAME).c