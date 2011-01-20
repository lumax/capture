/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2008 Christopher Harvey
*/

/*
  This program is a viewer written for usb-webcams using the uvc 
  video driver. Eventually it will be the track-pad input server.
 */

#define GUI

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

//v4l2 stuff
#include <fcntl.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

//pipe stuff
#include <linux/stat.h>
#include <sys/stat.h>

#ifdef GUI
#include <SDL/SDL.h>
#endif

typedef struct {
  void *start;
  size_t length;
} CamBuffer;

const char         *g_v4l2Name;
int                 g_deviceFD;
enum v4l2_priority  g_priority = V4L2_PRIORITY_RECORD;
int                 g_physicalConnection = 0;
struct v4l2_format  g_imgFormat;
enum v4l2_memory    g_streamType;
int                 g_width;
int                 g_height;
CamBuffer          *g_buffer;
int                 g_numBuffers;
int                 g_done = 0; //capture while this equals 0
V4L2_PIX_FMT_YUYV 
int openDevice(const char *name);
int closeDevice(int fd);

int listCaps(int fd);
int listInputs(int fd);
int printCurrentStandards(int fd);
int listImageFormats(int fd);
int printCurrentFormat(int fd);
enum v4l2_memory getStreamType(int fd);
int unmapBuffers(CamBuffer *buffers, int numBuffers);
CamBuffer *mapBuffers(int request, int min, int fd, int *numBuffers);
void mainLoop();
#ifdef GUI
  int checkSDLKeyEvent();
#endif

/**
   The main function.
   @param args[1] the v4l2 device name.
 */
int main(int argc, const char **args)
{
  //Setup the device name
  if(argc<2)
    {
      printf("Missing v4l2 device name.\n");
      return 1;
    }
  g_v4l2Name = args[1];
  if(strcmp(g_v4l2Name, "--help")==0)
    {
      printf("Print the help\n");
      return 0;
    }

  printf("Device name: %s\n", g_v4l2Name);

  //parse arguments
  int i;
  for(i = 0;i<argc;i++)
    {
      if(strcmp(args[i], "-w")==0)
	{
	  i++;
	  g_width = atoi(args[i]);
	  printf("Requested Width: %i\n", g_width);
	}
      else if(strcmp(args[i], "-h")==0)
	{
	  i++;
	  g_height = atoi(args[i]);
	  printf("Requested Height %i\n", g_height);
	}
    }

  //Get the file descriptor
  g_deviceFD = openDevice(g_v4l2Name);
  if(g_deviceFD == -1)
    {
      printf("Could not open device.\n");
      return 1;
    }

  //Get the device capabilities
  int capsResult = listCaps(g_deviceFD);
  if(capsResult==-1)
    {
      printf("Device not compatible with the v4l2 specification.");
      closeDevice(g_deviceFD);
      return 1;
    }
  else if(capsResult==1)
    {
      printf("Required capabilities not met.\n");
      closeDevice(g_deviceFD);
      return 1;
    }
  assert(capsResult==0);
  
  listInputs(g_deviceFD);
  if(ioctl(g_deviceFD, VIDIOC_S_INPUT, &g_physicalConnection)==0)
    printf("Set to input %i\n", g_physicalConnection);
  else
    {
      printf("Could not set to connection %i\n", g_physicalConnection);
      closeDevice(g_deviceFD);
      return 1;
    }
 
  listImageFormats(g_deviceFD);
  
  memset(&g_imgFormat, 0, sizeof(struct v4l2_format));
  g_imgFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(ioctl(g_deviceFD, VIDIOC_G_FMT, &g_imgFormat)!=0) {
    assert(false);
  }

  g_imgFormat.fmt.pix.width = g_width;
  g_imgFormat.fmt.pix.height = g_height;
  g_imgFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

  if(ioctl(g_deviceFD, VIDIOC_S_FMT, &g_imgFormat)!=0)
    {
      printf("\tCould not lock the deault iamge format!\n");
      switch(errno)
	{
	case EINVAL:
	  printf("\tInvalid format.\n"); break;
	case EBUSY:
	  printf("\tDevice Busy.\n"); break;
	}
    }
  else
    printf("Default image format locked!\n");
  g_width = g_imgFormat.fmt.pix.width;
  g_height = g_imgFormat.fmt.pix.height;
  printf("\n");
  
  printCurrentFormat(g_deviceFD);
  //  g_streamType = getStreamType(g_deviceFD);
  g_buffer = mapBuffers(20, 5, g_deviceFD, &g_numBuffers);
  if(g_buffer==0)
    {
      printf("Could not get buffers for streaming.\n");
      closeDevice(g_deviceFD);
      return 1;
    }
  printf("Got %i buffers for memory map streaming.\n", g_numBuffers);

  //Try to get hihest priority
  /* if(ioctl(g_deviceFD, VIDIOC_S_PRIORITY, &g_priority)==0)
    printf("Got highest priority for camera access.\n");
  else
    {
      printf("Could not get highest priority for camera access.\n");
      switch(errno)
	{
	case EINVAL:
	  printf("Invalid priority.\n"); break;
	case EBUSY:
	  printf("Another app has the priority already.\n"); break;
	} 
	}*/

  //assert(g_streamType==V4L2_MEMORY_MMAP); //only support this one now.

  int tmp = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(ioctl(g_deviceFD, VIDIOC_STREAMON, &tmp)!=0) {
    assert(false);
  }

  mainLoop();

  if(ioctl(g_deviceFD, VIDIOC_STREAMOFF, &tmp)==0) {
    assert(false);
  }
  
  //Shut everything down.
  unmapBuffers(g_buffer, g_numBuffers);
  closeDevice(g_deviceFD);
  return 0;
}

