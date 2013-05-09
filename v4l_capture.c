/*
  Copyright 2010, 2011 Bastian Ruppert.
  based on the V4L2 video capture example
 
 This library is free software: you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library.  If not, see <http://www.gnu.org/licenses/>.

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>

#include <SDL/SDL.h>
#include <SDL_image.h>
#ifdef EUMAX01_MJPEG_SUPPORT
#include <dsp_jpeg.h>
#endif
#include "v4l_capture.h"

#define  MULTIPLIKATOR 1 

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define CAM_OTHER 0
#define CAM_LOGITEC 1



struct buffer {
        void *                  start;
        size_t                  length;
};



static void setOverlayArea(struct v4l_capture* cap,int Zoom);
static void errno_exit(const char * err);
static void errno_print(const char * err);
static int xioctl(struct v4l_capture * cap,int request,void * arg);
static void process_image(struct v4l_capture* cap,const void * p,int method,size_t len);
static void process_image2(struct v4l_capture* cap,const void * p,int method,size_t len);
static int read_frame(struct v4l_capture * cap);
static int stop_capturing(struct v4l_capture * cap);
static int start_capturing(struct v4l_capture * cap);
static int uninit_device(struct v4l_capture * cap);
static void init_read(struct v4l_capture * cap,unsigned int buffer_size);
static int open_device(struct v4l_capture * cap,int * fd);
static int init_device(struct v4l_capture * cap);
static int cap_cam_uninit(struct v4l_capture * cap);

static int PixFormat = 0;
static int Zoom = 0;
//static SDL_Surface * pCrossair;

static unsigned int camWidth;
static unsigned int camHeight;
static unsigned int sdlWidth;
static unsigned int sdlHeight;

static struct v4l_capture capt = {.camnumber=0};
static struct v4l_capture capt2 = {.camnumber=1};

static SDL_Overlay * theoverlay = 0; 
static SDL_Surface * mainSurface = 0;

/*static SDL_Surface * getCrossair()
{
  if(pCrossair)
    {
      return pCrossair;
    }
  else
    {
      SDL_Surface * tmp = 0;
      //normal
      tmp = IMG_Load("crossair.png");
      if(!tmp)
	{
	  return 0;
	}
      pCrossair = SDL_DisplayFormatAlpha(tmp);
      if(!pCrossair)
	{
	  SDL_FreeSurface(tmp);
	return 0;
	}
      SDL_FreeSurface(tmp);
      
      return pCrossair;
    }
}
*/
 /*
static SDL_Surface * getCrossair2()
{
  if(pCrossair)
    {
      return pCrossair;
    }
  else
    {
      SDL_PixelFormat * format = SDL_GetVideoSurface()->format;
      pCrossair=SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCALPHA,		\
			       300,					\
			       300,					\
				     24,	\
			       format->Rmask,				\
			       format->Gmask,				\
			       format->Bmask,				\
			       format->Amask);  
      if(!pCrossair)
	{
	  return 0;
	}
      Uint32 color = SDL_MapRGB(SDL_GetVideoSurface()->format,0xff,0x33,0xcc);
      SDL_Rect kreuz = {.x=50,.y=50,.w=30,.h=100};
      SDL_FillRect(pCrossair,&kreuz,color);
      return pCrossair;
    }
}
*/
 static void setOverlayArea(struct v4l_capture* cap,int Zoom)
{
  if(cap==0)
    return;

  cap->sdlRect.x = cap->mainSurface->w/2-cap->camWidth;
  cap->sdlRect.y = 0;
  cap->sdlRect.w = cap->camWidth*2;
  cap->sdlRect.h = cap->camHeight;

  if(Zoom)
    {
      cap->sdlRect.x = cap->sdlRect.x - cap->camWidth;
      cap->sdlRect.w = cap->sdlRect.w*2;
      cap->sdlRect.h = cap->sdlRect.h*2;
    }
}

void cap_cam_setOverlayBottomSide(int BottomSide)
{
  capt.sdlRect.y = -(capt.camHeight-BottomSide);
  capt2.sdlRect.y = -(capt2.camHeight-BottomSide);
}

void cap_setZoom(int Zoom)
{
  
}

static void errno_exit(const char *           s)
{
        fprintf (stderr, "%s error %d, %s\n",
                 s, errno, strerror (errno));

        exit (EXIT_FAILURE);
}

static void errno_print(const char *           s)
{
        fprintf (stderr, "%s error %d, %s\n",
                 s, errno, strerror (errno));
}

static int xioctl(struct v4l_capture * cap,int request,void * arg)
{
        int r;

        do r = ioctl (cap->fd, request, arg);
        while (-1 == r && EINTR == errno);

        return r;
}

