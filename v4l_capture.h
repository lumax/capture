/*
v4l_capture.h
Bastian Ruppert
*/

#ifndef __V4L_CAPTURE_H__
#define __V4L_CAPTURE_H__

extern void cap_setZoom(int Zoom);
extern void cap_init(SDL_Surface * surface,	\
		     unsigned int camWidth,	\
		     unsigned int camHeight,	\
		     int zoom,\
		     int Pixelformat);
extern void cap_uninit();

#endif /* __V4L_CAPTURE_H__*/
