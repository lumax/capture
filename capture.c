/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
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

#define CAM_OTHER 0
#define CAM_LOGITEC 1

#define THEWIDTH 800//640//1280//160
#define THEHEIGHT 448//480//720//120

#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;

struct buffer {
        void *                  start;
        size_t                  length;
};

static char *           dev_name        = NULL;
static io_method	io		= IO_METHOD_MMAP;
static int              fd              = -1;
struct buffer *         buffers         = NULL;
static unsigned int     n_buffers       = 0;

static int              cam             = 0;

static SDL_Surface * mainSurface;
static SDL_Overlay * sdlOverlay;
static SDL_Rect sdlRect = {.w=THEWIDTH,.h=THEHEIGHT};

int initSDL(void)
{

  /* v4l2_format fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = 640; 
        fmt.fmt.pix.height      = 480;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;V4L2_PIX_FMT_UYVY 
        fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
  */

  /*unsigned int bmask = 0x000000ff;
    unsigned int gmask = 0x0000ff00;
    unsigned int rmask = 0x00ff0000;
    unsigned int amask = 0xff000000;*/

    int bpp = 32;
    /*    switch ( app->frame->format )
    {
        case VIDEO_PALETTE_RGB24:
            bpp = 24;
            break;

        case VIDEO_PALETTE_RGB32:
            bpp = 32;
            break;

        default:
            fprintf( stderr, "Unsupported frame format! %u",
                     app->frame->format );
            return -1;
	    }*/

    // Start SDL

    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        fprintf( stderr, "Failed to initialise SDL: %s\n", SDL_GetError() );
        return -1;
    }
    atexit( SDL_Quit );

    // Create a window the size of the display

    SDL_WM_SetCaption( "camview", NULL );

    mainSurface = SDL_SetVideoMode( THEWIDTH,
				    THEHEIGHT,
                                    bpp, SDL_SWSURFACE );

    if ( mainSurface == NULL )
    {
        fprintf( stderr, "Failed to set video mode: %s\n", SDL_GetError() );
        return -1;
    }
    sdlOverlay = SDL_CreateYUVOverlay(THEWIDTH,THEHEIGHT,SDL_YVYU_OVERLAY ,mainSurface);
    printf("Overlay planes = %i\n",sdlOverlay->planes);
    //sdlOverlay->planes = 3;

    /*    if(initCustom("maske.bmp"))
      {
        fprintf( stderr, "Failed to create Custom Surface %s\n", SDL_GetError());
        return 1;	
	}*/

    // Ignore alpha channel, go opaque
    //    SDL_SetAlpha( app->image, 0, 0 );

    //    gettimeofday( &app->timestamp, NULL );
    return 0;
}


static void
errno_exit                      (const char *           s)
{
        fprintf (stderr, "%s error %d, %s\n",
                 s, errno, strerror (errno));

        exit (EXIT_FAILURE);
}

static int
xioctl                          (int                    fd,
                                 int                    request,
                                 void *                 arg)
{
        int r;

        do r = ioctl (fd, request, arg);
        while (-1 == r && EINTR == errno);

        return r;
}

