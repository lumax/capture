PROJECT_NAME=v4l_capture
DEFS+=-D_GNU_SOURCE=1 -D_REENTRANT

INCLUDES+=-I. -I/usr/include/SDL
LIBS+=
#-L/usr/lib

CFLAGS+= 
LDFLAGS=-L$(ELDK_FS)/lib \
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


LDFLAGS+=-lSDL -lSDL_image -ldsp_jpeg

OBJS = main.o v4l_capture.o
#dsp_jpeg.o
#color.o utils.o


include $(MAKE_DIR)/global.mak

public:
	cp $(PROJECT_NAME) $(ELDK_FS)/usr/work/capture/$(PROJECT_NAME)
	cp capture.c $(ELDK_FS)/usr/work/capture/$(PROJECT_NAME).c