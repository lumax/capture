

#define ERR_NO_SOI 1
#define ERR_NOT_8BIT 2
#define ERR_HEIGHT_MISMATCH 3
#define ERR_WIDTH_MISMATCH 4
#define ERR_BAD_WIDTH_OR_HEIGHT 5
#define ERR_TOO_MANY_COMPPS 6
#define ERR_ILLEGAL_HV 7
#define ERR_QUANT_TABLE_SELECTOR 8
#define ERR_NOT_YCBCR_221111 9
#define ERR_UNKNOWN_CID_IN_SCAN 10
#define ERR_NOT_SEQUENTIAL_DCT 11
#define ERR_WRONG_MARKER 12
#define ERR_NO_EOI 13
#define ERR_BAD_TABLES 14
#define ERR_DEPTH_MISMATCH 15

#ifndef __DSP_JPEG_H__
#define __DSP_JPEG_H__
#ifdef __cplusplus
extern "C" {
#endif

int jpeg_decode(unsigned char **pic, unsigned char *buf, int *width,
		int *height);
//int 
//get_picture(unsigned char *buf,int size);
//int
//get_pictureYV2(unsigned char *buf,int width,int height);

//#####color.h anfang


#define CLIP(color) (unsigned char)(((color)>0xFF)?0xff:(((color)<0)?0:(color)))

unsigned char
RGB24_TO_Y(unsigned char r, unsigned char g, unsigned char b);

unsigned char
YR_TO_V(unsigned char r, unsigned char y);

unsigned char
YB_TO_U(unsigned char b, unsigned char y);

unsigned char
R_FROMYV(unsigned char y, unsigned char v);

unsigned char
G_FROMYUV(unsigned char y, unsigned char u, unsigned char v);

unsigned char
B_FROMYU(unsigned char y, unsigned char u);

#define YfromRGB(r,g,b) CLIP((77*(r)+150*(g)+29*(b))>>8)
#define UfromRGB(r,g,b) CLIP(((128*(b)-85*(g)-43*(r))>>8 )+128)
#define VfromRGB(r,g,b) CLIP(((128*(r)-107*(g)-21*(b))>>8) +128)

#define PACKRGB16(r,g,b) (__u16) ((((b) & 0xF8) << 8 ) | (((g) & 0xFC) << 3 ) | (((r) & 0xF8) >> 3 ))
#define UNPACK16(pixel,r,g,b) r=((pixel)&0xf800) >> 8; 	g=((pixel)&0x07e0) >> 3; b=(((pixel)&0x001f) << 3)

void initLut(void);
void freeLut(void);
//##color.h ende

#ifdef __cplusplus
}
#endif
#endif /* __DSP_JPEG_H__*/
