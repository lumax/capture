/*
v4l_capture.h
Bastian Ruppert
*/

#ifndef __V4L_CAPTURE_H__
#define __V4L_CAPTURE_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;

struct v4l_capture
{
  char *          dev_name;
  io_method	 io;
  int             fd;
  struct buffer * buffers;
  unsigned int    n_buffers;
  int cam;
  SDL_Surface * mainSurface;
  SDL_Overlay * sdlOverlay;
  SDL_Rect sdlRect;
  int camnumber;
  int camWidth;
  int camHeight;
};

  extern void cap_setZoom(int Zoom);
  extern void cap_init(SDL_Surface * surface,	\
		       unsigned int camWidth,	\
		       unsigned int camHeight,	\
		       int zoom,		\
		       int pixelFormat);
  extern int cap_cam_init(struct v4l_capture * cap,	\
			  char * path,			\
			  int Zoom);
  extern int cap_uninit(struct v4l_capture * cap);
  extern int capMain(int args, char ** argv);
  
#ifdef __cplusplus
}
#endif
#endif /* __V4L_CAPTURE_H__*/