void mainLoop()
{
  unsigned char *userBuffer = malloc(sizeof(unsigned char)*g_width*g_height*2);
#ifdef GUI
  SDL_Surface *screen;
  SDL_Overlay *overlay;
  if(SDL_Init(SDL_INIT_VIDEO)<0)
    {
      printf("Could not init SDL video: '%s'\n", SDL_GetError());
      return;
    }
  screen = SDL_SetVideoMode(g_width, g_height, 0,
			    SDL_HWSURFACE | SDL_DOUBLEBUF);
  if(screen == NULL)
    {
      printf("Could not set display: '%s'\n", SDL_GetError());
      return;
    }

  overlay = SDL_CreateYUVOverlay(g_width, g_height, SDL_YVYU_OVERLAY, screen);
  if(overlay == NULL)
    {
      printf("Could not create overlay: '%s'\n", SDL_GetError());
      return;
    }
  printf("OverLay has %i planes.\n", overlay->planes);
#endif
  int i;
  fd_set fds;
  struct timeval timeout;
  struct v4l2_buffer buf;


  while(g_done==0)
    {
      timeout.tv_sec = 2;
      timeout.tv_usec = 0;

      FD_SET(g_deviceFD, &fds);

      if(select(g_deviceFD +1, &fds, NULL, NULL, &timeout)==-1)
	{
	  printf("Error with select.\n");
	  break;
	}
      memset(&buf, 0, sizeof(struct v4l2_buffer));
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      while(ioctl(g_deviceFD, VIDIOC_DQBUF, &buf)!=0)
	{
	  switch(errno)
	    {
	    case EAGAIN:
	      printf("Again.\n"); break;
	    case EINVAL:
	      printf("Invalid Buffer.\n"); break;
	    case ENOMEM:
	      printf("Memory shortage.\n"); break;
	    case EIO:
	      printf("Internal error or memory loss.\n"); break;
	    }
#ifdef GUI
	  if(checkSDLKeyEvent()!=0) {
	    g_done = 1;
	  }
#endif
	  if(g_done!=0)
	    break;
	  usleep(1000);
	}
      memcpy(userBuffer, g_buffer[buf.index].start, buf.bytesused);
#ifdef GUI
      SDL_LockSurface(screen);
      SDL_LockYUVOverlay(overlay);
      
      memcpy(overlay->pixels[0], userBuffer, buf.bytesused);

      SDL_UnlockYUVOverlay(overlay);
      SDL_UnlockSurface(screen);

      SDL_DisplayYUVOverlay(overlay, &screen->clip_rect);
      if(checkSDLKeyEvent()!=0)
	g_done = 1;
#endif
      if(ioctl(g_deviceFD, VIDIOC_QBUF, &buf)!=0)
	{
	  printf("Could not requeue buffer.\n");
	  g_done = 1;
	}
    }
#ifdef GUI
  SDL_FreeSurface(screen);
  SDL_FreeYUVOverlay(overlay);
#endif
  free(userBuffer);
}

