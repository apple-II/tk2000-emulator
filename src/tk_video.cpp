/****************************************************************************
*
*  APPLE //E EMULATOR FOR WINDOWS                    
*
*  Copyright (C) 1994-96, Michael O'Brien.  All rights reserved.
*
***/

/*  Emulador do computador TK2000 (Microdigital)
 *  por Fábio Belavenuto - Copyright (C) 2004
 *
 *  Adaptado do emulador Applewin por Michael O'Brien
 *  Part of code is Copyright (C) 2003-2004 Tom Charlesworth
 *
 *  Este arquivo é distribuido pela Licença Pública Geral GNU.
 *  Veja o arquivo Licenca.txt distribuido com este software.
 *
 *  ESTE SOFTWARE NÃO OFERECE NENHUMA GARANTIA
 *
 */

// Emula o vídeo

#include "tk_stdhdr.h"
#include "tk_main.h"
#include "tk_video.h"
#include "tk_memoria.h"
#include "tk_teclado.h"
#include "tk_cpu.h"
#include "tk_joystick.h"
#include "tk_janela.h"
#include "tk_registro.h"
#include "tk_tape.h"

/* reference: technote tn-iigs-063 "Master Color Values"

          Color  Color Register LR HR  DHR Master Color R,G,B
          Name       Value      #  #   #      Value
          ----------------------------------------------------
          Black       0         0  0,4 0      $0000    (0,0,0) -> (00,00,00) Windows
(Magenta) Deep Red    1         1      1      $0D03    (D,0,3) -> (D0,00,30) Custom
          Dark Blue   2         2      8      $0009    (0,0,9) -> (00,00,80) Windows
 (Violet) Purple      3         3  2   9      $0D2D    (D,2,D) -> (FF,00,FF) Windows
          Dark Green  4         4      4      $0072    (0,7,2) -> (00,80,00) Windows
 (Gray 1) Dark Gray   5         5      5      $0555    (5,5,5) -> (80,80,80) Windows
   (Blue) Medium Blue 6         6  6   C      $022F    (2,2,F) -> (00,00,FF) Windows
   (Cyan) Light Blue  7         7      D      $06AF    (6,A,F) -> (60,A0,FF) Custom
          Brown       8         8      2      $0850    (8,5,0) -> (80,50,00) Custom
          Orange      9         9  5   3      $0F60    (F,6,0) -> (FF,80,00) Custom (modified to match better with the other Hi-Res Colors)
 (Gray 2) Light Gray  A         A      A      $0AAA    (A,A,A) -> (C0,C0,C0) Windows
          Pink        B         B      B      $0F98    (F,9,8) -> (FF,90,80) Custom
  (Green) Light Green C         C  1   6      $01D0    (1,D,0) -> (00,FF,00) Windows
          Yellow      D         D      7      $0FF0    (F,F,0) -> (FF,FF,00) Windows
   (Aqua) Aquamarine  E         E      E      $04F9    (4,F,9) -> (40,FF,90) Custom
          White       F         F  3,7 F      $0FFF    (F,F,F) -> (FF,FF,FF) Windows

   LR: Lo-Res   HR: Hi-Res   DHR: Double Hi-Res */

// STANDARD WINDOWS COLORS
#define  BLACK            0x00
#define  DARK_RED         0x01
#define  DARK_GREEN       0x02
#define  DARK_YELLOW      0x03
#define  DARK_BLUE        0x04
#define  DARK_MAGENTA     0x05
#define  DARK_CYAN        0x06
#define  LIGHT_GRAY       0x07
#define  MONEY_GREEN      0x08
#define  SKY_BLUE         0x09
#define  CREAM            0xF6
#define  MEDIUM_GRAY      0xF7
#define  DARK_GRAY        0xF8
#define  RED              0xF9
#define  GREEN            0xFA
#define  YELLOW           0xFB
#define  BLUE             0xFC
#define  MAGENTA          0xFD
#define  CYAN             0xFE
#define  WHITE            0xFF

// OUR CUSTOM COLORS
#define  DEEP_RED         0x0A
#define  LIGHT_BLUE       0x0B
#define  BROWN            0x0C
#define  ORANGE           0X0D
#define  PINK             0x0E
#define  AQUA             0x0F

// CUSTOM HGR COLORS (don't change values) - For tv emulation mode
#define  HGR_BLACK        0x10
#define  HGR_WHITE        0x11
#define  HGR_BLUE         0x12
#define  HGR_RED          0x13
#define  HGR_GREEN        0x14
#define  HGR_CYAN         0x15

#define  HGR_GREY1        0x16
#define  HGR_GREY2        0x17
#define  HGR_YELLOW       0x18
#define  HGR_AQUA         0x19
#define  HGR_PURPLE       0x1A
#define  HGR_GREEN2       0x1B

// USER CUSTOMIZABLE COLOR
#define  MONOCHROME       0x20

#define  SRCOFFS          0
#define  SRCOFFS_TOTAL    512

#define  VF_COLOR         0x00000010
#define  VF_PAGE2         0x00000020

#define  SW_COLOR         (vidmode & VF_COLOR)
#define  SW_PAGE2         (vidmode & VF_PAGE2)

#define  HGR_MATRIX_YOFFSET 2	// For tv emulation mode

// Posições na memória:
#define  PAG1OFFSET   0x2000
#define  PAG2OFFSET   0xA000
#define  VIDEOLENGTH  0x2000

#define  SETSOURCEPIXEL(x,y,c)  sourceoffsettable[(y)][(x)] = (c)

#define  SETFRAMECOLOR(i,r,g,b)  framebufferinfo->bmiColors[i].rgbRed   = r; \
                                 framebufferinfo->bmiColors[i].rgbGreen = g; \
                                 framebufferinfo->bmiColors[i].rgbBlue  = b;

typedef BOOL (*updatetype)(int,int,int,int,int);

