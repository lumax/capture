PROJECT_NAME=v4l_capture
DEFS+=-D_GNU_SOURCE=1 -D_REENTRANT

INCLUDES+=-I. -I/usr/include/SDL
LIBS+=
#-L/usr/lib

CFLAGS+= 
LDFLAGS=-L.\
	-L$(ELDK_FS)/lib \
	-L$(ELDK_FS)/usr/lib \
	-L$(ELDK_FS)/usr/local/lib \
	-L$(STAGE)/lib \
	-L$(STAGE)/usr/local/lib \
	-Wl,-rpath-link -Wl,$(STAGE)/usr/local/lib \
	-Wl,-rpath -Wl,$(ELDK_FS)/usr/local/lib \


CFLAGS+=-g -c -Wall

# Use defines required by DSP/BIOS Link, leveraging file created by Link's build

#CFLAGS+=-DCOMPILE_DATE=\"$(shell date +%d.%m.%Y_%H:%M:%S)\"
#CFLAGS+=-DVERSIONSNUMMER=$(VERSIONSNUMMER)


LDFLAGS+=-lSDL -lSDL_image

OBJS+= main.o v4l_capture.o

ifdef CROSS_COMPILE
LIBS+=libc6_dsp_jpeg.lib
else
LDFLAGS+=-ldsp_jpeg
endif

include $(MAKE_DIR)/global.mak


public:
	cp $(PROJECT_NAME) $(ELDK_FS)/usr/work/capture/$(PROJECT_NAME)
	cp capture.c $(ELDK_FS)/usr/work/capture/$(PROJECT_NAME).c