static void process_image(struct v4l_capture* cap,const void * p,int method,size_t len)
{
  printf("pi");
  if(method==IO_METHOD_MMAP)
    {
SDL_RWops * rw;
SDL_Surface  * pSjpeg;
      if(CAM_LOGITEC==cap->cam)
	{
	  rw = SDL_RWFromConstMem(p,len);
	  if(0==rw)
	    {
	      printf("error SDL_RWFromConstMem\n");
	      errno_exit ("SDL stuff");
	    }
	  if(IMG_isJPG(rw))
	    {
	      // printf("sample.jpg is a JPG file.\n");
	    }
	  else
	    {
	      printf("sample.jpg is not a JPG file, or JPG support is not available.\n");
	      SDL_FreeRW(rw);
	      errno_exit ("SDL IMG_isJPG");
	    }

	  pSjpeg = IMG_LoadJPG_RW(rw);
	  
	  if(0==pSjpeg)
	    {
	      printf("IMG_LoadJPG_RW: %s\n", IMG_GetError());
	      SDL_FreeRW(rw);
	      errno_exit ("SDL IMG_isJPG");
	    }

	  /* Clean up after ourselves */
	  SDL_FreeRW(rw);

	  //SDL_LockSurface(mainSurface);
	  if(SDL_BlitSurface(pSjpeg, 0, cap->mainSurface,0)) 
	    {
	      printf("SDL_BlitSurface failed\n");
	      errno_exit ("SDL_BlitSurface");
	    }
	  //SDL_UnlockSurface(mainSurface);
	  SDL_Flip(cap->mainSurface);
	  SDL_FreeSurface(pSjpeg);
	  //SDL_DisplayYUVOverlay(sdlOverlay, &sdlRect);
	}
      else
	{

	  SDL_LockSurface(cap->mainSurface);
	  SDL_LockYUVOverlay(cap->sdlOverlay);
	  
	  memcpy(cap->sdlOverlay->pixels[0], p, len);
	  
	  SDL_UnlockYUVOverlay(cap->sdlOverlay);
	  SDL_UnlockSurface(cap->mainSurface);

	  SDL_DisplayYUVOverlay(cap->sdlOverlay, &cap->sdlRect);
	}
    }
  else if(method==IO_METHOD_USERPTR)
    {
        if(CAM_LOGITEC==cap->cam)
	{
	  SDL_RWops * rw;
	  SDL_Surface  * pSjpeg;
	  //SDL_Rect theRect;
	  rw = SDL_RWFromConstMem(p,len);
	  if(0==rw)
	    {
	      printf("error SDL_RWFromConstMem\n");
	      errno_exit ("SDL stuff");
	    }
	  if(IMG_isJPG(rw))
	    {
	      // printf("sample.jpg is a JPG file.\n");
	    }
	  else
	    {
	      printf("sample.jpg is not a JPG file, or JPG support is not available.\n");
	      SDL_FreeRW(rw);
	      errno_exit ("SDL IMG_isJPG");
	    }
	  pSjpeg = IMG_LoadJPG_RW(rw);
	  if(0==pSjpeg)
	    {
	      printf("IMG_LoadJPG_RW: %s\n", IMG_GetError());
	      SDL_FreeRW(rw);
	      errno_exit ("SDL IMG_isJPG");
	    }

	  /* Clean up after ourselves */
	  SDL_FreeRW(rw);

	  //SDL_LockSurface(mainSurface);
	  if(SDL_BlitSurface(pSjpeg, 0, cap->mainSurface,0)) 
	    {
	      printf("SDL_BlitSurface failed\n");
	      errno_exit ("SDL_BlitSurface");
	    }
	  //SDL_UnlockSurface(mainSurface);
	  SDL_Flip(cap->mainSurface);
	  SDL_FreeSurface(pSjpeg);
	  //SDL_DisplayYUVOverlay(sdlOverlay, &sdlRect);
	}    
    }
  else
    {
      fputc ('_', stdout);
        fflush (stdout);
    }
}

