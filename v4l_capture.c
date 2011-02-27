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

#define  MULTIPLIKATOR 1 

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define CAM_OTHER 0
#define CAM_LOGITEC 1

typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;

struct buffer {
        void *                  start;
        size_t                  length;
};

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

static int PixFormat = 0;
static SDL_Surface * pCrossair;

static void setOverlayArea(struct v4l_capture* cap,int Zoom);

static SDL_Surface * getCrossair()
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
				     24/*FSGDEFAULTCOLORDEPTH*/,	\
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

static void setOverlayArea(struct v4l_capture* cap,int Zoom)
{
  if(cap==0)
    return;

  cap->sdlRect.x = cap->mainSurface->w/2-cap->camWidth;
  cap->sdlRect.y = 10;
  cap->sdlRect.w = cap->camWidth*2;
  cap->sdlRect.h = cap->camHeight;

  if(Zoom)
    {
      cap->sdlRect.x = cap->sdlRect.x - cap->camWidth;
      cap->sdlRect.w = cap->sdlRect.w*2;
      cap->sdlRect.h = cap->sdlRect.h*2;
    }
}

/*void cap_setZoom(int Zoom)
{
  
}*/

void init_v4l_caputre(struct v4l_capture * cap,	\
		      int w,			\
		      int h,			\
		      int Zoom,			\
		      SDL_Surface * display,	\
		      SDL_Overlay * overlay	\
		      )
{
  cap->dev_name        = 0;
  cap->io	= IO_METHOD_MMAP;
  cap->fd              = -1;
  cap->buffers         = 0;
  cap->n_buffers       = 0;
  cap->cam             =CAM_OTHER;
  cap->mainSurface = display;
  cap->sdlRect.w = 0;
  cap->sdlRect.h = 0;
  cap->sdlRect.x = 0;
  cap->sdlRect.y = 0;
  cap->camWidth = w;
  cap->camHeight = h;
  
  cap->sdlOverlay = overlay;
  
  setOverlayArea(cap,Zoom);

}


static void errno_exit(const char *           s)
{
        fprintf (stderr, "%s error %d, %s\n",
                 s, errno, strerror (errno));

        exit (EXIT_FAILURE);
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
static char leerbuf[1024];

/***************************************************************/
static void process_image2(struct v4l_capture* cap,const void * p,int method,size_t len)
{
  int i;
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
	  for(i=0;i<h;i++)
	    {
	      if(i>148&&i<150)
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

    		process_image (cap,cap->buffers[0].start,IO_METHOD_READ,cap->buffers[0].length);

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

	        process_image2 (cap,cap->buffers[buf.index].start,IO_METHOD_MMAP,cap->buffers[buf.index].length);

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

static void stop_capturing(struct v4l_capture * cap)
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
			errno_exit ("VIDIOC_STREAMOFF");

		break;
	}
}

static void start_capturing(struct v4l_capture * cap)
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
                    		errno_exit ("VIDIOC_QBUF");
		}
		
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (cap, VIDIOC_STREAMON, &type))
			errno_exit ("VIDIOC_STREAMON");

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
                    		errno_exit ("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (cap, VIDIOC_STREAMON, &type))
			errno_exit ("VIDIOC_STREAMON");

		break;
	}
}

static void uninit_device(struct v4l_capture * cap)
{
        unsigned int i;

	switch (cap->io) {
	case IO_METHOD_READ:
		free (cap->buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < cap->n_buffers; ++i)
			if (-1 == munmap (cap->buffers[i].start, cap->buffers[i].length))
				errno_exit ("munmap");
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < cap->n_buffers; ++i)
			free (cap->buffers[i].start);
		break;
	}

	free (cap->buffers);
}