BYTE         celldirty[40][32];
COLORREF     customcolors[32];
HBITMAP      devicebitmap;
HDC          devicedc;
BYTE*        framebufferbits;
LPBITMAPINFO framebufferinfo;
BYTE*        frameoffsettable[384];
HBITMAP      logobitmap;
HPALETTE     palette;
HBITMAP      sourcebitmap;
BYTE*        sourcebits;
LPBITMAPINFO sourceinfo;
BYTE*        sourceoffsettable[1024];
BYTE*        videoptr;

// For tv emulation mode
static BYTE          hgrpixelmatrix[280][192 + 2 * HGR_MATRIX_YOFFSET];
static BYTE          colormixbuffer[6];
static WORD          colormixmap[6][6][6];
//

BOOL     displaypage2     = 0;
BYTE*    framebufferaddr  = (BYTE*)0;
LONG     framebufferpitch = 0;
BOOL     graphicsmode     = 0;
BOOL     hasrefreshed     = 0;
DWORD    lastpageflip     = 0;
COLORREF monochrome       = RGB(0xFF,0xFF,0xFF);
BOOL     rebuiltsource    = 0;
BOOL     redrawfull       = 1;
//DWORD    vblcounter       = 0;
BYTE*    vidlastmem       = NULL;
DWORD    vidmode          = VF_COLOR;
DWORD    videotype        = VT_COLOR_TVEMU;
DWORD    emulmsec         = 0;
DWORD    emulmsec_frac    = 0; //--
BOOL     behind           = 0;
int      ScanLines        = 1; // Scan Lines


void DrawVideoSource ();
void DrawMonoVideoSource ();

//===========================================================================
void __stdcall CopySource (int destx, int desty,
                           int xsize, int ysize,
                           int sourcex, int sourcey)
{
	BYTE* currdestptr   = frameoffsettable [desty]  +destx;
	BYTE* currsourceptr = sourceoffsettable[sourcey]+sourcex;
	int bytesleft;
	while (ysize--)
	{
		bytesleft = xsize;
		while (bytesleft & 3)
		{
			--bytesleft;
			// Scan Lines
			*(currdestptr+bytesleft) = (!(ScanLines && !(ysize & 1))) ? *(currsourceptr+bytesleft) : 0;
//			*(currdestptr+bytesleft) = *(currsourceptr+bytesleft);
		}
		while (bytesleft)
		{
			bytesleft -= 4;
			// Scan Lines
			*(DWORD*)(currdestptr+bytesleft) = (!(ScanLines && !(ysize & 1))) ? *(DWORD*)(currsourceptr+bytesleft) : 0;
//			*(DWORD*)(currdestptr+bytesleft) = *(DWORD*)(currsourceptr+bytesleft);
		}
		currdestptr   -= framebufferpitch;
		currsourceptr -= SRCOFFS_TOTAL;
	}
}

//===========================================================================
void CreateFrameOffsetTable (BYTE* addr, LONG pitch)
{
	int loop;
	if (framebufferaddr  == addr &&
		framebufferpitch == pitch)
		return;
	framebufferaddr  = addr;
	framebufferpitch = pitch;

	// CREATE THE OFFSET TABLE FOR EACH SCAN LINE IN THE FRAME BUFFER
	for (loop = 0; loop < 384; loop++)
		frameoffsettable[loop] = framebufferaddr+framebufferpitch*(383-loop);
}