int unmapBuffers(CamBuffer *buffers, int numBuffers)
{
  int unmappedCount = 0;
  int i;
  for (i = 0; i < numBuffers; i++)
    {
      if(munmap(buffers[i].start, buffers[i].length)==0)
	unmappedCount++;
    }
  if(unmappedCount<numBuffers)
    printf("Could only unmap %i of the %i buffers.\n", unmappedCount, 
	   numBuffers);
  else
    printf("Unmapped all %i buffers.\n", unmappedCount);

  free(buffers);

  return unmappedCount;
}

CamBuffer *mapBuffers(int request, int min, int fd, int *numBuffers)
{
  CamBuffer *buffers;
  *numBuffers= 0;
  struct v4l2_requestbuffers reqbuf;
  memset (&reqbuf, 0, sizeof (struct v4l2_requestbuffers));
  reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory = V4L2_MEMORY_MMAP;
  reqbuf.count = request;
  
  if(ioctl(fd, VIDIOC_REQBUFS, &reqbuf)==-1)
    {
      printf("Could not request buffers.\n");
      return NULL;
    }
  
  *numBuffers = reqbuf.count;
  if(*numBuffers<min)
    {
      printf("Out of memory. Only got %i buffers. Minumum: %i\n", 
	     numBuffers, min);
      return NULL;
    }
  
  buffers = calloc(reqbuf.count, sizeof(CamBuffer));
  assert(buffers!=NULL);
  
  int i;
  for (i = 0; i < reqbuf.count; i++) {
    struct v4l2_buffer buffer;
    
    memset (&buffer, 0, sizeof (struct v4l2_buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = i;
    
    if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buffer)) {
      printf("Could not query buffer on index %i\n", i);
      return NULL;
    }
    
    buffers[i].length = buffer.length; /* remember for munmap() */
    
    buffers[i].start = mmap (NULL, buffer.length,
			     PROT_READ | PROT_WRITE, /* recommended */
			     MAP_SHARED,             /* recommended */
			     fd, buffer.m.offset);
    
    if (MAP_FAILED == buffers[i].start) {
      printf("Map filed on %i\n", i);
      printf("---Exit here---\n");
      return NULL;
    }
  }
  
  for(i = 0;i < reqbuf.count;i++)
    {
      struct v4l2_buffer bufff;
      memset(&bufff, 0, sizeof(struct v4l2_buffer));
      bufff.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      bufff.memory      = V4L2_MEMORY_MMAP;
      bufff.index       = i;
      if(ioctl(fd, VIDIOC_QBUF, &bufff)!=0)
	printf("Could not queue buffer %i.\n", i);
    }
  
  return buffers;
}

enum v4l2_memory getStreamType(int fd)
{
  struct v4l2_requestbuffers reqbuf;
  
  memset (&reqbuf, 0, sizeof (reqbuf));
  reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory = V4L2_MEMORY_USERPTR;
  
  if (ioctl (fd, VIDIOC_REQBUFS, &reqbuf) == -1) 
    {
      if (errno == EINVAL)
	printf ("Video capturing by user pointer streaming is not supported\n");
    } 
  else
    {
      printf("Got userptr streaming.\n");
      return V4L2_MEMORY_USERPTR;
    }
  
  memset (&reqbuf, 0, sizeof (reqbuf));
  reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory = V4L2_MEMORY_MMAP;
  reqbuf.count = 20;
  
  if (-1 == ioctl (fd, VIDIOC_REQBUFS, &reqbuf)) 
    {
      if (errno == EINVAL)
	{
	  printf ("Video mmap-streaming is not supported either!\n");
	}
      printf("Couldn't get a streaming method.\n");
      return 999;
    }
  printf("Got memory mapping streaming with %i buffers\n", reqbuf.count);
  return V4L2_MEMORY_MMAP;
}