static void
process_image                   (const void *           p,
				 int               method,size_t len)
{
  int i;
  if(method==IO_METHOD_MMAP)
    {
      if(CAM_LOGITEC==cam)
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

	  /*pSjpeg=IMG_Load("test.jpg");
	  if(!pSjpeg) {
	    printf("IMG_Load: %s\n", IMG_GetError());
	    errno_exit("IMG_Load failed\n");
	    }*/
	  
	  if(0==pSjpeg)
	    {
	      printf("IMG_LoadJPG_RW: %s\n", IMG_GetError());
	      SDL_FreeRW(rw);
	      errno_exit ("SDL IMG_isJPG");
	    }

	  /* Clean up after ourselves */
	  SDL_FreeRW(rw);

	  //SDL_LockSurface(mainSurface);
	  if(SDL_BlitSurface(pSjpeg, 0, mainSurface,0)) 
	    {
	      printf("SDL_BlitSurface failed\n");
	      errno_exit ("SDL_BlitSurface");
	    }
	  //SDL_UnlockSurface(mainSurface);
	  SDL_Flip(mainSurface);
	  SDL_FreeSurface(pSjpeg);
	  //SDL_DisplayYUVOverlay(sdlOverlay, &sdlRect);
	}
      else
	{
	  //printf("%i\n", p);
	  SDL_LockSurface(mainSurface);
	  SDL_LockYUVOverlay(sdlOverlay);
	  
	  memcpy(sdlOverlay->pixels[0], p, len);
	  
	  SDL_UnlockYUVOverlay(sdlOverlay);
	  SDL_UnlockSurface(mainSurface);
	  
	  SDL_DisplayYUVOverlay(sdlOverlay, &sdlRect);
	}
      /*
 app->image = SDL_CreateRGBSurfaceFrom( app->frame->data,
                                           app->frame->width,
                                           app->frame->height,
                                           bpp,
                                           app->frame->width * 4,
                                           rmask, gmask, bmask, amask);
       */

      /*
      SDL_LockYUVOverlay(sdlOverlay); 
      *sdlOverlay->pixels = p;
      SDL_UnlockYUVOverlay(sdlOverlay); 
      i = SDL_DisplayYUVOverlay(sdlOverlay,&sdlRect);
      */
       fputc ('.', stdout);
        fflush (stdout);
    }
  else if(method==IO_METHOD_USERPTR)
    {
        if(CAM_LOGITEC==cam)
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

	  /*pSjpeg=IMG_Load("test.jpg");
	  if(!pSjpeg) {
	    printf("IMG_Load: %s\n", IMG_GetError());
	    errno_exit("IMG_Load failed\n");
	    }*/
	  
	  if(0==pSjpeg)
	    {
	      printf("IMG_LoadJPG_RW: %s\n", IMG_GetError());
	      SDL_FreeRW(rw);
	      errno_exit ("SDL IMG_isJPG");
	    }

	  /* Clean up after ourselves */
	  SDL_FreeRW(rw);

	  //SDL_LockSurface(mainSurface);
	  if(SDL_BlitSurface(pSjpeg, 0, mainSurface,0)) 
	    {
	      printf("SDL_BlitSurface failed\n");
	      errno_exit ("SDL_BlitSurface");
	    }
	  //SDL_UnlockSurface(mainSurface);
	  SDL_Flip(mainSurface);
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

static int
read_frame			(void)
{
        struct v4l2_buffer buf;
	unsigned int i;

	switch (io) {
	case IO_METHOD_READ:
    		if (-1 == read (fd, buffers[0].start, buffers[0].length)) {
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

    		process_image (buffers[0].start,IO_METHOD_READ,buffers[0].length);

		break;

	case IO_METHOD_MMAP:
		CLEAR (buf);

            	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            	buf.memory = V4L2_MEMORY_MMAP;

    		if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
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

                assert (buf.index < n_buffers);

	        process_image (buffers[buf.index].start,IO_METHOD_MMAP,buffers[buf.index].length);

		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");

		break;

	case IO_METHOD_USERPTR:
		CLEAR (buf);

    		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
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

		for (i = 0; i < n_buffers; ++i)
			if (buf.m.userptr == (unsigned long) buffers[i].start
			    && buf.length == buffers[i].length)
				break;

		assert (i < n_buffers);

    		process_image ((void *) buf.m.userptr,IO_METHOD_USERPTR,buf.length);

		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");

		break;
	}

	return 1;
}

static void
mainloop                        (void)
{
	unsigned int count;

        count = 300;

        while (count-- > 0) {
                for (;;) {
                        fd_set fds;
                        struct timeval tv;
                        int r;

                        FD_ZERO (&fds);
                        FD_SET (fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 2;
                        tv.tv_usec = 0;

                        r = select (fd + 1, &fds, NULL, NULL, &tv);

                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;

                                errno_exit ("select");
                        }

                        if (0 == r) {
                                fprintf (stderr, "select timeout\n");
                                exit (EXIT_FAILURE);
                        }

			if (read_frame ())
                    		break;
	
			/* EAGAIN - continue select loop. */
                }
        }
}

static void
stop_capturing                  (void)
{
        enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
			errno_exit ("VIDIOC_STREAMOFF");

		break;
	}
}

static void
start_capturing                 (void)
{
        unsigned int i;
        enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i) {
            		struct v4l2_buffer buf;

        		CLEAR (buf);

        		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        		buf.memory      = V4L2_MEMORY_MMAP;
        		buf.index       = i;

        		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                    		errno_exit ("VIDIOC_QBUF");
		}
		
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
			errno_exit ("VIDIOC_STREAMON");

		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i) {
            		struct v4l2_buffer buf;

        		CLEAR (buf);

        		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        		buf.memory      = V4L2_MEMORY_USERPTR;
			buf.index       = i;
			buf.m.userptr	= (unsigned long) buffers[i].start;
			buf.length      = buffers[i].length;

			if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                    		errno_exit ("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
			errno_exit ("VIDIOC_STREAMON");

		break;
	}
}

static void
uninit_device                   (void)
{
        unsigned int i;

	switch (io) {
	case IO_METHOD_READ:
		free (buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i)
			if (-1 == munmap (buffers[i].start, buffers[i].length))
				errno_exit ("munmap");
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i)
			free (buffers[i].start);
		break;
	}

	free (buffers);
}