//===========================================================================
void CreateIdentityPalette ()
{
	HWND window;
	HDC  dc;
	int  colors;
	int  system;

	if (palette)
		DeleteObject(palette);

	// SET SIX FRAME BUFFER TABLE ENTRIES TO OUR COLORS
	SETFRAMECOLOR(DEEP_RED,  0xD0,0x00,0x30);
	SETFRAMECOLOR(LIGHT_BLUE,0x60,0xA0,0xFF);
	SETFRAMECOLOR(BROWN,     0x80,0x50,0x00);
	SETFRAMECOLOR(ORANGE,    0xFF,0x80,0x00);
	SETFRAMECOLOR(PINK,      0xFF,0x90,0x80);
	SETFRAMECOLOR(AQUA,      0x40,0xFF,0x90);

	SETFRAMECOLOR(HGR_BLACK,  0x00,0x00,0x00);	// For tv emulation mode
	SETFRAMECOLOR(HGR_WHITE,  0xFF,0xFF,0xFE);
	SETFRAMECOLOR(HGR_BLUE,   0x00,0x80,0xFF); // 00 80 ff
	SETFRAMECOLOR(HGR_RED,    0xF0,0x10,0x00); // f0 50 00
	SETFRAMECOLOR(HGR_GREEN,  0x20,0xC0,0x00); // 20 c0 00
	SETFRAMECOLOR(HGR_CYAN,   0x20,0xB0,0xB0); // a0 00 ff
	// TODO: Mudar combinacoes de cores
	SETFRAMECOLOR(HGR_GREY1,  0x80,0x80,0x80);
	SETFRAMECOLOR(HGR_GREY2,  0x80,0x80,0x80);
	SETFRAMECOLOR(HGR_YELLOW, 0xD0,0xB0,0x10);
	SETFRAMECOLOR(HGR_AQUA,   0x20,0xB0,0xB0);
	SETFRAMECOLOR(HGR_PURPLE, 0x60,0x50,0xE0);
	SETFRAMECOLOR(HGR_GREEN2, 0x10,0x80,0x00);

	
	SETFRAMECOLOR(MONOCHROME,GetRValue(monochrome),
							GetGValue(monochrome),
							GetBValue(monochrome));

	// IF WE ARE IN A PALETTIZED VIDEO MODE, CREATE AN IDENTITY PALETTE
	window = GetDesktopWindow();
	dc     = GetDC(window);
	colors = GetDeviceCaps(dc,SIZEPALETTE);
	system = GetDeviceCaps(dc,NUMCOLORS);
	if ((GetDeviceCaps(dc,RASTERCAPS) & RC_PALETTE) && (colors <= 256))
	{
		// GET THE PALETTE ENTRIES OF THE LOGO
		RGBQUAD logotable[256];
		LOGPALETTE *paldata;
		int paletteindex;
		int logoindex;
		int halftoneindex;
		int ourindex;
		int loop;

		ZeroMemory(logotable,sizeof(logotable));
		if (logobitmap)
		{
			HDC memdc = CreateCompatibleDC(dc);
			SelectObject(memdc,logobitmap);
			GetDIBColorTable(memdc,0,colors,logotable);
			DeleteDC(memdc);
		}

		// CREATE A PALETTE ENTRY ARRAY
		paldata = (LOGPALETTE *)VirtualAlloc(NULL,
			sizeof(LOGPALETTE)
			+256*sizeof(PALETTEENTRY),
			MEM_COMMIT,
			PAGE_READWRITE);
		paldata->palVersion    = 0x300;
		paldata->palNumEntries = colors;
		GetSystemPaletteEntries(dc,0,colors,paldata->palPalEntry);

		// FILL IN THE PALETTE ENTRIES
		paletteindex  = 0;
		logoindex     = 0;
		halftoneindex = 0;

		// COPY THE SYSTEM PALETTE ENTRIES AT THE BEGINNING OF THE PALETTE
		for (; paletteindex < system/2; paletteindex++)
			paldata->palPalEntry[paletteindex].peFlags = 0;

		// FILL IN THE MIDDLE PORTION OF THE PALETTE WITH OUR OWN COLORS
		for (ourindex = DEEP_RED; ourindex <= MONOCHROME; ourindex++)
		{
			paldata->palPalEntry[paletteindex].peRed   = framebufferinfo->bmiColors[ourindex].rgbRed;
			paldata->palPalEntry[paletteindex].peGreen = framebufferinfo->bmiColors[ourindex].rgbGreen;
			paldata->palPalEntry[paletteindex].peBlue  = framebufferinfo->bmiColors[ourindex].rgbBlue;
			paldata->palPalEntry[paletteindex].peFlags = PC_NOCOLLAPSE;
			paletteindex++;
		}
		for (; paletteindex < colors-system/2; paletteindex++)
		{
			// IF THIS PALETTE ENTRY IS NEEDED FOR THE LOGO, COPY IN THE LOGO COLOR
			if (logotable[logoindex].rgbRed &&
				logotable[logoindex].rgbGreen &&
				logotable[logoindex].rgbBlue)
			{
				paldata->palPalEntry[paletteindex].peRed   = logotable[logoindex].rgbRed;
				paldata->palPalEntry[paletteindex].peGreen = logotable[logoindex].rgbGreen;
				paldata->palPalEntry[paletteindex].peBlue  = logotable[logoindex].rgbBlue;
			}

			// OTHERWISE, ADD A HALFTONING COLOR, SO THAT OTHER APPLICATIONS
			// RUNNING IN THE BACKGROUND WILL HAVE SOME REASONABLE COLORS TO USE
			else
			{
				static BYTE halftonetable[6] = {32,64,96,160,192,224};
				paldata->palPalEntry[paletteindex].peRed   = halftonetable[halftoneindex    % 6];
				paldata->palPalEntry[paletteindex].peGreen = halftonetable[halftoneindex/6  % 6];
				paldata->palPalEntry[paletteindex].peBlue  = halftonetable[halftoneindex/36 % 6];
				++halftoneindex;
			}
			++logoindex;
			paldata->palPalEntry[paletteindex].peFlags = PC_NOCOLLAPSE;
		}

		// COPY THE SYSTEM PALETTE ENTRIES AT THE END OF THE PALETTE
		for (; paletteindex < colors; paletteindex++)
			paldata->palPalEntry[paletteindex].peFlags = 0;

		// FILL THE FRAME BUFFER TABLE WITH COLORS FROM OUR PALETTE
		for (loop = 0; loop < colors; loop++)
		{
			framebufferinfo->bmiColors[loop].rgbRed   = paldata->palPalEntry[loop].peRed;
			framebufferinfo->bmiColors[loop].rgbGreen = paldata->palPalEntry[loop].peGreen;
			framebufferinfo->bmiColors[loop].rgbBlue  = paldata->palPalEntry[loop].peBlue;
		}

		// CREATE THE PALETTE
		palette = CreatePalette(paldata);
		VirtualFree(paldata,0,MEM_RELEASE);
	}

	// OTHERWISE, FILL THE FRAME BUFFER TABLE WITH THE STANDARD WINDOWS COLORS
	else
	{
		SETFRAMECOLOR(BLACK,       0x00,0x00,0x00);
		SETFRAMECOLOR(DARK_RED,    0x80,0x00,0x00);
		SETFRAMECOLOR(DARK_GREEN,  0x00,0x80,0x00);
		SETFRAMECOLOR(DARK_YELLOW, 0x80,0x80,0x00);
		SETFRAMECOLOR(DARK_BLUE,   0x00,0x00,0x80);
		SETFRAMECOLOR(DARK_MAGENTA,0x80,0x00,0x80);
		SETFRAMECOLOR(DARK_CYAN,   0x00,0x80,0x80);
		SETFRAMECOLOR(LIGHT_GRAY,  0xC0,0xC0,0xC0);
		SETFRAMECOLOR(MONEY_GREEN, 0xC0,0xDC,0xC0);
		SETFRAMECOLOR(SKY_BLUE,    0xA6,0xCA,0xF0);
		SETFRAMECOLOR(CREAM,       0xFF,0xFB,0xF0);
		SETFRAMECOLOR(MEDIUM_GRAY, 0xA0,0xA0,0xA4);
		SETFRAMECOLOR(DARK_GRAY,   0x80,0x80,0x80);
		SETFRAMECOLOR(RED,         0xFF,0x00,0x00);
		SETFRAMECOLOR(GREEN,       0x00,0xFF,0x00);
		SETFRAMECOLOR(YELLOW,      0xFF,0xFF,0x00);
		SETFRAMECOLOR(BLUE,        0x00,0x00,0xFF);
		SETFRAMECOLOR(MAGENTA,     0xFF,0x00,0xFF);
		SETFRAMECOLOR(CYAN,        0x00,0xFF,0xFF);
		SETFRAMECOLOR(WHITE,       0xFF,0xFF,0xFF);
		palette = (HPALETTE)0;
	}
	ReleaseDC(window,dc);
}

