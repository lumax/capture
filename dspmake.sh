#!/bin/bash
DSPCCDIR=/opt/TI/C6Run_0_95_02_02/bin
#CODESOURCELIBC=/opt/CodeSourcery/Sourcery_G++_Lite/arm-none-linux-gnueabi/libc/

#$DSPCCDIR/./c6runlib-cc -isystem $CODESOURCELIBC/usr/local/include -isystem /usr -isystem $CODESOURCELIBC/usr/include -I$CODESOURCELIBC/usr/include -c -o dsp_utils.o dsp_utils.c

echo "c6 cc:"
$DSPCCDIR/./c6runlib-cc -DC6COMPILE -c -o libc6_dsp_jpeg.o dsp_jpeg.c
echo "c6 ar:"
$DSPCCDIR/./c6runlib-ar rcs libc6_dsp_jpeg.lib libc6_dsp_jpeg.o


#$DSPCCDIR/./c6runlib-cc -isystem /opt/eldk/arm/usr/local/include -isystem /opt/eldk/arm/usr -isystem /opt/eldk/arm/usr/include -I/opt/eldk/arm/usr/include -c -o dsp_utils.o dsp_utils.c
#$DSPCCDIR/./c6runlib-cc -c -o dsp_color.o dsp_color.c
#$DSPCCDIR/./c6runlib-cc -c -o libdsp_capture.o dsp_capture.c