static void
init_read			(unsigned int		buffer_size)
{
        buffers = calloc (1, sizeof (*buffers));

        if (!buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

	buffers[0].length = buffer_size;
	buffers[0].start = malloc (buffer_size);

	if (!buffers[0].start) {
    		fprintf (stderr, "Out of memory\n");
            	exit (EXIT_FAILURE);
	}
}

static void
init_mmap			(void)
{
	struct v4l2_requestbuffers req;

        CLEAR (req);

        req.count               = 4;
        req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory              = V4L2_MEMORY_MMAP;

	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf (stderr, "%s does not support "
                                 "memory mapping\n", dev_name);
                        exit (EXIT_FAILURE);
                } else {
                        errno_exit ("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf (stderr, "Insufficient buffer memory on %s\n",
                         dev_name);
                exit (EXIT_FAILURE);
        }

        buffers = calloc (req.count, sizeof (*buffers));

        if (!buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR (buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit ("VIDIOC_QUERYBUF");

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start =
                        mmap (NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit ("mmap");
        }
}

static void
init_userp			(unsigned int		buffer_size)
{
	struct v4l2_requestbuffers req;
        unsigned int page_size;

        page_size = getpagesize ();
        buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

        CLEAR (req);

        req.count               = 4;
        req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory              = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf (stderr, "%s does not support "
                                 "user pointer i/o\n", dev_name);
                        exit (EXIT_FAILURE);
                } else {
                        errno_exit ("VIDIOC_REQBUFS");
                }
        }

        buffers = calloc (4, sizeof (*buffers));

        if (!buffers) {
                fprintf (stderr, "Out of memory\n");
                exit (EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
                buffers[n_buffers].length = buffer_size;
                buffers[n_buffers].start = memalign (/* boundary */ page_size,
                                                     buffer_size);

                if (!buffers[n_buffers].start) {
    			fprintf (stderr, "Out of memory\n");
            		exit (EXIT_FAILURE);
		}
        }
}

static void
init_device                     (void)
{
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
	unsigned int min;

        if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf (stderr, "%s 1is no V4L2 device\n",
                                 dev_name);
                        exit (EXIT_FAILURE);
                } else {
                        errno_exit ("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf (stderr, "%s 2is no video capture device\n",
                         dev_name);
                exit (EXIT_FAILURE);
        }

	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf (stderr, "%s does not support read i/o\n",
				 dev_name);
			exit (EXIT_FAILURE);
		}

		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf (stderr, "%s does not support streaming i/o\n",
				 dev_name);
			exit (EXIT_FAILURE);
		}

		break;
	}


        /* Select video input, video standard and tune here. */


	CLEAR (cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
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
        fmt.fmt.pix.width       = THEWIDTH;//160;//320; 
        fmt.fmt.pix.height      = THEHEIGHT;//120;//240;
	if(CAM_LOGITEC==cam)
	  {
	    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
	  }
	else
	  {
	    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;//V4L2_PIX_FMT_MJPEG;
	  }
        fmt.fmt.pix.field       = V4L2_FIELD_NONE;//V4L2_FIELD_INTERLACED;
	//	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

        if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
                errno_exit ("VIDIOC_S_FMT");

        /* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	switch (io) {
	case IO_METHOD_READ:
		init_read (fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		init_mmap ();
		break;

	case IO_METHOD_USERPTR:
		init_userp (fmt.fmt.pix.sizeimage);
		break;
	}
}

static void
close_device                    (void)
{
        if (-1 == close (fd))
	        errno_exit ("close");

        fd = -1;
}

static void
open_device                     (void)
{
        struct stat st; 

        if (-1 == stat (dev_name, &st)) {
                fprintf (stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror (errno));
                exit (EXIT_FAILURE);
        }

        if (!S_ISCHR (st.st_mode)) {
                fprintf (stderr, "%s is no device\n", dev_name);
                exit (EXIT_FAILURE);
        }

        fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf (stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror (errno));
                exit (EXIT_FAILURE);
        }
}

static void
usage                           (FILE *                 fp,
                                 int                    argc,
                                 char **                argv)
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

static const char short_options [] = "d:hmruc";

static const struct option
long_options [] = {
        { "device",     required_argument,      NULL,           'd' },
        { "help",       no_argument,            NULL,           'h' },
        { "mmap",       no_argument,            NULL,           'm' },
        { "read",       no_argument,            NULL,           'r' },
        { "userp",      no_argument,            NULL,           'u' },
	{ "cam",      no_argument,            NULL,           'c' },
        { 0, 0, 0, 0 }
};

int
main                            (int                    argc,
                                 char **                argv)
{
        dev_name = "/dev/video0";

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
                        dev_name = optarg;
                        break;

                case 'h':
                        usage (stdout, argc, argv);
                        exit (EXIT_SUCCESS);

                case 'm':
                        io = IO_METHOD_MMAP;
			break;

                case 'r':
                        io = IO_METHOD_READ;
			break;

                case 'u':
                        io = IO_METHOD_USERPTR;
			break;
		case 'c':
                        cam = CAM_LOGITEC;
			break;


                default:
                        usage (stderr, argc, argv);
                        exit (EXIT_FAILURE);
                }
        }

	if(initSDL())
	  {
	    printf("initSDL failed\n");
	  }

        open_device ();

        init_device ();

        start_capturing ();

        mainloop ();

        stop_capturing ();

        uninit_device ();

        close_device ();

        exit (EXIT_SUCCESS);

        return 0;
}