//===========================================================================
void CreateDIBSections ()
{
	// CREATE THE DEVICE CONTEXT
	HWND window  = GetDesktopWindow();
	HDC dc       = GetDC(window);
	HDC sourcedc;
	int loop;

	CopyMemory(sourceinfo->bmiColors,framebufferinfo->bmiColors,256*sizeof(RGBQUAD)); // mudado
  
	if (devicedc)
		DeleteDC(devicedc);
	devicedc = CreateCompatibleDC(dc);

	// CREATE THE FRAME BUFFER DIB SECTION
	if (devicebitmap)
		DeleteObject(devicebitmap);
	devicebitmap = CreateDIBSection(dc,framebufferinfo,DIB_RGB_COLORS,
				(LPVOID *)&framebufferbits,0,0);
	SelectObject(devicedc,devicebitmap);

	// CREATE THE SOURCE IMAGE DIB SECTION
	sourcedc = CreateCompatibleDC(dc);
	ReleaseDC(window,dc);
	if (sourcebitmap)
		DeleteObject(sourcebitmap);
	sourcebitmap = CreateDIBSection(sourcedc,sourceinfo,DIB_RGB_COLORS,
					(LPVOID *)&sourcebits,0,0);
	SelectObject(sourcedc,sourcebitmap);

	// CREATE THE OFFSET TABLE FOR EACH SCAN LINE IN THE SOURCE IMAGE
	for (loop = 0; loop < 1024; loop++) // mudado
		sourceoffsettable[loop] = sourcebits+SRCOFFS_TOTAL*(1023-loop);

	// DRAW THE SOURCE IMAGE INTO THE SOURCE BIT BUFFER
	ZeroMemory(sourcebits,SRCOFFS_TOTAL*1024); //mudado
	DrawVideoSource();
	DeleteDC(sourcedc);
}

//===========================================================================
void DrawColorVideoSource ()
{
	BYTE colorval[6] = {HGR_GREEN,HGR_CYAN,HGR_BLUE,HGR_RED,HGR_BLACK,HGR_WHITE};
	int column;

	for (column = 0; column < 16; column++)
	{
		int coloffs = column << 5;
		unsigned byteval;

		for (byteval = 0; byteval < 256; byteval++)
		{
			BOOL pixelon[11];
			int bitval = 1;
			int pixel;
			int hibit;
			int x;
			int y;

			pixelon[ 0] = column & 4;
			pixelon[ 1] = column & 8;
			pixelon[ 9] = column & 1;
			pixelon[10] = column & 2;
			for (pixel  = 2; pixel < 9; pixel++)
			{
				pixelon[pixel] = ((byteval & bitval) != 0);
				bitval <<= 1;
			}
			hibit = ((byteval & 0x80) != 0);
			x     = 0;
			y     = byteval << 1;
			while (x < 28) {
				int adj = (x >= 14) << 1;
				int odd = (x >= 14);
				for (pixel = 2; pixel < 9; pixel++)
				{
					int color = 4;

					if (pixelon[pixel])
						if (pixelon[pixel-1] || pixelon[pixel+1])
							color = 5;
						else
							color = ((odd ^ (pixel&1)) << 1) | hibit;
					else if (pixelon[pixel-1] && pixelon[pixel+1])
					{
						/***          
							activate for fringe reduction on white hgr text - 
							drawback: loss of color mix patterns in hgr mode.
							select videotype by index exclusion
						***/

						if ((videotype == VT_COLOR_STANDARD) || (videotype == VT_COLOR_TVEMU) || !(pixelon[pixel-2] && pixelon[pixel+2]))
							color = ((odd ^ !(pixel&1)) << 1) | hibit;	// // No white HGR text optimization
					}
					SETSOURCEPIXEL(SRCOFFS+coloffs+x+adj  ,y  ,colorval[color]);
					SETSOURCEPIXEL(SRCOFFS+coloffs+x+adj+1,y  ,colorval[color]);
					SETSOURCEPIXEL(SRCOFFS+coloffs+x+adj  ,y+1,colorval[color]);
					SETSOURCEPIXEL(SRCOFFS+coloffs+x+adj+1,y+1,colorval[color]);
					x += 2;
				}
			}
		}
	}
}

//===========================================================================
void DrawMonoVideoSource ()
{
	int column;
	int x, y;

	for (column = 0; column < 512; column += 16)
	{
		for (y = 0; y < 512; y += 2)
		{
			unsigned val = (y >> 1);

			for ( x = 0; x < 16; x += 2)
			{
				BYTE colorval = (val & 1) ? MONOCHROME : BLACK;
				val >>= 1;
				SETSOURCEPIXEL(SRCOFFS+column+x  ,y  ,colorval);
				SETSOURCEPIXEL(SRCOFFS+column+x+1,y  ,colorval);
				SETSOURCEPIXEL(SRCOFFS+column+x  ,y+1,colorval);
				SETSOURCEPIXEL(SRCOFFS+column+x+1,y+1,colorval);
			}
		}
	}
}

//===========================================================================
void DrawVideoSource()
{
	if (SW_COLOR && videotype != VT_MONO)
		DrawColorVideoSource();
	else
		DrawMonoVideoSource();
}

//===========================================================================
void SetLastDrawnImage ()
{
	int loop;

	memcpy(vidlastmem, videoptr, VIDEOLENGTH);
	for (loop = 0; loop < 256; loop++)
		*(memdirty+loop) &= ~2;
}