/***************************************************************/
static void processMJPEG(struct v4l_capture* cap,const void * p,int method,size_t len)
{
  static int counter =0;
  unsigned int i,ii;
  unsigned char *framebuffer;
  static unsigned char *fb0 = 0;
  static unsigned char *fb1 = 0;
#ifndef EUMAX01_MJPEG_SUPPORT
  printf("NO MJPEG support\n");
  return;
#endif
#ifdef C6COMPILE
  if(!fb0) {
    fb0 =
    (unsigned char *) C6RUN_MEM_calloc(1,
			     (size_t) cap->camWidth*(cap->camHeight +
						   8) * 2);
  }
  if(!fb1) {
    fb1 =
    (unsigned char *) C6RUN_MEM_calloc(1,
			     (size_t) cap->camWidth*(cap->camHeight +
						   8) * 2);
  }
#else
  if(!fb0) {
    fb0 =
    (unsigned char *) calloc(1,
			     (size_t) cap->camWidth*(cap->camHeight +
						   8) * 2);
  }
  if(!fb1) {
    fb1 =
    (unsigned char *) calloc(1,
			     (size_t) cap->camWidth*(cap->camHeight +
						   8) * 2);
  }
#endif /* C6COMPILE */

  //if(cap->camnumber)
  //  return;
  if(counter<=50)
    {
      counter++;
      return;
    }
  if(method==IO_METHOD_MMAP)
    {
      printf("pixelformat = MJPEG\n");

      if(cap->camnumber)
	{
	  framebuffer = fb1;
	}
      else
	{
	  framebuffer = fb0;
	}
      counter++;
#ifdef EUMAX01_MJPEG_SUPPORT
      printf("DSP_test %i + %i = %i\n",5,counter,dsp_test(5,counter));

	i = jpeg_decode(fb0,(unsigned char*)p,		\
		      &cap->camWidth,			\
				      &cap->camHeight);
#endif      
      SDL_LockSurface(cap->mainSurface);
      SDL_LockYUVOverlay(cap->sdlOverlay);

      int w = cap->camWidth;
      int h = cap->camHeight;
      int alles = 0;
      int cam = cap->camnumber;
      int wMalZwei = w*2;
      int wMalVier = w*4;
      int offset = cam*wMalZwei;

      //Fadenkreuz
      unsigned int crossBreite = cap->camWidth;
      unsigned int crossDicke = 2;
      int zeile = w*2;
      //unsigned int crossX = w/2-crossBreite/2;
      //unsigned int crossY = h/2-crossDicke;
      unsigned int crossX = cap->camCrossX;
      unsigned int crossY = cap->camHeight/4*3;

      int start = zeile*crossY;
      //int lineoffset = crossY*h*4;
      char * pc = (char *)framebuffer;
      
      //horizontale Linie
      for(i=0;i<crossDicke;i++)
	{
	  for(ii=0;ii<crossBreite*2;ii++)
	    {
	      pc[start+ii]=0x00;
	    }
	  start+=zeile;
	}
      //vertikale Linie
      start = (crossX*2);//+zeile*(crossBreite/2);
      for(i=0;i<h;i++)
	{
	  for(ii=0;ii<crossDicke*2;ii++)
	    {
	      pc[start+ii]=0x00;
	    }
	  start+=zeile;
	}
      
      /*  memcpy(cap->sdlOverlay->pixels[0], camCtrl->framebuffer,
	     cap->camWidth * (cap->camHeight) *2);
      */
      for(i=0;i<h;i++)
	{
	  memcpy(cap->sdlOverlay->pixels[0]+i*wMalVier+offset,	\
		 framebuffer+alles,\
		 wMalZwei);
	  alles += w*2;
	  }
      //printf("alles = %i, len = %i\n",alles,len);
      SDL_UnlockYUVOverlay(cap->sdlOverlay);
      SDL_UnlockSurface(cap->mainSurface);

      /*tmpRect.h=tmpRect.h*MULTIPLIKATOR;*2;//untereinander
	tmpRect.w=tmpRect.w*2*MULTIPLIKATOR;*4;//nebeneinander*/
      SDL_DisplayYUVOverlay(cap->sdlOverlay, &cap->sdlRect);
    }
  else if(method==IO_METHOD_USERPTR)
    {
      printf("IO_METHOD_USEPTR not supported in process_image2\n");
    }
  else
    {
      fputc ('_', stdout);
      fflush (stdout);
    }
}

static char leerbuf[1024];

