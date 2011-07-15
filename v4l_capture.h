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
  void(*processFnk)(struct v4l_capture*,			\
		    const void *,				\
		    int,					\
		    size_t);
  int camnumber;
  int camWidth;
  int camHeight;
  int camCrossX;
};

  extern void cap_setZoom(int Zoom);
  extern void cap_init(SDL_Surface * surface,	\
		       unsigned int camWidth,	\
		       unsigned int camHeight,	\
		       int zoom,		\
		       int pixelFormat);
  extern int cap_cam_init(int camera,void(*fnk)(struct v4l_capture*, \
						const void *,		\
						int,		\
						size_t));
  extern int cap_uninit();
  extern int capMain(int args, char ** argv);
  extern int cap_read_frame(int camera);
  extern void cap_cam_addCrossX(int camNumber,int summand);
  extern int cap_cam_enable50HzFilter(int camfd);
  
#ifdef __cplusplus
}
#endif
#endif /* __V4L_CAPTURE_H__*/