//===========================================================================
BYTE MixColors(BYTE c1, BYTE c2)
{	// For tv emulation mode
#define COMBINATION(c1,c2,ref1,ref2) (((c1)==(ref1)&&(c2)==(ref2)) || ((c1)==(ref2)&&(c2)==(ref1)))

	if (c1 == c2)
		return c1;
	if (COMBINATION(c1,c2,HGR_BLUE,HGR_RED))
		return HGR_GREY1;
	else if (COMBINATION(c1,c2,HGR_GREEN,HGR_CYAN))
		return HGR_GREY2;
	else if (COMBINATION(c1,c2,HGR_RED,HGR_GREEN))
		return HGR_YELLOW;
	else if (COMBINATION(c1,c2,HGR_BLUE,HGR_GREEN))
		return HGR_AQUA;
	else if (COMBINATION(c1,c2,HGR_BLUE,HGR_CYAN))
		return HGR_PURPLE;
	else if (COMBINATION(c1,c2,HGR_RED,HGR_CYAN))
		return HGR_GREEN2;
	else
		return MONOCHROME; // visible failure indicator

#undef COMBINATION
}

//===========================================================================
void CreateColorMixMap()
{	// For tv emulation mode
#define FROM_NEIGHBOUR 0x00
	int t,m,b;
	BYTE cTop, cMid, cBot;
	WORD mixTop, mixBot;

	for (t=0; t<6; t++)
		for (m=0; m<6; m++)
			for (b=0; b<6; b++)
			{
				cTop = t | 0x10;
				cMid = m | 0x10;
				cBot = b | 0x10;
				if (cMid < HGR_BLUE)
				{
					mixTop = mixBot = cMid;
				}
				else
				{
					if (cTop < HGR_BLUE)
					{
						mixTop = FROM_NEIGHBOUR;
					}
					else
					{ 
						mixTop = MixColors(cMid,cTop);
					}
					if (cBot < HGR_BLUE)
					{
						mixBot = FROM_NEIGHBOUR;
					}
					else
					{
						mixBot = MixColors(cMid,cBot);
					}
					if (mixTop == FROM_NEIGHBOUR && mixBot != FROM_NEIGHBOUR)
					{
						mixTop = mixBot;
					}
					else
						if (mixBot == FROM_NEIGHBOUR && mixTop != FROM_NEIGHBOUR)
						{
							mixBot = mixTop;
						}
						else
							if (mixBot == FROM_NEIGHBOUR && mixTop == FROM_NEIGHBOUR)
							{
								mixBot = mixTop = cMid;
							}
				}
				colormixmap[t][m][b] = (mixTop << 8) | mixBot;
			}
#undef FROM_NEIGHBOUR
}

//===========================================================================
void __stdcall MixColorsVertical(int matx, int maty)
{	// For tv emulation mode

	WORD twoHalfPixel;
	int bot1idx, bot2idx;

/*
	if (SW_MIXED && maty > 159)
	{
		if (maty < 161)
		{
			bot1idx = hgrpixelmatrix[matx][maty+1] & 0x0F;
			bot2idx = 0;
		}
		else
		{
			bot1idx = bot2idx = 0;
		}
	}
	else
*/
	{
		bot1idx = hgrpixelmatrix[matx][maty+1] & 0x0F;
		bot2idx = hgrpixelmatrix[matx][maty+2] & 0x0F;
	}

	twoHalfPixel = colormixmap[hgrpixelmatrix[matx][maty-2] & 0x0F]
								[hgrpixelmatrix[matx][maty-1] & 0x0F]
								[hgrpixelmatrix[matx][maty  ] & 0x0F];
	colormixbuffer[0] = (twoHalfPixel & 0xFF00) >> 8;
	colormixbuffer[1] = twoHalfPixel & 0x00FF;

	twoHalfPixel = colormixmap[hgrpixelmatrix[matx][maty-1] & 0x0F]
								[hgrpixelmatrix[matx][maty  ] & 0x0F]
								[bot1idx];
	colormixbuffer[2] = (twoHalfPixel & 0xFF00) >> 8;
	colormixbuffer[3] = twoHalfPixel & 0x00FF;

	twoHalfPixel = colormixmap[hgrpixelmatrix[matx][maty  ] & 0x0F]
								[bot1idx]
								[bot2idx];
	colormixbuffer[4] = (twoHalfPixel & 0xFF00) >> 8;
	colormixbuffer[5] = twoHalfPixel & 0x00FF;
}

//===========================================================================
void __stdcall CopyMixedSource (int x, int y, int sourcex, int sourcey)
{	// For tv emulation mode

	LPBYTE currsourceptr = sourceoffsettable[sourcey]+sourcex;
	LPBYTE currdestptr   = frameoffsettable[y<<1] + (x<<1);
	LPBYTE currptr;

	int matx = x;
	int maty = HGR_MATRIX_YOFFSET + y;
	int count;
	int bufxoffset;
	int hgrlinesabove = (y > 0)?   1 : 0;
	int hgrlinesbelow = /*SW_MIXED ? ((y < 159)? 1:0) :*/ ((y < 191)? 1:0);
	int i;
	int istart        = 2 - (hgrlinesabove << 1);
	int iend          = 3 + (hgrlinesbelow << 1);

	// transfer 7 pixels (i.e. the visible part of an apple hgr-byte) from row to pixelmatrix
	for (count = 0, bufxoffset = 0; 
			count < 7; 
			count++, bufxoffset += 2)
	{
		hgrpixelmatrix[matx+count][maty] = *(currsourceptr+bufxoffset);
	
		// color mixing between adjacent scanlines at current x position
		MixColorsVertical(matx+count, maty);

		// transfer up to 6 mixed (half-)pixels of current column to framebuffer
		currptr = currdestptr+bufxoffset;
		if (hgrlinesabove)
			currptr += framebufferpitch << 1;

		for (i = istart;
				i <= iend; 
				currptr -= framebufferpitch, i++)
		{
			// Scan Lines
			*currptr = *(currptr+1) = (!(ScanLines && (i & 1))) ? colormixbuffer[i] : 0;
//			*currptr = *(currptr+1) = colormixbuffer[i];
		}
	}
}