int printCurrentFormat(int fd)
{
  struct v4l2_format fmt;
  char fcc[5];
  fcc[4] = '\0';
  memset(&fmt, 0, sizeof(struct v4l2_format));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int result = ioctl(fd, VIDIOC_G_FMT, &fmt);
  if(result==-1)
    printf("Could nto get the format.\n");

  printf("\tCurrent image format:\n");
  printf("Width: %i\n", fmt.fmt.pix.width);
  printf("Height %i\n", fmt.fmt.pix.height);
  printf("PixelFormat: %i\n", fmt.fmt.pix.pixelformat);
  printf("Bytes per line: %i\n", fmt.fmt.pix.bytesperline);

  printf("Feild: ");
  switch(fmt.fmt.pix.field)
    {
    case V4L2_FIELD_ANY:
      printf("V4L2_FIELD_ANY\n"); break;
    case V4L2_FIELD_NONE:
      printf("V4L2_FIELD_NONE\n"); break;
    case V4L2_FIELD_TOP:
      printf("V4L2_FIELD_TOP\n"); break;
    case V4L2_FIELD_BOTTOM:
      printf("V4L2_FIELD_BOTTOM\n"); break;
    case V4L2_FIELD_INTERLACED:
      printf("V4L2_FIELD_INTERLACED\n"); break;
    case V4L2_FIELD_SEQ_TB:
      printf("V4L2_FIELD_SEQ_TB\n"); break;
    case V4L2_FIELD_SEQ_BT:
      printf("V4L2_FIELD_SEQ_BT\n"); break;
    case V4L2_FIELD_ALTERNATE:
      printf("V4L2_FIELD_ALTERNATE\n"); break;
    case V4L2_FIELD_INTERLACED_TB:
      printf("V4L2_FIELD_INTERLACED_TB\n"); break;
    case V4L2_FIELD_INTERLACED_BT:
      printf("V4L2_FIELD_INTERLACED_BT\n"); break;
    }

  __u32 x = fmt.fmt.pix.pixelformat;
  fcc[0] = (x&0xff);
  fcc[1] = ((x>>8)&0xff);
  fcc[2] = ((x>>16)&0xff);
  fcc[3] = ((x>>24)&0xff);
  printf("Image code: %s\n", fcc);

  printf("Colorspace: ");
  switch(fmt.fmt.pix.colorspace)
    {
    case V4L2_COLORSPACE_SMPTE170M:
      printf("V4L2_COLORSPACE_SMPTE170M\n"); break;
    case V4L2_COLORSPACE_SMPTE240M:
      printf("V4L2_COLORSPACE_SMPTE240M\n"); break;
    case V4L2_COLORSPACE_REC709:
      printf("V4L2_COLORSPACE_REC709\n"); break;
    case V4L2_COLORSPACE_BT878:
      printf("V4L2_COLORSPACE_BT878\n"); break;
    case V4L2_COLORSPACE_SRGB:
      printf("V4L2_COLORSPACE_SRGB\n"); break;
    case V4L2_COLORSPACE_470_SYSTEM_M:
      printf("V4L2_COLORSPACE_470_SYSTEM_M\n"); break;
    case V4L2_COLORSPACE_470_SYSTEM_BG:
      printf("V4L2_COLORSPACE_470_SYSTEM_BG\n"); break;
    case V4L2_COLORSPACE_JPEG:
      printf("V4L2_COLORSPACE_JPEG\n"); break;
    default:
      printf("Unknown\n"); break;
      
    }
  printf("\n");
  //TODO: print all info
  return result;
}

int listImageFormats(int fd)
{
  printf("-------Image format enumeration-----\n");
  struct v4l2_fmtdesc fmt;
  memset(&fmt, 0, sizeof(struct v4l2_fmtdesc));
  __u32 index = 0;
  fmt.index = 0;
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  while(ioctl(fd, VIDIOC_ENUM_FMT, &fmt)==0)
    {
      printf("\tFormat #%i\n", index);
      printf("\t%s\n", fmt.description);
      printf("Pixel format id: %i\n", fmt.pixelformat);

      printf("Type: ");
      switch(fmt.type)
	{
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	  printf("V4L2_BUF_TYPE_VIDEO_CAPTURE\n"); break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	  printf("V4L2_BUF_TYPE_VIDEO_OUTPUT\n"); break;
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	  printf("V4L2_BUF_TYPE_VIDEO_OVERLAY\n"); break;
	case V4L2_BUF_TYPE_PRIVATE:
	  printf("V4L2_BUF_TYPE_PRIVATE\n"); break;
	default:
	  printf("Other\n"); break;
	}

      printf("Compressed: ");
      if(fmt.flags & V4L2_FMT_FLAG_COMPRESSED)
	printf("Yes\n");
      else
	printf("No\n");
      
      
      index++;
      memset(&fmt, 0, sizeof(struct v4l2_fmtdesc));
      fmt.index = index;
      fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    }
  printf("-------------------------------------\n");
  return index;
}