static void init_read(struct v4l_capture * cap,unsigned int buffer_size)
{
        cap->buffers = calloc (1, sizeof (*cap->buffers));

        if (!cap->buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

	cap->buffers[0].length = buffer_size;
	cap->buffers[0].start = malloc (buffer_size);

	if (!cap->buffers[0].start) {
    		fprintf (stderr, "Out of memory\n");
            	exit (EXIT_FAILURE);
	}
}

static void init_mmap(struct v4l_capture * cap)
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
                        exit (EXIT_FAILURE);
                } else {
                        errno_exit ("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf (stderr, "Insufficient buffer memory on %s\n",
                         cap->dev_name);
                exit (EXIT_FAILURE);
        }

        cap->buffers = calloc (req.count, sizeof (*cap->buffers));

        if (!cap->buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

        for (cap->n_buffers = 0; cap->n_buffers < req.count; ++cap->n_buffers) {
                struct v4l2_buffer buf;

                CLEAR (buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = cap->n_buffers;

                if (-1 == xioctl (cap, VIDIOC_QUERYBUF, &buf))
                        errno_exit ("VIDIOC_QUERYBUF");

                cap->buffers[cap->n_buffers].length = buf.length;
                cap->buffers[cap->n_buffers].start =
                        mmap (NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              cap->fd, buf.m.offset);

                if (MAP_FAILED == cap->buffers[cap->n_buffers].start)
                        errno_exit ("mmap");
        }
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

        cap->buffers = calloc (4, sizeof (*cap->buffers));

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

static void init_device(struct v4l_capture * cap)
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
                        exit (EXIT_FAILURE);
                } else {
                        errno_exit ("VIDIOC_QUERYCAP");
                }
        }

        if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf (stderr, "%s 2is no video capture device\n",
                         cap->dev_name);
                exit (EXIT_FAILURE);
        }

	switch (cap->io) {
	case IO_METHOD_READ:
		if (!(capability.capabilities & V4L2_CAP_READWRITE)) {
			fprintf (stderr, "%s does not support read i/o\n",
				 cap->dev_name);
			exit (EXIT_FAILURE);
		}

		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(capability.capabilities & V4L2_CAP_STREAMING)) {
			fprintf (stderr, "%s does not support streaming i/o\n",
				 cap->dev_name);
			exit (EXIT_FAILURE);
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
                errno_exit ("VIDIOC_S_FMT");

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
		init_mmap (cap);
		break;

	case IO_METHOD_USERPTR:
	  init_userp (cap,fmt.fmt.pix.sizeimage);
		break;
	}
}

static void
close_device                    (int * fd )
{
        if (-1 == close (*fd))
	        errno_exit ("close");

        *fd = -1;
}

static void open_device(struct v4l_capture * cap,int * fd)
{
        struct stat st; 

        if (-1 == stat (cap->dev_name, &st)) {
                fprintf (stderr, "Cannot identify '%s': %d, %s\n",
                         cap->dev_name, errno, strerror (errno));
                exit (EXIT_FAILURE);
        }

        if (!S_ISCHR (st.st_mode)) {
                fprintf (stderr, "%s is no device\n", cap->dev_name);
                exit (EXIT_FAILURE);
        }

        *fd = open (cap->dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == *fd) {
                fprintf (stderr, "Cannot open '%s': %d, %s\n",
                         cap->dev_name, errno, strerror (errno));
                exit (EXIT_FAILURE);
        }
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

#define SDLWIDTH 1024//800
#define SDLHEIGHT 768//600

static struct v4l_capture capt;
static struct v4l_capture capt2;

static SDL_Surface * mainSurface;

static int DEVICES =1;

static SDL_Overlay * theoverlay= 0; 

int main(int argc,char ** argv)
{
  int bpp = 32;
  int i;
  struct v4l_capture acap[2];

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
  SDL_Surface * tmp = getCrossair2();
  if(SDL_BlitSurface(tmp,0,mainSurface,0))
    {
      fprintf( stderr, "Failed to blit Surface: %s\n", SDL_GetError() );
      return -1; 
    }
  SDL_UpdateRect(mainSurface,0,0,0,0);

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
      PixFormat=1;
      break;
      
      
    default:
      usage (stderr, argc, argv);
      exit (EXIT_FAILURE);
    }
  }


  acap[0]=capt;
  acap[1]=capt2;
#define CAMWIDTH 352
#define CAMHEIGHT 288
  #define DauerSelect 300
  
  theoverlay = SDL_CreateYUVOverlay(CAMWIDTH*2,		\
				    CAMHEIGHT,			\
				    SDL_YUY2_OVERLAY,		\
				    mainSurface);

  for(i=0;i<DEVICES;i++)
    { 
      init_v4l_caputre(&acap[i],			\
		       CAMWIDTH,			\
		       CAMHEIGHT,			\
		       0,\
		       mainSurface,			\
		       theoverlay);
    }

    //+++++++++++++++++++++++++++

  for(i=0;i<DEVICES;i++)
    { 
      acap[i].camnumber=i;
      if(i)
	acap[i].dev_name =  "/dev/video1";
      else
	acap[i].dev_name =  "/dev/video0";
    }
  
  for(i=0;i<DEVICES;i++)
    {
      open_device (&acap[i],&acap[i].fd);
    }
  
  for(i=0;i<DEVICES;i++)
    {  
      init_device (&acap[i]);
    }
  
  for(i=0;i<DEVICES;i++)
    {  
      start_capturing (&acap[i]);
    }  
  if(2==DEVICES)
    mainloop (&acap[0],&acap[1],DauerSelect);
  else
    mainloop (&acap[0],0,DauerSelect);

  printf ("cam1 : %i, cam2 : %i",counter1,counter2);

  for(i=0;i<DEVICES;i++)
    {  
      stop_capturing (&acap[i]);
    }
  for(i=0;i<DEVICES;i++)
    {  
      uninit_device (&acap[i]);
    }

   for(i=0;i<DEVICES;i++)
    { 
      close_device (&acap[i].fd);
    }
  
  exit (EXIT_SUCCESS);
  
  return 0;
}