//===========================================================================
BOOL UpdateCell (int x, int y, int xpixel, int ypixel, int offset)
{
	BOOL dirty   = 0;
	int  yoffset = 0;
	while (yoffset < VIDEOLENGTH)
	{
		BYTE byteval1 = (x >  0) ? *(videoptr+offset+yoffset-1) : 0;
		BYTE byteval2 = *(videoptr+offset+yoffset);
		BYTE byteval3 = (x < 39) ? *(videoptr+offset+yoffset+1) : 0;
		if ((byteval2 != *(vidlastmem+offset+yoffset)) ||
				((x >  0) && ((byteval1 & 0x60) != (*(vidlastmem+offset+yoffset+0x1FFF) & 0x60))) ||
				((x < 39) && ((byteval3 & 0x03) != (*(vidlastmem+offset+yoffset+0x2001) & 0x03))) ||
				redrawfull)
		{
#define COLOFFS  (((byteval1 & 0x60) << 2) | \
                  ((byteval3 & 0x03) << 5))
		if ((videotype == VT_COLOR_TVEMU) && SW_COLOR)
		{
  			CopyMixedSource(xpixel >> 1, (ypixel+(yoffset >> 9)) >> 1,
							SRCOFFS+COLOFFS+((x & 1) << 4),(((int)byteval2) << 1));
		}
		else
		{
			CopySource(xpixel,ypixel+(yoffset >> 9),
					   14,2,
					   SRCOFFS+COLOFFS+((x & 1) << 4),(((int)byteval2) << 1));
		}
#undef COLOFFS
			dirty = 1;
		}
		yoffset += 0x400;
	}
	return dirty;
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

//===========================================================================
BOOL VideoApparentlyDirty ()
{
	DWORD address;
	DWORD length;

	if (/*SW_MIXED ||*/ redrawfull)
		return 1;
	address = displaypage2 ? (PAG2OFFSET >> 8) : (PAG1OFFSET >> 8);
	length  = VIDEOLENGTH >> 8;
	while (length--)
		if (*(memdirty+(address++)) & 2)
			return 1;
	return 0;
}

//===========================================================================
BYTE __stdcall VideoCheckMode (WORD programcounter, BYTE address, BYTE write, BYTE value)
{
/*
	if (address == 0x7F)
		return MemReturnRandomData(SW_DHIRES != 0);
	else
	{
		BOOL result = 0;
		switch (address)
		{
			case 0x1A: result = SW_TEXT;    break;
			case 0x1B: result = SW_MIXED;   break;
			case 0x1D: result = SW_HIRES;   break;
			case 0x1E: result = charoffs;   break;
			case 0x1F: result = SW_80COL;   break;
			case 0x7F: result = SW_DHIRES;  break;
		}
		return KeybGetKeycode() | (result ? 0x80 : 0);
	}
*/
	return MemReturnRandomData(0);
}

//===========================================================================
void VideoCheckPage (BOOL force)
{
	if ((displaypage2 != (SW_PAGE2 != 0)) &&
			(force || (emulmsec-lastpageflip > 500)))
	{
		displaypage2 = (SW_PAGE2 != 0);
		VideoRefreshScreen();
		hasrefreshed = 1;
		lastpageflip = emulmsec;
	}
}

/*
//===========================================================================
BYTE __stdcall VideoCheckVbl (WORD programcounter, BYTE address, BYTE write, BYTE value)
{
	return MemReturnRandomData(vblcounter < 22);
}
*/

//===========================================================================
void VideoChooseColor()
{
	CHOOSECOLOR cc;
	ZeroMemory(&cc,sizeof(CHOOSECOLOR));
	cc.lStructSize     = sizeof(CHOOSECOLOR);
	cc.hwndOwner       = (HWND)framewindow;
	cc.rgbResult       = monochrome;
	cc.lpCustColors    = customcolors;
	cc.Flags           = CC_RGBINIT | CC_SOLIDCOLOR;
	if (ChooseColor(&cc))
	{
		monochrome = cc.rgbResult;
		VideoReinitialize();
		if ((mode != MODE_LOGO) && (mode != MODE_DEBUG))
			VideoRedrawScreen();
		RegSaveValue(CONFIG, EMUVIDEOM, 1, monochrome);
	}
}

//===========================================================================
void VideoDestroy ()
{
	// DESTROY BUFFERS
	VirtualFree(framebufferinfo,0,MEM_RELEASE);
	VirtualFree(sourceinfo     ,0,MEM_RELEASE);
	VirtualFree(vidlastmem     ,0,MEM_RELEASE);
	framebufferinfo = NULL;
	sourceinfo      = NULL;
	vidlastmem      = NULL;

	// DESTROY FRAME BUFFER
	DeleteDC(devicedc);
	DeleteObject(devicebitmap);
	devicedc     = (HDC)0;
	devicebitmap = (HBITMAP)0;

	// DESTROY SOURCE IMAGE
	DeleteObject(sourcebitmap);
	sourcebitmap = (HBITMAP)0;

	// DESTROY LOGO
	if (logobitmap)
	{
		DeleteObject(logobitmap);
		logobitmap = (HBITMAP)0;
	}

	// DESTROY PALETTE
	if (palette)
	{
		DeleteObject(palette);
		palette = (HPALETTE)0;
	}
}