int printCurrentStandards(int fd)
{
  struct v4l2_input input;
  struct v4l2_standard standard;
  
  memset (&input, 0, sizeof (input));
  
  if (-1 == ioctl (fd, VIDIOC_G_INPUT, &input.index)) {
    return 1;
  }
  
  if (-1 == ioctl (fd, VIDIOC_ENUMINPUT, &input)) {
    return 1;
  }
  
  printf ("Current input %s supports:\n", input.name);
  
  memset (&standard, 0, sizeof (standard));
  standard.index = 0;
  
  while (0 == ioctl (fd, VIDIOC_ENUMSTD, &standard)) {
    if (standard.id & input.std)
      printf ("%s\n", standard.name);
    
    standard.index++;
  }
  
  /*
  __u32 index = 0;
  struct v4l2_standard iostd;
  iostd.index = 0;
  while(ioctl(fd, VIDIOC_ENUMSTD, &iostd)==0)
    {
      printf("\tIndex: #i\n", index);
      printf("\tName: %s\n", iostd.name);
      index++;
      iostd.index = index;
    }
    return index;*/
}

int listInputs(int fd)
{
  printf("----------Device Connections---------\n");
  struct v4l2_input inputs;
  int index = 0;
  inputs.index = 0;
  while(ioctl(fd, VIDIOC_ENUMINPUT, &inputs)==0)
    {
      printf("\tInput #%i\n", index);
      printf("Name: %s\n", inputs.name);

      printf("Type: ");
      if(inputs.type == V4L2_INPUT_TYPE_CAMERA)
	printf("Camera\n");
      else if(inputs.type == V4L2_INPUT_TYPE_TUNER)
	printf("Tuner\n");
      else
	assert(0);

      printf("Video standard: %i\n\n", inputs.std);
      index++;
      inputs.index = index;
    }
  printf("-------------------------------------\n");
  return index;
}

int listCaps(int fd)
{
  struct v4l2_capability capdata;
  int result = ioctl(fd, VIDIOC_QUERYCAP, &capdata);
  if(result==-1)
    return -1;
  
  printf("---------Device Capabilities---------\n");
  printf("Driver:         %s\n", capdata.driver);
  printf("Name:           %s\n", capdata.card);
  printf("Bus:            %s\n", capdata.bus_info);
  __u32 version = capdata.version;
  printf ("Driver Version: %u.%u.%u\n\n",
	  (version >> 16) & 0xFF,
	  (version >> 8) & 0xFF,
	  version & 0xFF);

  printf("Video Capture: ");
  if(capdata.capabilities & V4L2_CAP_VIDEO_CAPTURE)
    printf("Yes\n");
  else
    {
      printf("No\nDevice has to have a capture interface to continue.\n");
      return 1;
    }

  printf("Read/write: ");
  if(capdata.capabilities & V4L2_CAP_READWRITE)
    printf("Yes\n");
  else
    {
      printf("No\n");
      //printf("No\nDevice has to have read and/or write to continue.\n");
      //      return 1;
    }

  printf("Async IO: ");
  if(capdata.capabilities & V4L2_CAP_ASYNCIO)
    printf("Yes\n");
  else
    printf("No\n");

  printf("Streaming: ");
  if(capdata.capabilities & V4L2_CAP_STREAMING)
    printf("Yes\n");
  else
    printf("No\n");

  if(!(capdata.capabilities & V4L2_CAP_ASYNCIO || 
       capdata.capabilities & V4L2_CAP_STREAMING))
    {
      printf("Deivce needs either async or streaming io.\n");
      return 1;
    }
  printf("-------------------------------------\n");
  return 0;
}

int openDevice(const char *name)
{
  int result = open(name, O_RDWR | O_NONBLOCK, 0);
  if(result == -1)
    {
      printf("openDevice: Could not open the device: ");
      switch(errno)
	{
	case EACCES:
	  printf("Do not have access.\n"); break;
	case EBUSY:
	  printf("The device is busy.\n"); break;
	case ENXIO:
	  printf("No device corresponding to this device special file exists.");
	  break;
	case ENOMEM:
	  printf("Not enough kernel memory.\n"); break;
	case EMFILE:
	  printf("Too many open files in process.\n"); break;
	case ENFILE:
	  printf("Too many file open on the system.\n"); break;
	default:
	  printf("Undefined open error.\n"); break;
	}
      return -1;
    }
  return result;
}

int closeDevice(int fd)
{
  int result = close(fd);
  if(result == -1)
    {
      printf("Problem closing the device.\n");
      return result;
    }
  return result;
}

#ifdef GUI
int checkSDLKeyEvent()
{
  SDL_Event event;
  int ret = 0;
  while ( SDL_PollEvent(&event) ) 
    {
      switch (event.type) 
	{
	case SDL_KEYDOWN:
	  ret = 1;
	  break;
	}
    }
  return ret;
}
#endif