/***************************************************************/
static void process_image2(struct v4l_capture* cap,const void * p,int method,size_t len)
{
  int i,ii;
  if(method==IO_METHOD_MMAP)
    {
      if(CAM_LOGITEC==cap->cam)
	{
	  printf("CAM_LOGITEC not supported in process_image2");
	}
      else
	{
	  SDL_LockSurface(cap->mainSurface);
	  SDL_LockYUVOverlay(cap->sdlOverlay);
	  
	  int w = cap->camWidth;
	  int h = cap->camHeight;
	  int alles = 0;
	  int cam = cap->camnumber;
	  int wMalZwei = w*2;
	  int wMalVier = w*4;
	  int offset = cam*wMalZwei;

	  //Fadenkreuz	  
	  unsigned int crossBreite = 88;
	  unsigned int crossDicke = 4;
	  int zeile = w*2;
	  //unsigned int crossX = w/2-crossBreite/2;
	  //unsigned int crossY = h/2-crossDicke;
	  unsigned int crossX = 100;
	  unsigned int crossY = 200;
	  if(cam)
	    {
	      crossX = 200;
	      crossY = 150;
	    }
	  else
	    {
	      crossX = 50;
	      crossY = 50;      
	    }
	  int start = crossX*2+zeile*crossY;
	  //int lineoffset = crossY*h*4;
	  char * pc = (char *)p;

	  //horizontale Linie
	  for(i=0;i<crossDicke;i++)
	    {
	      for(ii=0;ii<crossBreite*2;ii++)
		{
		  pc[start+ii]=0x00;
		}
	      start+=zeile;
	    }
	  //vertikale Linie
	  start = (crossX+crossBreite/2)*2+zeile*(crossY-crossBreite/2);
	  for(i=0;i<crossBreite;i++)
	    {
	      for(ii=0;ii<crossDicke*2;ii++)
		{
		  pc[start+ii]=0x00;
		}
	      start+=zeile;
	    }
	  
	  for(i=0;i<h;i++)
	    {
	      if(0)//i>148&&i<150)
		{
		  memcpy(cap->sdlOverlay->pixels[0]+i*wMalVier+offset,leerbuf, wMalZwei);
		}
	      else
		{
		  memcpy(cap->sdlOverlay->pixels[0]+i*wMalVier+offset,p+alles, wMalZwei);
		}
	      alles += w*2;
	    }
	  //printf("alles = %i, len = %i\n",alles,len);
	  SDL_UnlockYUVOverlay(cap->sdlOverlay);
	  SDL_UnlockSurface(cap->mainSurface);

	  /*tmpRect.h=tmpRect.h*MULTIPLIKATOR;*2;//untereinander
	  tmpRect.w=tmpRect.w*2*MULTIPLIKATOR;*4;//nebeneinander*/
	  SDL_DisplayYUVOverlay(cap->sdlOverlay, &cap->sdlRect);
	}
      }
  else if(method==IO_METHOD_USERPTR)
    {
      printf("IO_METHOD_USEPTR not supported in process_image2\n");
    }
  else
    {
      fputc ('_', stdout);
        fflush (stdout);
    }
}

/*! \brief read camera-frame and processs data
 *
 * \param camera The cam to read
 *  \return 1 on success, 0 on EAGAIN, -1 on error
 */
int cap_read_frame(int camera)
{
  if(camera)
    return read_frame(&capt2);
  else
    return read_frame(&capt);
}

static int read_frame(struct v4l_capture * cap)
{
        struct v4l2_buffer buf;
	unsigned int i;

	switch (cap->io) {
	case IO_METHOD_READ:
    		if (-1 == read (cap->fd, cap->buffers[0].start, cap->buffers[0].length)) {
            		switch (errno) {
            		case EAGAIN:
                    		return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("read");
			}
		}

    		(*cap->processFnk)(cap,cap->buffers[0].start,IO_METHOD_READ,cap->buffers[0].length);

		break;

	case IO_METHOD_MMAP:
		CLEAR (buf);

            	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            	buf.memory = V4L2_MEMORY_MMAP;

    		if (-1 == xioctl (cap, VIDIOC_DQBUF, &buf)) {
            		switch (errno) {
            		case EAGAIN:
                    		return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("VIDIOC_DQBUF");
			}
		}

                assert (buf.index < cap->n_buffers);

	        (*cap->processFnk)(cap,cap->buffers[buf.index].start,IO_METHOD_MMAP,cap->buffers[buf.index].length);

		if (-1 == xioctl (cap, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");

		break;

	case IO_METHOD_USERPTR:
		CLEAR (buf);

    		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl (cap, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit ("VIDIOC_DQBUF");
			}
		}

		for (i = 0; i < cap->n_buffers; ++i)
			if (buf.m.userptr == (unsigned long)cap->buffers[i].start
			    && buf.length ==cap->buffers[i].length)
				break;

		assert (i < cap->n_buffers);

    		process_image (cap,(void *) buf.m.userptr,IO_METHOD_USERPTR,buf.length);

		if (-1 == xioctl (cap, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");

		break;
	}

	return 1;
}

static int counter1 = 0;
static int counter2=0;

static void mainloop(struct v4l_capture * cap,	\
		     struct v4l_capture * cap2,int count)
{
        while (count-- > 0) {
                for (;;) {
                        fd_set fds;
                        struct timeval tv;
                        int r;

                        FD_ZERO (&fds);
                        FD_SET (cap->fd, &fds);
			if(cap2)
			  {
			    //FD_ZERO (&fds);
			    FD_SET (cap2->fd, &fds);
			  }

                        /* Timeout. */
                        tv.tv_sec = 10;
                        tv.tv_usec = 0;
			if(cap2)
			  r = select (cap2->fd + 1, &fds, NULL, NULL, &tv);
			else
			  r = select (cap->fd + 1, &fds, NULL, NULL, &tv);

                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;

                                errno_exit ("select");
                        }

                        if (0 == r) {
                                fprintf (stderr, "select timeout\n");
                                exit (EXIT_FAILURE);
                        }
			if(FD_ISSET(cap->fd,&fds))
			  {
			    counter1++;
			    fputc ('1', stdout);
			    fflush (stdout);
			    if (read_frame (cap))
			      break;
			  }
			if(cap2)
			  {
			    if(FD_ISSET(cap2->fd,&fds))
			      {
				counter2++;
				fputc ('2', stdout);
				 fflush (stdout);
				if (read_frame (cap2))
				  break;
			      }	    
			  }
	
			/* EAGAIN - continue select loop. */
                }
        }
}

static int stop_capturing(struct v4l_capture * cap)
{
        enum v4l2_buf_type type;

	switch (cap->io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (cap, VIDIOC_STREAMOFF, &type))
		  {
			errno_print ("VIDIOC_STREAMOFF");
			return -1;
		  }

		break;
	}
	return 0;
}

static int start_capturing(struct v4l_capture * cap)
{
        unsigned int i;
        enum v4l2_buf_type type;

	switch (cap->io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < cap->n_buffers; ++i) {
            		struct v4l2_buffer buf;

        		CLEAR (buf);

        		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        		buf.memory      = V4L2_MEMORY_MMAP;
        		buf.index       = i;

        		if (-1 == xioctl (cap, VIDIOC_QBUF, &buf))
			  {
			    errno_print ("VIDIOC_QBUF");
			    return -1;
			  }
		}
		
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (cap, VIDIOC_STREAMON, &type))
		  {
			errno_print ("VIDIOC_STREAMON");
			return -1;
		  }

		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < cap->n_buffers; ++i) {
            		struct v4l2_buffer buf;

        		CLEAR (buf);

        		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        		buf.memory      = V4L2_MEMORY_USERPTR;
			buf.index       = i;
			buf.m.userptr	= (unsigned long) cap->buffers[i].start;
			buf.length      = cap->buffers[i].length;

			if (-1 == xioctl (cap, VIDIOC_QBUF, &buf))
			  {
                    		errno_print ("VIDIOC_QBUF");
				return -1;
			  }
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (cap, VIDIOC_STREAMON, &type))
		  {
			errno_print ("VIDIOC_STREAMON");
			return -1;
		  }

		break;
	}
	return 0;
}