//===========================================================================
void VideoDisplayLogo ()
{
	HDC framedc = (HDC)FrameGetDC();
	HFONT font;

	// DRAW THE LOGO
	HBRUSH brush = CreateSolidBrush(PALETTERGB(0x70,0x30,0xE0));
	if (logobitmap)
	{
		HDC memdc = CreateCompatibleDC(framedc);
		SelectObject(memdc,logobitmap);
		BitBlt(framedc,0,0,560,384,memdc,0,0,SRCCOPY);
		DeleteDC(memdc);
	}
	else
	{
		SelectObject(framedc,brush);
		SelectObject(framedc,GetStockObject(NULL_PEN));
		Rectangle(framedc,0,0,560+1,384+1);
	}

	// DRAW THE VERSION NUMBER
	font = CreateFont(-20,0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,
							OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
							VARIABLE_PITCH | 4 | FF_SWISS,
							"Arial");
	SelectObject(framedc,font);
	SetTextAlign(framedc,TA_RIGHT | TA_TOP);
	SetBkMode(framedc,TRANSPARENT);

#define  DRAWVERSION(x,y,c)		SetTextColor(framedc,c);                \
								TextOut(framedc,                        \
										540+x,358+y,                    \
										"Versão " VERSIONSTRING,		\
										strlen("Versão " VERSIONSTRING));

	if (GetDeviceCaps(framedc,PLANES) * GetDeviceCaps(framedc,BITSPIXEL) <= 4)
	{
		DRAWVERSION( 2, 2,RGB(0x00,0x00,0x00));
		DRAWVERSION( 1, 1,RGB(0x00,0x00,0x00));
		DRAWVERSION( 0, 0,RGB(0xFF,0x00,0xFF));
	}
	else
	{
		DRAWVERSION( 1, 1,PALETTERGB(0x30,0x30,0x70));
		DRAWVERSION(-1,-1,PALETTERGB(0xC0,0x70,0xE0));
		DRAWVERSION( 0, 0,PALETTERGB(0x70,0x30,0xE0));
	}
#undef  DRAWVERSION
	FrameReleaseDC();
	DeleteObject(brush);
	DeleteObject(font);
}

//===========================================================================
BOOL VideoHasRefreshed ()
{
	BOOL result = hasrefreshed;
	hasrefreshed = 0;
	return result;
}

//===========================================================================
void VideoInitialize ()
{
	char  filename[MAX_PATH];
	int index;

	// CREATE A BUFFER FOR AN IMAGE OF THE LAST DRAWN MEMORY
	vidlastmem = (BYTE*)VirtualAlloc(NULL,0x10000,MEM_COMMIT,PAGE_READWRITE);
	ZeroMemory(vidlastmem, 0x10000);

	// LOAD THE LOGO
	strcpy(filename, progdir);
	strcat(filename, NOMEARQBMP);
	logobitmap = (HBITMAP)LoadImage(0,filename,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION |
										LR_LOADFROMFILE);

	// CREATE A BITMAPINFO STRUCTURE FOR THE FRAME BUFFER
	framebufferinfo = (LPBITMAPINFO)VirtualAlloc(NULL,
												sizeof(BITMAPINFOHEADER)
												+256*sizeof(RGBQUAD),
												MEM_COMMIT,
												PAGE_READWRITE);
	ZeroMemory(framebufferinfo,sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD));
	framebufferinfo->bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
	framebufferinfo->bmiHeader.biWidth    = 560;
	framebufferinfo->bmiHeader.biHeight   = 384;
	framebufferinfo->bmiHeader.biPlanes   = 1;
	framebufferinfo->bmiHeader.biBitCount = 8;
	framebufferinfo->bmiHeader.biClrUsed  = 256;

	// CREATE A BITMAPINFO STRUCTURE FOR THE SOURCE IMAGE
	sourceinfo = (LPBITMAPINFO)VirtualAlloc(NULL,
											sizeof(BITMAPINFOHEADER)
											+256*sizeof(RGBQUAD),
											MEM_COMMIT,
											PAGE_READWRITE);
	ZeroMemory(sourceinfo,sizeof(BITMAPINFOHEADER));
	sourceinfo->bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
	sourceinfo->bmiHeader.biWidth    = SRCOFFS_TOTAL;
	sourceinfo->bmiHeader.biHeight   = 1024;
	sourceinfo->bmiHeader.biPlanes   = 1;
	sourceinfo->bmiHeader.biBitCount = 8;
	sourceinfo->bmiHeader.biClrUsed  = 256;

	// CREATE AN IDENTITY PALETTE AND FILL IN THE CORRESPONDING COLORS IN
	// THE BITMAPINFO STRUCTURE
	CreateIdentityPalette();

	// PREFILL THE 16 CUSTOM COLORS AND MAKE SURE TO INCLUDE THE CURRENT MONOCHROME COLOR
	for (index = DARK_RED; index <= MONOCHROME; index++)
		customcolors[index-DARK_RED] = RGB(framebufferinfo->bmiColors[index].rgbRed,
										framebufferinfo->bmiColors[index].rgbGreen,
										framebufferinfo->bmiColors[index].rgbBlue);

	// CREATE THE FRAME BUFFER DIB SECTION AND DEVICE CONTEXT,
	// CREATE THE SOURCE IMAGE DIB SECTION AND DRAW INTO THE SOURCE BIT BUFFER
	CreateDIBSections();

	// RESET THE VIDEO MODE SWITCHES AND THE CHARACTER SET OFFSET
	VideoResetState();
}

//===========================================================================
void VideoRealizePalette (HDC dc)
{
	if (palette)
	{
		SelectPalette(dc,palette,0);
		RealizePalette(dc);
	}
}

//===========================================================================
void VideoRedrawScreen ()
{
//	if ((mode != MODE_LOGO) && (mode != MODE_DEBUG)) {
		redrawfull = 1;
		VideoRefreshScreen();
//	}
}

