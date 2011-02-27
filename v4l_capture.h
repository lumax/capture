/*
v4l_capture.h
Bastian Ruppert
*/

#ifndef __V4L_CAPTURE_H__
#define __V4L_CAPTURE_H__
#ifdef __cplusplus
extern "C" {
#endif
extern void cap_setZoom(int Zoom);
extern void cap_init(SDL_Surface * surface,	\
		     unsigned int camWidth,	\
		     unsigned int camHeight,	\
		     int zoom,			\
		     int Pixelformat,\
		     int devices);
extern void cap_uninit();
extern int capMain(int args, char ** argv);

#ifdef __cplusplus
}
#endif
#endif /* __V4L_CAPTURE_H__*/