static int uninit_device(struct v4l_capture * cap)
{
        unsigned int i;

	switch (cap->io) {
	case IO_METHOD_READ:
		free (cap->buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < cap->n_buffers; ++i)
			if (-1 == munmap (cap->buffers[i].start, cap->buffers[i].length))
			  {
				errno_print ("munmap");
				return -1;
			  }
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < cap->n_buffers; ++i)
			free (cap->buffers[i].start);
		break;
	}

	free (cap->buffers);
	return 0;
}

static void init_read(struct v4l_capture * cap,unsigned int buffer_size)
{
#ifdef C6COMPILE
        cap->buffers = C6RUN_MEM_calloc (1, sizeof (*cap->buffers));
#else
        cap->buffers = calloc (1, sizeof (*cap->buffers));
#endif
        if (!cap->buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

	cap->buffers[0].length = buffer_size;
#ifdef C6COMPILE
	cap->buffers[0].start = C6RUN_MEM_malloc (buffer_size);
#else
	cap->buffers[0].start = malloc (buffer_size);	
#endif
	if (!cap->buffers[0].start) {
    		fprintf (stderr, "Out of memory\n");
            	exit (EXIT_FAILURE);
	}
}

static int init_mmap(struct v4l_capture * cap)
{
	struct v4l2_requestbuffers req;

        CLEAR (req);

        req.count               = 4;
        req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory              = V4L2_MEMORY_MMAP;

	if (-1 == xioctl (cap, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf (stderr, "%s does not support "
                                 "memory mapping\n", cap->dev_name);
                        return -1;
                } else {
                        errno_print ("VIDIOC_REQBUFS");
			return -1;
                }
        }

        if (req.count < 2) {
                fprintf (stderr, "Insufficient buffer memory on %s\n",
                         cap->dev_name);
                return -1;
        }

#ifdef C6COMPILE
	cap->buffers = C6RUN_MEM_calloc (req.count, sizeof (*cap->buffers));
#else
	cap->buffers = calloc (req.count, sizeof (*cap->buffers));
#endif

        if (!cap->buffers) {
                fprintf (stderr, "Out of memory\n");
                return -1;
        }

        for (cap->n_buffers = 0; cap->n_buffers < req.count; ++cap->n_buffers) {
                struct v4l2_buffer buf;

                CLEAR (buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = cap->n_buffers;

                if (-1 == xioctl (cap, VIDIOC_QUERYBUF, &buf))
		  {
                        errno_print ("VIDIOC_QUERYBUF");
			return -1;
		  }

                cap->buffers[cap->n_buffers].length = buf.length;
                cap->buffers[cap->n_buffers].start =
                        mmap (NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              cap->fd, buf.m.offset);

                if (MAP_FAILED == cap->buffers[cap->n_buffers].start)
		  {
		    errno_print ("mmap");
		    return -1;
		  }
        }
	return 0;
}

static void init_userp(struct v4l_capture * cap,unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
        unsigned int page_size;

        page_size = getpagesize ();
        buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

        CLEAR (req);

        req.count               = 4;
        req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory              = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl (cap, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf (stderr, "%s does not support "
                                 "user pointer i/o\n", cap->dev_name);
                        exit (EXIT_FAILURE);
                } else {
                        errno_exit ("VIDIOC_REQBUFS");
                }
        }

#ifdef C6COMPILE
	cap->buffers = C6RUN_MEM_calloc (4, sizeof (*cap->buffers));
#else
	cap->buffers = calloc (4, sizeof (*cap->buffers));
#endif

        if (!cap->buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

        for (cap->n_buffers = 0; cap->n_buffers < 4; ++cap->n_buffers) {
                cap->buffers[cap->n_buffers].length = buffer_size;
                cap->buffers[cap->n_buffers].start = memalign (/* boundary */ page_size,
                                                     buffer_size);

                if (!cap->buffers[cap->n_buffers].start) {
    			fprintf (stderr, "Out of memory\n");
            		exit (EXIT_FAILURE);
		}
        }
}

static int init_device(struct v4l_capture * cap)
{
        struct v4l2_capability capability;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
	unsigned int min;

        if (-1 == xioctl (cap, VIDIOC_QUERYCAP, &capability)) {
                if (EINVAL == errno) {
                        fprintf (stderr, "%s 1is no V4L2 device\n",
                                 cap->dev_name);
                        return -1;
                } else {
		  errno_print("VIDIOC_QUERYCAP");
		  return -1;
                }
        }

        if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf (stderr, "%s 2is no video capture device\n",
                         cap->dev_name);
                return -1;
        }

	switch (cap->io) {
	case IO_METHOD_READ:
		if (!(capability.capabilities & V4L2_CAP_READWRITE)) {
			fprintf (stderr, "%s does not support read i/o\n",
				 cap->dev_name);
			return -1;
		}

		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(capability.capabilities & V4L2_CAP_STREAMING)) {
			fprintf (stderr, "%s does not support streaming i/o\n",
				 cap->dev_name);
			return -1;
		}

		break;
	}


        /* Select video input, video standard and tune here. */

	CLEAR (cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl (cap, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl (cap, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {	
                /* Errors ignored. */
        }


        CLEAR (fmt);

        fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = cap->camWidth;//160;//320; 
        fmt.fmt.pix.height      = cap->camHeight;//120;//240;
	if(CAM_LOGITEC==cap->cam)
	  {
	    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
	  }
	else
	  {
	    if(PixFormat)
	      fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;//V4L2_PIX_FMT_YUYV;//V4L2_PIX_FMT_MJPEG;
	    else
	      fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	      
	  }
        fmt.fmt.pix.field       = V4L2_FIELD_NONE;//V4L2_FIELD_INTERLACED;
	//	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

        if (-1 == xioctl (cap, VIDIOC_S_FMT, &fmt))
	  {
	    errno_print ("VIDIOC_S_FMT");
	    return -1;
	  }

        /* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	switch (cap->io) {
	case IO_METHOD_READ:
	  init_read (cap,fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
	  if(init_mmap (cap))
	    {
	      return -1;
	    }
	      break;

	case IO_METHOD_USERPTR:
	  init_userp (cap,fmt.fmt.pix.sizeimage);
		break;
	}
	return 0;
}

static int
close_device                    (int * fd )
{
        if (-1 == close (*fd))
	  {
	    errno_print ("close");
	    return -1;
	  }

        *fd = -1;
	return 0;
}

static int open_device(struct v4l_capture * cap,int * fd)
{
        struct stat st; 

        if (-1 == stat (cap->dev_name, &st)) {
	  //fprintf (stderr, "Cannot identify '%s': %d, %s\n",
	  //             cap->dev_name, errno, strerror (errno));
                return -1;
        }

        if (!S_ISCHR (st.st_mode)) {
                fprintf (stderr, "%s is no device\n", cap->dev_name);
                return -1;
        }

        *fd = open (cap->dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == *fd) {
	  //                fprintf (stderr, "Cannot open '%s': %d, %s\n",
	  //             cap->dev_name, errno, strerror (errno));
                return -1;
        }
	return 0;
}

static void usage(FILE * fp,int argc,char ** argv)
{
        fprintf (fp,
                 "Usage: %s [options]\n\n"
                 "Options:\n"
                 "-d | --device name   Video device name [/dev/video0]\n"
                 "-h | --help          Print this message\n"
                 "-m | --mmap          Use memory mapped buffers\n"
                 "-r | --read          Use read() calls\n"
                 "-u | --userp         Use application allocated buffers\n"
		 "-c | --cam           Use logitec ebcam\n"
                 "",
		 argv[0]);
}

static const char short_options [] = "d:hmruc2j";

static const struct option
long_options [] = {
        { "device",     required_argument,      NULL,           'd' },
        { "help",       no_argument,            NULL,           'h' },
        { "mmap",       no_argument,            NULL,           'm' },
        { "read",       no_argument,            NULL,           'r' },
        { "userp",      no_argument,            NULL,           'u' },
	{ "cam",      no_argument,            NULL,           'c' },
	{ "2cams",      no_argument,            NULL,           '2' },
	{ "MJPEG",      no_argument,            NULL,           'j' },
        { 0, 0, 0, 0 }
};

#define SDLWIDTH 1024//1024//800
#define SDLHEIGHT 567//768//600

int capMain(int argc,char ** argv)
{
  int bpp = 32;
  SDL_Surface * mainSurface;
  int Pixelformat = 0;
  int DEVICES = 0;

  // Start SDL
  if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
      fprintf( stderr, "Failed to initialise SDL: %s\n", SDL_GetError() );
      return -1;
    }
  atexit( SDL_Quit );
  
  // Create a window the size of the display
  SDL_WM_SetCaption( "MBE", NULL );
  mainSurface = SDL_SetVideoMode( SDLWIDTH,
				  SDLHEIGHT,
				  bpp, SDL_HWSURFACE);//SDL_SWSURFACE );
  if ( mainSurface == NULL )
    {
      fprintf( stderr, "Failed to set video mode: %s\n", SDL_GetError() );
      return -1;
    }
  //    gettimeofday( &app->timestamp, NULL );
  /*SDL_Surface * tmp = getCrossair2();
  if(SDL_BlitSurface(tmp,0,mainSurface,0))
    {
      fprintf( stderr, "Failed to blit Surface: %s\n", SDL_GetError() );
      return -1; 
    }
  SDL_UpdateRect(mainSurface,0,0,0,0);
  */
  for (;;) {
    int index;
    int c;
    
    c = getopt_long (argc, argv,
		     short_options, long_options,
		     &index);
    
    if (-1 == c)
      break;
    
    switch (c) {
    case 0: /* getopt_long() flag */
      break;
      
    case 'd':
      capt.dev_name = optarg;
      break;
      
    case 'h':
      usage (stdout, argc, argv);
      exit (EXIT_SUCCESS);
      
    case 'm':
      capt.io = IO_METHOD_MMAP;
      break;
      
    case 'r':
      capt.io = IO_METHOD_READ;
      break;
      
    case 'u':
      capt.io = IO_METHOD_USERPTR;
      break;
    case 'c':
      capt.cam = CAM_LOGITEC;
      break;
    case '2':
      DEVICES=2;
      break;
    case 'j':
      Pixelformat=1;
      break;
      
      
    default:
      usage (stderr, argc, argv);
      exit (EXIT_FAILURE);
    }
  }

#define CAMWIDTH 352//640
#define CAMHEIGHT 288//480
#define DauerSelect 220

  cap_init(mainSurface,CAMWIDTH,CAMHEIGHT,SDLWIDTH,SDLHEIGHT,0,Pixelformat);
  void(*fnk)(struct v4l_capture*,const void *,int,size_t);

  if(Pixelformat)
    {
      fnk = processMJPEG;
    }
  else
    {
      fnk = process_image2;
    }

  if(2==DEVICES)
    {
      if(cap_cam_init(0,0,fnk)<0)
	{
	  printf("cap_cam_init for /dev/video0 failed!\n");
	  return -1;
	}

      if(cap_cam_init(1,0,fnk)<0)
	{
	  printf("cap_cam_init for /dev/video1 failed!\n");
	  return -1;
	}
      
      mainloop (&capt,&capt2,DauerSelect);
	 
      printf ("cam1 : %i, cam2 : %i",counter1,counter2);
      
            if(cap_uninit())
	printf("cap_uninit capt failed!\n");
      
    }
  else
    {
      if(cap_cam_init(0,0,fnk)<0)
	{
	  printf("cap_cam_init for /dev/video0 failed!\n");
	  return -1;
	}
      
      mainloop (&capt,0,DauerSelect);
	 
      printf ("cam1 : %i, cam2 : %i",counter1,counter2);
      
      if(cap_cam_uninit(&capt))
	printf("cap_uninit capt failed!\n");
      
    }

  exit (EXIT_SUCCESS);
  
  return 0;
}

void cap_init(SDL_Surface * surface,		\
	      unsigned int cam_Width,		\
	      unsigned int cam_Height,		\
	      unsigned int sdl_Width,		\
	      unsigned int sdl_Height,		\
	      int zoom,				\
	      int pixelFormat)
{
  PixFormat = pixelFormat;
  Zoom = zoom;
  mainSurface = surface;
  SDL_FreeYUVOverlay(theoverlay);
  camWidth = cam_Width;
  camHeight = cam_Height;
  sdlWidth = sdl_Width;
  sdlHeight = sdl_Height;
  theoverlay = SDL_CreateYUVOverlay(camWidth*2,			\
				    camHeight,			\
				    SDL_YUY2_OVERLAY,		\
				    mainSurface);  
}

int cap_cam_init(int camera,unsigned int CrossXLimit,		\
		 void(*fnk)(struct v4l_capture*,		\
			    const void *,			\
			    int method,				\
			    size_t len))
{
  int ret=0;
  struct v4l_capture * cap;
  if(camera)
    {
      cap = &capt2;
      cap->dev_name        = "/dev/video1";
    }
  else
    {
      cap = &capt;
      cap->dev_name        = "/dev/video0";
    }

  
  cap->io	= IO_METHOD_MMAP;
  cap->fd              = -1;
  cap->buffers         = 0;
  cap->n_buffers       = 0;
  cap->cam             =CAM_OTHER;
  cap->mainSurface = mainSurface;
  cap->sdlRect.w = 0;
  cap->sdlRect.h = 0;
  cap->sdlRect.x = 0;
  cap->sdlRect.y = 0;
  cap->processFnk = fnk;
  cap->camWidth = camWidth;
  cap->camHeight = camHeight;
  cap->camCrossXLimit = CrossXLimit;

  if(camera)
    {
      cap->camCrossX=(camWidth*2-sdlWidth)/2+(3*sdlWidth/4);
    }
  else
    {
      cap->camCrossX=(camWidth*2-sdlWidth)/2 + sdlWidth/4;
    }

  cap->sdlOverlay = theoverlay;

  setOverlayArea(cap,Zoom);

  ret = open_device (cap,&cap->fd);
  if(ret)
    return -1;

  ret = init_device (cap);
  if(ret)
    return -2;

  ret = start_capturing (cap);
  if(ret)
    return -3;
  
  return cap->fd;
}

int cap_uninit()
{
  if(cap_cam_uninit(&capt)||cap_cam_uninit(&capt2))
    {
      return -1;
    }
  return 0;
}

static int cap_cam_uninit(struct v4l_capture * cap)
{
  int ret = 0;
  if(theoverlay)
    {
      SDL_FreeYUVOverlay(theoverlay);
      theoverlay = 0;
    }
  
  ret = stop_capturing (cap);
  if(ret)
    {
      return -1;
    }
  
   ret = uninit_device (cap);
  if(ret)
    {
      return -2;
    }     
  
  ret = close_device (&cap->fd);
  if(ret)
    {
      return -3;
      }
  return 0;
}

int cap_cam_getCrossX(int camNumber)
{
  struct v4l_capture * cap;
  if(camNumber)
    {
      cap = &capt2;
    }
  else
    {
      cap = &capt;
    }
  return cap->camCrossX;  
}

void cap_cam_setCrossX(int camNumber,int val)
{
  struct v4l_capture * cap;
  if(camNumber)
    {
      cap = &capt2;
    }
  else
    {
      cap = &capt;
    }
  cap->camCrossX = val;
  if(cap->camCrossX < (2+cap->camCrossXLimit))
    {
      cap->camCrossX = (2+cap->camCrossXLimit);
    }
  else if(cap->camCrossX>cap->camWidth-(2+cap->camCrossXLimit))
    {
      cap->camCrossX = cap->camWidth-(2+cap->camCrossXLimit);
    }
}

void cap_cam_addCrossX(int camNumber,int summand)
{
  struct v4l_capture * cap;
  if(camNumber)
    {
      cap = &capt2;
    }
  else
    {
      cap = &capt;
    }
  cap->camCrossX +=summand;
  if(cap->camCrossX < (2+cap->camCrossXLimit))
    {
      cap->camCrossX = (2+cap->camCrossXLimit);
    }
  else if(cap->camCrossX>cap->camWidth-(2+cap->camCrossXLimit))
    {
      cap->camCrossX = cap->camWidth-(2+cap->camCrossXLimit);
    }
}

int cap_cam_enable50HzFilter(int fd)
{
  struct v4l2_queryctrl queryctrl;
  struct v4l2_control control;

  memset (&queryctrl, 0, sizeof (queryctrl));
  queryctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;

  if (-1 == ioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
    if (errno != EINVAL) {
      perror ("VIDIOC_QUERYCTRL");
      return -1;
    } else {
      printf ("V4L2_CID_POWER_LINE_FREQUENCY is not supported\n");
    }
  } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
    printf ("V4L2_CID_BRIGHTNESS is not supported\n");
  } else {
    memset (&control, 0, sizeof (control));
        control.id = V4L2_CID_POWER_LINE_FREQUENCY;
        control.value = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;

        if (-1 == ioctl (fd, VIDIOC_S_CTRL, &control)) {
	  perror ("VIDIOC_S_CTRL");
                return -1;
        }
  }
  return 0;
}

int cap_cam_getFd(int camNumber)
{
  struct v4l_capture * cap;
  if(camNumber)
    {
      cap = &capt2;
    }
  else
    {
      cap = &capt;
    }
  return cap->fd;
}