//===========================================================================
void VideoRefreshScreen ()
{
	BYTE*      addr           = framebufferbits;
	LONG       pitch          = 560;
	HDC        framedc        = (HDC)FrameGetVideoDC((char **)&addr,&pitch);
	updatetype update;
	BOOL       anydirty       = 0;
	BOOL       remainingdirty = 0;
	int        y              = 0;
	int        ypixel         = 0;

	CreateFrameOffsetTable(addr,pitch);

	// CHECK EACH CELL FOR CHANGED BYTES.  REDRAW PIXELS FOR THE CHANGED BYTES
	// IN THE FRAME BUFFER.  MARK CELLS IN WHICH REDRAWING HAS TAKEN PLACE AS
	// DIRTY.
	videoptr  = MemGetMainPtr(displaypage2 ? PAG2OFFSET : PAG1OFFSET);

	ZeroMemory(celldirty, 40*32); // asdfg
	update = UpdateCell;
	anydirty = 0;
	y        = 0;
	ypixel   = 0;
	while (y < 24)
	{
		int offset = ((y & 7) << 7) + ((y >> 3) * 40);
		int x      = 0;
		int xpixel = 0;
		while (x < 40)
		{
			anydirty |= celldirty[x][y] = update(x,y,xpixel,ypixel,offset+x);
			++x;
			xpixel += 14;
		}
		++y;
		ypixel += 16;
	}
	if (!framedc || !anydirty)
	{
		FrameReleaseVideoDC();
		SetLastDrawnImage();
		redrawfull = 0;
		return;
	}

	// COPY DIRTY CELLS FROM THE DEVICE DEPENDENT BITMAP ONTO THE SCREEN
	// IN LONG HORIZONTAL RECTANGLES
	remainingdirty = 0;
	y              = 0;
	ypixel         = 0;
	while (y < 24)
	{
		int start  = -1;
		int startx = 0;
		int x      = 0;
		int xpixel = 0;
		while (x < 40)
		{
			if ((x == 39) && celldirty[x][y])
			if (start >= 0)
			{
				xpixel += 14;
				celldirty[x][y] = 0;
			}
			else
				remainingdirty = 1;
			if ((start >= 0) && !celldirty[x][y])
			{
				if ((x - startx > 1) || ((x == 39) && (xpixel == 560)))
				{
					int height = 1;
					while ((y+height < 24)
							&& celldirty[startx][y+height]
							&& celldirty[x-1][y+height]
							&& celldirty[(startx+x-1) >> 1][y+height])
					height++;
					BitBlt(framedc,start,ypixel,xpixel-start,height << 4,
								devicedc,start,ypixel,SRCCOPY);
					while (height--)
					{
						int loop = startx;
						while (loop < x+(xpixel == 560))
							celldirty[loop++][y+height] = 0;
					}
					start = -1;
				}
				else
					remainingdirty = 1;
				start = -1;
			}
			else if ((start == -1) && celldirty[x][y] && (x < 39))
			{
				start  = xpixel;
				startx = x;
			}
			x++;
			xpixel += 14;
		}
		y++;
		ypixel += 16;
	}

	// COPY ANY REMAINING DIRTY CELLS FROM THE DEVICE DEPENDENT BITMAP
	// ONTO THE SCREEN IN VERTICAL RECTANGLES
	if (remainingdirty)
	{
		int x      = 0;
		int xpixel = 0;
		while (x < 40)
		{
			int start  = -1;
			int y      = 0;
			int ypixel = 0;
			while (y < 24)
			{
				if ((y == 23) && celldirty[x][y])
				{
					if (start == -1)
						start = ypixel;
					ypixel += 16;
					celldirty[x][y] = 0;
				}
				if ((start >= 0) && !celldirty[x][y])
				{
					BitBlt(framedc,xpixel,start,14,ypixel-start,
					devicedc,xpixel,start,SRCCOPY);
					start = -1;
				}
				else if ((start == -1) && celldirty[x][y])
					start = ypixel;
				y++;
				ypixel += 16;
			}
			x++;
			xpixel += 14;
		}
	}
	GdiFlush();
	FrameReleaseVideoDC();
	SetLastDrawnImage();
	redrawfull = 0;
}

//===========================================================================
void VideoReinitialize ()
{
	CreateIdentityPalette();
	CreateDIBSections();
}

//===========================================================================
void VideoResetState()
{
	displaypage2 = 0;
	vidmode      = VF_COLOR;
	redrawfull   = 1;
}

//===========================================================================
BYTE __stdcall VideoSetMode (WORD programcounter, BYTE address, BYTE write, BYTE value)
{
	DWORD oldpage2 = SW_PAGE2;
	DWORD oldvalue = vidmode;

	switch (address)
	{
		case 0x50: vidmode |=  VF_COLOR;   break;
		case 0x51: vidmode &= ~VF_COLOR;   break;
		case 0x54: vidmode &= ~VF_PAGE2;   break;
		case 0x55: vidmode |=  VF_PAGE2;   break;
	}
	if (oldvalue != vidmode)
	{
		redrawfull   = 1;
		VideoReinitialize();  // for color change
	}
	if (fullspeed && oldpage2 && !SW_PAGE2)
	{
		static DWORD lasttime = 0;
		DWORD currtime = GetTickCount();
		if (currtime-lasttime >= 20)
			lasttime = currtime;
		else
			oldpage2 = SW_PAGE2;
	}
	if (oldpage2 != SW_PAGE2)
	{
		static DWORD lastrefresh = 0;
		BOOL fastvideoslowcpu = 0;
		if ((emulmsec-lastrefresh >= 20))
			fastvideoslowcpu = 1;
		if ((displaypage2 && !SW_PAGE2) || (!behind) || fastvideoslowcpu)
		{
			displaypage2 = (SW_PAGE2 != 0);
			if (!redrawfull)
			{
				VideoRefreshScreen();
				hasrefreshed = 1;
				lastrefresh  = emulmsec;
			}
		}
		else if ((!SW_PAGE2) && (!redrawfull) && (emulmsec-lastrefresh >= 20))
		{
			displaypage2 = 0;
			VideoRefreshScreen();
			hasrefreshed = 1;
			lastrefresh  = emulmsec;
		}
		lastpageflip = emulmsec;
	}
	return MemReturnRandomData(1);
}

/*
//===========================================================================
void VideoUpdateVbl (DWORD cycles, BOOL nearrefresh)
{
	if (vblcounter)
		vblcounter -= MIN(vblcounter,(cycles >> 6));
	else if (!nearrefresh)
		vblcounter = 250;
}
*/

// EOF