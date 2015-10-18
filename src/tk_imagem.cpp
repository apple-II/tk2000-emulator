/****************************************************************************
*
*  APPLE //E EMULATOR FOR WINDOWS                    
*
*  Copyright (C) 1994-96, Michael O'Brien.  All rights reserved.
*
***/

/*  Emulador do computador TK3000 //e (Microdigital)
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

// Funções para manipulação de imagens de diskete

#include "tk_stdhdr.h"
#include "tk_imagem.h"
#include "tk_memoria.h"
#include "tk_cpu.h"
#include "tk_disco.h"

/* DO logical order  0 1 2 3 4 5 6 7 8 9 A B C D E F */
/*    physical order 0 D B 9 7 5 3 1 E C A 8 6 4 2 F */

/* PO logical order  0 E D C B A 9 8 7 6 5 4 3 2 1 F */
/*    physical order 0 2 4 6 8 A C E 1 3 5 7 9 B D F */

#define  IMAGETYPES  7
#define  NIBBLES     6656

// imageinfoptr

typedef BOOL  (*boottype  )(imageinfoptr);
typedef DWORD (*detecttype)(BYTE*,DWORD);
typedef void  (*readtype  )(imageinfoptr,int,int,BYTE*,int *);
typedef void  (*writetype )(imageinfoptr,int,int,BYTE*,int);

BOOL  AplBoot (imageinfoptr ptr);
DWORD AplDetect (BYTE* imageptr, DWORD imagesize);
DWORD DoDetect (BYTE* imageptr, DWORD imagesize);
void  DoRead (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimagebuffer, int *nibbles);
void  DoWrite (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimage, int nibbles);
DWORD IieDetect (BYTE* imageptr, DWORD imagesize);
void  IieRead (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimagebuffer, int *nibbles);
void  IieWrite (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimage, int nibbles);
DWORD Nib1Detect (BYTE* imageptr, DWORD imagesize);
void  Nib1Read (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimagebuffer, int *nibbles);
void  Nib1Write (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimage, int nibbles);
DWORD Nib2Detect (BYTE* imageptr, DWORD imagesize);
void  Nib2Read (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimagebuffer, int *nibbles);
void  Nib2Write (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimage, int nibbles);
DWORD PoDetect (BYTE* imageptr, DWORD imagesize);
void  PoRead (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimagebuffer, int *nibbles);
void  PoWrite (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimage, int nibbles);
BOOL  PrgBoot (imageinfoptr ptr);
DWORD PrgDetect (BYTE* imageptr, DWORD imagesize);

typedef struct _imagetyperec
{
	char*      createexts;
	char*      rejectexts;
	detecttype detect;
	boottype   boot;
	readtype   read;
	writetype  write;
	int        TamPadrao;
} imagetyperec, *imagetypeptr;

imagetyperec imagetype[IMAGETYPES] =
{
	{
		".prg",
		".do;.dsk;.iie;.nib;.po",
		PrgDetect,
		PrgBoot,
		NULL,
		NULL,
		0
	},
	{
		".do;.dsk",
		".nib;.iie;.po;.prg",
		DoDetect,
		NULL,
		DoRead,
		DoWrite,
		143360
	},
	{
		".po",
		".do;.iie;.nib;.prg",
		PoDetect,
		NULL,
		PoRead,
		PoWrite,
		143360
	},
	{
		".apl",
		".do;.dsk;.iie;.nib;.po",
		AplDetect,
		AplBoot,
		NULL,
		NULL,
		0
	},
	{
		".nib",
		".do;.iie;.po;.prg",
		Nib1Detect,
		NULL,
		Nib1Read,
		Nib1Write,
		232960
	},
	{
		".nb2",
		".do;.iie;.po;.prg",
		Nib2Detect,
		NULL,
		Nib2Read,
		Nib2Write,
		223440
	},
	{
		".iie",
		".do.;.nib;.po;.prg",
		IieDetect,
		NULL,
		IieRead,
		IieWrite,
		0
	}
};

BYTE diskbyte[0x40] = {0x96,0x97,0x9A,0x9B,0x9D,0x9E,0x9F,0xA6,
                       0xA7,0xAB,0xAC,0xAD,0xAE,0xAF,0xB2,0xB3,
                       0xB4,0xB5,0xB6,0xB7,0xB9,0xBA,0xBB,0xBC,
                       0xBD,0xBE,0xBF,0xCB,0xCD,0xCE,0xCF,0xD3,
                       0xD6,0xD7,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,
                       0xDF,0xE5,0xE6,0xE7,0xE9,0xEA,0xEB,0xEC,
                       0xED,0xEE,0xEF,0xF2,0xF3,0xF4,0xF5,0xF6,
                       0xF7,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF};

BYTE sectornumber[3][0x10] = {{0x00,0x08,0x01,0x09,0x02,0x0A,0x03,0x0B,
                               0x04,0x0C,0x05,0x0D,0x06,0x0E,0x07,0x0F},
                              {0x00,0x07,0x0E,0x06,0x0D,0x05,0x0C,0x04,
                               0x0B,0x03,0x0A,0x02,0x09,0x01,0x08,0x0F},
                              {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                               0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};

BYTE* workbuffer = NULL;
BYTE   VolumeAtual = VOLUME;

/****************************************************************************
*
*  NIBBLIZATION FUNCTIONS
*
***/

//===========================================================================
BYTE* Code62 (int sector)
{
    int loop = 0;

  // CONVERT THE 256 8-BIT BYTES INTO 342 6-BIT BYTES, WHICH WE STORE
  // STARTING AT 4K INTO THE WORK BUFFER.
  {
    BYTE* sectorbase = workbuffer+(sector << 8);
    BYTE* resultptr  = workbuffer+0x1000;
    BYTE   offset     = 0xAC;
    while (offset != 0x02) {
      BYTE value = 0;
#define ADDVALUE(a) value = (value << 2) |        \
                            (((a) & 0x01) << 1) | \
                            (((a) & 0x02) >> 1)
      ADDVALUE(*(sectorbase+offset));  offset -= 0x56;
      ADDVALUE(*(sectorbase+offset));  offset -= 0x56;
      ADDVALUE(*(sectorbase+offset));  offset -= 0x53;
#undef ADDVALUE
      *(resultptr++) = value << 2;
    }
    *(resultptr-2) &= 0x3F;
    *(resultptr-1) &= 0x3F;
    loop = 0;
    while (loop < 0x100)
      *(resultptr++) = *(sectorbase+(loop++));
  }

  // EXCLUSIVE-OR THE ENTIRE DATA BLOCK WITH ITSELF OFFSET BY ONE BYTE,
  // CREATING A 343RD BYTE WHICH IS USED AS A CHECKSUM.  STORE THE NEW
  // BLOCK OF 343 BYTES STARTING AT 5K INTO THE WORK BUFFER.
  {
    BYTE   savedval  = 0;
    BYTE* sourceptr = workbuffer+0x1000;
    BYTE* resultptr = workbuffer+0x1400;
    int    loop      = 342;
    while (loop--) {
      *(resultptr++) = savedval ^ *sourceptr;
      savedval = *(sourceptr++);
    }
    *resultptr = savedval;
  }

  // USING A LOOKUP TABLE, CONVERT THE 6-BIT BYTES INTO DISK BYTES.  A VALID
  // DISK BYTE IS A BYTE THAT HAS THE HIGH BIT SET, AT LEAST TWO ADJACENT
  // BITS SET (EXCLUDING THE HIGH BIT), AND AT MOST ONE PAIR OF CONSECUTIVE
  // ZERO BITS.  THE CONVERTED BLOCK OF 343 BYTES IS STORED STARTING AT 4K
  // INTO THE WORK BUFFER.
  {
    BYTE* sourceptr = workbuffer+0x1400;
    BYTE* resultptr = workbuffer+0x1000;
    int    loop      = 343;
    while (loop--)
      *(resultptr++) = diskbyte[(*(sourceptr++)) >> 2];
  }

  return workbuffer+0x1000;
}

//===========================================================================
void Decode62 (BYTE* imageptr)
{
    int loop = 0;

  // IF WE HAVEN'T ALREADY DONE SO, GENERATE A TABLE FOR CONVERTING
  // DISK BYTES BACK INTO 6-BIT BYTES
  static BOOL tablegenerated = 0;
  static BYTE sixbitbyte[0x80];
  if (!tablegenerated) {
    //ZeroMemory(sixbitbyte,0x80);
	  memset(sixbitbyte, 0, 0x80);
    loop = 0;
    while (loop < 0x40) {
      sixbitbyte[diskbyte[loop]-0x80] = loop << 2;
      loop++;
    }
    tablegenerated = 1;
  }

  // USING OUR TABLE, CONVERT THE DISK BYTES BACK INTO 6-BIT BYTES
  {
    BYTE* sourceptr = workbuffer+0x1000;
    BYTE* resultptr = workbuffer+0x1400;
    int    loop      = 343;
    while (loop--)
      *(resultptr++) = sixbitbyte[*(sourceptr++) & 0x7F];
  }

  // EXCLUSIVE-OR THE ENTIRE DATA BLOCK WITH ITSELF OFFSET BY ONE BYTE
  // TO UNDO THE EFFECTS OF THE CHECKSUMMING PROCESS
  {
    BYTE   savedval  = 0;
    BYTE* sourceptr = workbuffer+0x1400;
    BYTE* resultptr = workbuffer+0x1000;
    int    loop      = 342;
    while (loop--) {
      *resultptr = savedval ^ *(sourceptr++);
      savedval = *(resultptr++);
    }
  }

  // CONVERT THE 342 6-BIT BYTES INTO 256 8-BIT BYTES
  {
    BYTE* lowbitsptr = workbuffer+0x1000;
    BYTE* sectorbase = workbuffer+0x1056;
    BYTE   offset     = 0xAC;
    while (offset != 0x02) {
      if (offset >= 0xAC)
        *(imageptr+offset) = (*(sectorbase+offset) & 0xFC)
                               | (((*lowbitsptr) & 0x80) >> 7)
                               | (((*lowbitsptr) & 0x40) >> 5);
      offset -= 0x56;
      *(imageptr+offset) = (*(sectorbase+offset) & 0xFC)
                             | (((*lowbitsptr) & 0x20) >> 5)
                             | (((*lowbitsptr) & 0x10) >> 3);
      offset -= 0x56;
      *(imageptr+offset) = (*(sectorbase+offset) & 0xFC)
                             | (((*lowbitsptr) & 0x08) >> 3)
                             | (((*lowbitsptr) & 0x04) >> 1);
      offset -= 0x53;
      lowbitsptr++;
    }
  }

}

//===========================================================================
void DenibblizeTrack (BYTE* trackimage, BOOL dosorder, int nibbles)
{
	memset(workbuffer, 0, 0x1000);

	// SEARCH THROUGH THE TRACK IMAGE FOR EACH SECTOR.  FOR EVERY SECTOR
	// WE FIND, COPY THE NIBBLIZED DATA FOR THAT SECTOR INTO THE WORK
	// BUFFER AT OFFSET 4K.  THEN CALL DECODE62() TO DENIBBLIZE THE DATA
	// IN THE BUFFER AND WRITE IT INTO THE FIRST PART OF THE WORK BUFFER
	// OFFSET BY THE SECTOR NUMBER.
	{
		int offset    = 0;
		int partsleft = 33;
		int sector    = 0;
		while (partsleft--)
		{
			BYTE byteval[3] = {0,0,0};
			int  bytenum    = 0;
			int  loop       = nibbles;
			while ((loop--) && (bytenum < 3))
			{
				if (bytenum)
					byteval[bytenum++] = *(trackimage+offset++);
				else if (*(trackimage+offset++) == 0xD5)
					bytenum = 1;
				if (offset >= nibbles)
					offset = 0;
			}
			if ((bytenum == 3) && (byteval[1] = 0xAA))
			{
				int loop       = 0;
				int tempoffset = offset;
				while (loop < 384)
				{
					*(workbuffer+0x1000+loop++) = *(trackimage+tempoffset++);
					if (tempoffset >= nibbles)
						tempoffset = 0;
				}
				if (byteval[2] == 0x96)
					sector = ((*(workbuffer+0x1004) & 0x55) << 1)
								| (*(workbuffer+0x1005) & 0x55);
				else if (byteval[2] == 0xAD)
				{
					Decode62(workbuffer+(sectornumber[dosorder][sector] << 8));
					sector = 0;
				}
			}
		}
	}
}

//===========================================================================
DWORD NibblizeTrack(BYTE* trackimagebuffer, BOOL dosorder, int track)
{
	BYTE* imageptr = trackimagebuffer;
	BYTE   sector   = 0;
	int loop;

	memset(workbuffer+4096, 0, 4096);
	// WRITE GAP ONE, WHICH CONTAINS 48 SELF-SYNC BYTES
	for (loop = 0; loop < 48; loop++)
		*(imageptr++) = 0xFF;

	while (sector < 16)
	{
		// WRITE THE ADDRESS FIELD, WHICH CONTAINS:
		//   - PROLOGUE (D5AA96)
		//   - VOLUME NUMBER ("4 AND 4" ENCODED)
		//   - TRACK NUMBER ("4 AND 4" ENCODED)
		//   - SECTOR NUMBER ("4 AND 4" ENCODED)
		//   - CHECKSUM ("4 AND 4" ENCODED)
		//   - EPILOGUE (DEAAEB)
		*(imageptr++) = 0xD5;
		*(imageptr++) = 0xAA;
		*(imageptr++) = 0x96;
#define CODE44A(a) ((((a) >> 1) & 0x55) | 0xAA)
#define CODE44B(a) (((a) & 0x55) | 0xAA)
		*(imageptr++) = CODE44A(VolumeAtual);
		*(imageptr++) = CODE44B(VolumeAtual);
		*(imageptr++) = CODE44A((BYTE)track);
		*(imageptr++) = CODE44B((BYTE)track);
		*(imageptr++) = CODE44A(sector);
		*(imageptr++) = CODE44B(sector);
		*(imageptr++) = CODE44A(VolumeAtual ^ ((BYTE)track) ^ sector);
		*(imageptr++) = CODE44B(VolumeAtual ^ ((BYTE)track) ^ sector);
#undef CODE44A
#undef CODE44B
		*(imageptr++) = 0xDE;
		*(imageptr++) = 0xAA;
		*(imageptr++) = 0xEB;

		// WRITE GAP TWO, WHICH CONTAINS SIX SELF-SYNC BYTES
		for (loop = 0; loop < 6; loop++)
			*(imageptr++) = 0xFF;

		// WRITE THE DATA FIELD, WHICH CONTAINS:
		//   - PROLOGUE (D5AAAD)
		//   - 343 6-BIT BYTES OF NIBBLIZED DATA, INCLUDING A 6-BIT CHECKSUM
		//   - EPILOGUE (DEAAEB)
		*(imageptr++) = 0xD5;
		*(imageptr++) = 0xAA;
		*(imageptr++) = 0xAD;
		//CopyMemory(imageptr,Code62(sectornumber[dosorder][sector]),343);
		memcpy(imageptr,Code62(sectornumber[dosorder][sector]),343);
		imageptr += 343;
		*(imageptr++) = 0xDE;
		*(imageptr++) = 0xAA;
		*(imageptr++) = 0xEB;

		// WRITE GAP THREE, WHICH CONTAINS 27 SELF-SYNC BYTES
		for (loop = 0; loop < 27; loop++)
			*(imageptr++) = 0xFF;

		sector++;
	}
	return imageptr-trackimagebuffer;
}

//===========================================================================
void SkewTrack(int track, int nibbles, BYTE* trackimagebuffer)
{
	int skewbytes = (track*768) % nibbles;
	memcpy(workbuffer,trackimagebuffer,nibbles);
	memcpy(trackimagebuffer,workbuffer+skewbytes,nibbles-skewbytes);
	memcpy(trackimagebuffer+nibbles-skewbytes,workbuffer,skewbytes);
}

//===========================================================================
void CarregarNaMemoria()
{
	//
}

/****************************************************************************
*
*  RAW PROGRAM IMAGE (APL) FORMAT IMPLEMENTATION
*
***/

//===========================================================================
BOOL AplBoot (imageinfoptr ptr)
{
	WORD address = 0;
	WORD length  = 0;
	int loop;

	// Zip support
	memcpy(&address, ptr->imagem, sizeof(WORD));
	memcpy(&length, ptr->imagem+sizeof(WORD), sizeof(WORD));

//	fseek(ptr->file, 0, SEEK_SET);
//	fread(&address, 1, sizeof(WORD), ptr->file);
//	fread(&length,  1, sizeof(WORD), ptr->file);
	if ((((WORD)(address+length)) <= address) ||
			(address >= 0xC000) ||
			(address+length-1 >= 0xC000))
		return 0;
//	fread(mem+address, 1, sizeof(WORD), ptr->file);
	// TODO: Implementar o "BOOT"
	// memcpy(mem+address, ptr->imagem+128, length);

	loop = 192;
	while (loop--)
		*(memdirty+loop) = (char)0xFF;
	regs.pc = address;

	return 1;
}

//===========================================================================
DWORD AplDetect(BYTE* imageptr, DWORD imagesize)
{
	DWORD length = *(WORD*)(imageptr+2);
	return (((length+4) == imagesize) ||
			((length+4+((256-((length+4) & 255)) & 255)) == imagesize));
}

/****************************************************************************
*
*  DOS ORDER (DO) FORMAT IMPLEMENTATION
*
***/

//===========================================================================
DWORD DoDetect (BYTE* imageptr, DWORD imagesize)
{
  if (((imagesize < 143105) || (imagesize > 143364)) &&
      (imagesize != 143403) && (imagesize != 143488))
    return 0;

  // CHECK FOR A DOS ORDER IMAGE OF A DOS DISKETTE
  {
    int  loop     = 0;
    BOOL mismatch = 0;
    while ((loop++ < 15) && !mismatch)
      if (*(imageptr+0x11002+(loop << 8)) != loop-1)
        mismatch = 1;
    if (!mismatch)
      return 2;
  }

  // CHECK FOR A DOS ORDER IMAGE OF A PRODOS DISKETTE
  {
    int  loop     = 1;
    BOOL mismatch = 0;
    while ((loop++ < 5) && !mismatch)
      if ((*(WORD*)(imageptr+(loop << 9)+0x100) != ((loop == 5) ? 0 : 6-loop)) ||
          (*(WORD*)(imageptr+(loop << 9)+0x102) != ((loop == 2) ? 0 : 8-loop)))
        mismatch = 1;
    if (!mismatch)
      return 2;
  }

  return 1;
}

//===========================================================================
void DoRead (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimagebuffer, int *nibbles)
{
	int Posicao = ptr->offset+(track << 12);
//	fseek(ptr->file, ptr->offset+(track << 12), SEEK_SET);
	memset(workbuffer, 0, 4096);
	VolumeAtual = ptr->Volume;
//	fread(workbuffer, 4096, 1, ptr->file);
	memcpy(workbuffer, ptr->imagem+Posicao, 4096);
	*nibbles = NibblizeTrack(trackimagebuffer,1,track);
	if (!enhancedisk)
		SkewTrack(track,*nibbles,trackimagebuffer);
}

//===========================================================================
void DoWrite (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimage, int nibbles)
{
	int Posicao = ptr->offset+(track << 12);

	memset(workbuffer, 0, 4096);
	DenibblizeTrack(trackimage, 1, nibbles);
//	fseek(ptr->file, ptr->offset+(track << 12), SEEK_SET);
	VolumeAtual = ptr->Volume;
//	fwrite(workbuffer, 4096, 1, ptr->file);
	memcpy(ptr->imagem+Posicao, workbuffer, 4096);
}

/****************************************************************************
*
*  SIMSYSTEM IIE (IIE) FORMAT IMPLEMENTATION
*
***/

//===========================================================================
void IieConvertSectorOrder (BYTE* sourceorder)
{
	int loop = 16;
	while (loop--)
	{
		BYTE found = 0xFF;
		int  loop2 = 16;
		while (loop2-- && (found == 0xFF))
			if (*(sourceorder+loop2) == loop)
				found = loop2;
		if (found == 0xFF)
			found = 0;
		sectornumber[2][loop] = found;
	}
}

//===========================================================================
DWORD IieDetect (BYTE* imageptr, DWORD imagesize)
{
	if (strncmp((const char *)imageptr,"SIMSYSTEM_IIE",13) ||
			(*(imageptr+13) > 3))
		return 0;
	return 2;
}

//===========================================================================
void IieRead (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimagebuffer, int *nibbles)
{
	// IF WE HAVEN'T ALREADY DONE SO, READ THE IMAGE FILE HEADER
	if (!ptr->header)
	{
		//ptr->header = (BYTE*)VirtualAlloc(NULL,88,MEM_COMMIT,PAGE_READWRITE);
		ptr->header = (BYTE*)malloc(88);
		if (!ptr->header)
		{
			*nibbles = 0;
			return;
		}
		memset(ptr->header, 0, 88);
//		fseek(ptr->file, 0, SEEK_SET);
//		fread(ptr->header, 88, 1, ptr->file);
		memcpy(ptr->header, ptr->imagem, 88);
	}

	// IF THIS IMAGE CONTAINS USER DATA, READ THE TRACK AND NIBBLIZE IT
	if (*(ptr->header+13) <= 2)
	{
	  //    DWORD bytesread;
		IieConvertSectorOrder(ptr->header+14);
		//fseek(ptr->file, (track << 12)+30, SEEK_SET);
		memset(workbuffer, 0, 4096);
		//fread(workbuffer, 4096, 1, ptr->file);
		memcpy(workbuffer, ptr->imagem+((track << 12)+30), 4096);
		*nibbles = NibblizeTrack(trackimagebuffer,2,track);
	}
	// OTHERWISE, IF THIS IMAGE CONTAINS NIBBLE INFORMATION, READ IT
	// DIRECTLY INTO THE TRACK BUFFER
	else
	{
		DWORD offset = 88;
		*nibbles = *(WORD*)(ptr->header+(track << 1)+14);
		while (track--)
			offset += *(WORD*)(ptr->header+(track << 1)+14);
//		fseek(ptr->file, offset, SEEK_SET);
		memset(trackimagebuffer, 0, *nibbles);
//		fread(trackimagebuffer, *nibbles, 1, ptr->file);
		memcpy(trackimagebuffer, ptr->imagem+offset, *nibbles);
	}
}

//===========================================================================
void IieWrite(imageinfoptr ptr, int track, int quartertrack, BYTE* trackimage, int nibbles)
{
  // note: unimplemented
}

/****************************************************************************
*
*  NIBBLIZED 6656-NIBBLE (NIB) FORMAT IMPLEMENTATION
*
***/

//===========================================================================
DWORD Nib1Detect (BYTE* imageptr, DWORD imagesize)
{
	return (imagesize == 232960) ? 2 : 0;
}

//===========================================================================
void Nib1Read (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimagebuffer, int *nibbles)
{
	int Posicao = ptr->offset+track*NIBBLES;
//	fseek(ptr->file, ptr->offset+track*NIBBLES, SEEK_SET);
//	*nibbles = fread(trackimagebuffer, NIBBLES, 1, ptr->file);
	memcpy(trackimagebuffer, ptr->imagem+Posicao, NIBBLES);
	*nibbles = NIBBLES;
}

//===========================================================================
void Nib1Write (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimage, int nibbles)
{
	int Posicao = ptr->offset+track*NIBBLES;
//	fseek(ptr->file, ptr->offset+track*NIBBLES, SEEK_SET);
//	fwrite(trackimage, nibbles, 1, ptr->file);
	memcpy(ptr->imagem+Posicao, trackimage, nibbles);
}

/****************************************************************************
*
*  NIBBLIZED 6384-NIBBLE (NB2) FORMAT IMPLEMENTATION
*
***/

//===========================================================================
DWORD Nib2Detect (BYTE* imageptr, DWORD imagesize)
{
	return (imagesize == 223440) ? 2 : 0;
}

//===========================================================================
void Nib2Read(imageinfoptr ptr, int track, int quartertrack, BYTE* trackimagebuffer, int *nibbles)
{
	int Posicao = track*6384;

//	fseek(ptr->file, ptr->offset+track*6384, SEEK_SET);
//	*nibbles = fread(trackimagebuffer, 1, 6384, ptr->file);
	memcpy(trackimagebuffer, ptr->imagem+ptr->offset+Posicao, 6384);
	*nibbles = 6384;
}

//===========================================================================
void Nib2Write (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimage, int nibbles)
{
	int Posicao = track*6384;
//	fseek(ptr->file, ptr->offset+track*6384, SEEK_SET);
//	fwrite(trackimage, nibbles, 1, ptr->file);
	memcpy(ptr->imagem+ptr->offset+Posicao, trackimage, nibbles);
}

/****************************************************************************
*
*  PRODOS ORDER (PO) FORMAT IMPLEMENTATION
*
***/

//===========================================================================
DWORD PoDetect (BYTE* imageptr, DWORD imagesize)
{
	if (((imagesize < 143105) || (imagesize > 143364)) &&
			(imagesize != 143488))
		return 0;

	// CHECK FOR A PRODOS ORDER IMAGE OF A DOS DISKETTE
	{
		int  loop     = 4;
		BOOL mismatch = 0;
		while ((loop++ < 13) && !mismatch)
			if (*(imageptr+0x11002+(loop << 8)) != 14-loop)
				mismatch = 1;
		if (!mismatch)
			return 2;
	}

	// CHECK FOR A PRODOS ORDER IMAGE OF A PRODOS DISKETTE
	{
		int  loop     = 1;
		BOOL mismatch = 0;
		while ((loop++ < 5) && !mismatch)
			if ((*(WORD*)(imageptr+(loop << 9)  ) != ((loop == 2) ? 0 : loop-1)) ||
					(*(WORD*)(imageptr+(loop << 9)+2) != ((loop == 5) ? 0 : loop+1)))
				mismatch = 1;
		if (!mismatch)
			return 2;
	}

	return 1;
}

//===========================================================================
void PoRead (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimagebuffer, int *nibbles)
{
	int Posicao = ptr->offset+(track << 12);
//	fseek(ptr->file, ptr->offset+(track << 12), SEEK_SET);
//	memset(workbuffer, 0, 4096);
//	fread(workbuffer, 4096, 1, ptr->file);
	memcpy(workbuffer, ptr->imagem+Posicao, 4096);

	*nibbles = NibblizeTrack(trackimagebuffer,0,track);
	if (!enhancedisk)
		SkewTrack(track,*nibbles,trackimagebuffer);
}

//===========================================================================
void PoWrite (imageinfoptr ptr, int track, int quartertrack, BYTE* trackimage, int nibbles)
{
	int Posicao = ptr->offset+(track << 12);

	memset(workbuffer, 0, 4096);
	DenibblizeTrack(trackimage, 0, nibbles);
//	fseek(ptr->file, ptr->offset+(track << 12), SEEK_SET);
//	fwrite(workbuffer, 4096, 1, ptr->file);
	memcpy(ptr->imagem+Posicao, workbuffer, 4096);
}

/****************************************************************************
*
*  PRODOS PROGRAM IMAGE (PRG) FORMAT IMPLEMENTATION
*
***/

//===========================================================================
BOOL PrgBoot (imageinfoptr ptr)
{
	WORD address = 0;
	WORD length  = 0;
	int loop;

//	fseek(ptr->file, 5, SEEK_SET);
//	fread(&address, sizeof(WORD), 1, ptr->file);
//	fread(&length, sizeof(WORD), 1, ptr->file);

	// Zip support
	memcpy(&address, ptr->imagem, sizeof(WORD));
	memcpy(&length, ptr->imagem+sizeof(WORD), sizeof(WORD));
	length <<= 1;
	if ((((WORD)(address+length)) <= address) ||
			(address >= 0xC000) ||
			(address+length-1 >= 0xC000))
		return 0;
	// TODO: Implementar o "BOOT"
	// memcpy(mem+address, ptr->imagem+128, length);
	loop = 192;
	while (loop--)
		*(memdirty+loop) = (char)0xFF;
	regs.pc = address;
	return 1;
}

//===========================================================================
DWORD PrgDetect (BYTE* imageptr, DWORD imagesize)
{
	return (*(DWORD*)imageptr == 0x214C470A) ? 2 : 0;
}

//
// ----- ALL GLOBALLY ACCESSIBLE FUNCTIONS ARE BELOW THIS LINE -----
//

//===========================================================================
BOOL ImageBoot (HIMAGE imagehandle)
{
	imageinfoptr ptr = (imageinfoptr)imagehandle;
	BOOL result = 0;

	if (imagetype[ptr->format].boot)
		result = imagetype[ptr->format].boot(ptr);
	if (result)
		ptr->writeprotected = 1;
	return result;
}

//===========================================================================
void ImageClose (HIMAGE imagehandle)
{
	int  track;
	FILE *Arquivo;

	imageinfoptr ptr = (imageinfoptr)imagehandle;

	if (ptr->Mudado && !ptr->writeprotected)
	{
/*
		if (ptr->arquivozip) // Zip Support
		{
			zipFile ArqZip;
			zip_fileinfo InfArq;
			
			memset(&InfArq, 0, sizeof(InfArq));
			ArqZip = zipOpen(ptr->filename, APPEND_STATUS_CREATE);
			if (ArqZip)
			{
				if (zipOpenNewFileInZip(ArqZip,
									ptr->arquivozip,
									&InfArq,
									NULL,
									0,
									NULL,
									0,
									NULL,
									Z_DEFLATED,
									Z_BEST_COMPRESSION) == ZIP_OK)
				{
					zipWriteInFileInZip(ArqZip,
											ptr->imagem,
											ptr->tamanho);
					zipCloseFileInZip(ArqZip);
				} // if zipOpenNew...
				zipClose(ArqZip, NULL);
			} // if ArqZip
		} // if ptr->arquivozip
		else
*/
		{
			// Salvar pro arquivo
			if (!(Arquivo = fopen(ptr->filename, "wb")))
				return;
			fwrite(ptr->imagem, ptr->tamanho, 1, Arquivo);
			fclose(Arquivo); 
		}
	}
//	if (ptr->file)
//		fclose(ptr->file);
	for (track = 0; track < TRACKS; track++)
		if (!ptr->validtrack[track])
		{
			unlink(ptr->filename);
			break;
		}
	if (ptr->header)
		free(ptr->header);
//	if (ptr->imagem)			// Zip Support
//		free(ptr->imagem);
	free(ptr);
}

//===========================================================================
void ImageDestroy()
{
	free(workbuffer);
	workbuffer = NULL;
}

//===========================================================================
void ImageInitialize ()
{
	workbuffer = (BYTE*)malloc(0x2000);
}

//===========================================================================
int ImageOpen (char*  imagefilename,
			   BYTE   *Volume,
               HIMAGE *imagehandle,
               BOOL   *writeprotected,
               BOOL   createifnecessary)
{
	FILE  *file = NULL;
	char* imagefileext = imagefilename;
	char  ext[_MAX_EXT];
	char  imagename[MAX_PATH];
	DWORD size = 0, size2 = 0;
	BYTE* view;
	BYTE* imageptr;
	DWORD format;
	DWORD possibleformat;
	int   loop;
	int   track;
//	char  *BufferZip = NULL;
//	char  *ZipName;

	if (!(imagefilename && imagehandle && writeprotected && workbuffer))
		return -1;

	memset(imagename, 0, MAX_PATH);
	// DETERMINE THE FILE'S EXTENSION AND CONVERT IT TO LOWERCASE
	imagefileext = imagefilename;
	if (strrchr(imagefileext,'\\'))
		imagefileext = strrchr(imagefileext,'\\')+1;
	if (strrchr(imagefileext,'.'))
		imagefileext = strrchr(imagefileext,'.');
	strncpy(ext,imagefileext,_MAX_EXT);
	CharLowerBuff(ext,strlen(ext));

	
	loop = strlen(imagefilename);
	while (loop--)
	{
		if ((imagefilename[loop] == '\\') ||
					(imagefilename[loop] == ':'))
		{
			char* p = strrchr(imagefilename,'.');
			if (p)
				strncpy(imagename, imagefilename+loop+1, p - (imagefilename+loop+1));
			else
				strcpy(imagename, imagefilename+loop+1);
			strcat(imagename, ".dsk");
			break;
		}
	}
/*
	// ZIP support:
	if (!strcmp(ext, ".zip"))
	{
		unzFile       ArqZip;
		unz_file_info InfArq;

		// Verifica se o arquivo ZIP está como somente-leitura
		if (!*writeprotected)
			file = fopen(imagefilename, "rb+");
		if (file == NULL)
		{
			file = fopen(imagefilename, "rb");
			if (file != NULL)
				*writeprotected = 1;
		}
		if ((file == NULL) && createifnecessary)	// Cria novo ZIP
		{
			zip_fileinfo zInfArq;
			char         BufferVazio[1] = {0};
			
			memset(&zInfArq, 0, sizeof(zInfArq));
			ArqZip = zipOpen(imagefilename, APPEND_STATUS_CREATE);
			if (!ArqZip)
				return 1;

			if (zipOpenNewFileInZip(ArqZip,
									imagename,
									&zInfArq,
									NULL,
									0,
									NULL,
									0,
									NULL,
									Z_DEFLATED,
									Z_BEST_COMPRESSION) != ZIP_OK)
				{
					return 1;
				}
				
			zipWriteInFileInZip(ArqZip,
								BufferVazio,
								0);
			zipCloseFileInZip(ArqZip);
			zipClose(ArqZip, NULL);
			file = fopen(imagefilename, "r");		// para enganar a verificação abaixo;

		}

		// IF WE AREN'T ABLE TO OPEN THE FILE, RETURN
		if (file == NULL)
			return 1;
		else
			fclose(file);			// Quem abre o arquivo é o unzOpen()

		ArqZip = unzOpen(imagefilename);
		if (!ArqZip)
		{
			return 1;
		}
		if (unzGoToFirstFile(ArqZip) != UNZ_OK)
		{
			unzClose(ArqZip);
			return 1;
		}
		ZipName = (char *)malloc(MAX_PATH);
		if (!ZipName)
		{
			return 1;
		}
		if (unzGetCurrentFileInfo(ArqZip,
									&InfArq,
									ZipName,
									MAX_PATH,
									NULL,
									0,
									NULL,
									0) != UNZ_OK)
		{
			unzClose(ArqZip);
			return 1;
		}
		if (strrchr(ZipName, '.'))
			strcpy(ext, strrchr(ZipName, '.'));
		if (InfArq.uncompressed_size == 0)
		{
			size = 0;
			BufferZip = (char *)malloc(1);
		}
		else
		{
			size = InfArq.uncompressed_size;
			BufferZip = (char *)malloc(size);
			if (!BufferZip)
			{
				unzClose(ArqZip);
				return 1;
			}
			if (unzOpenCurrentFile(ArqZip) != UNZ_OK)
			{
				unzClose(ArqZip);
				return 1;
			}
			if (unzReadCurrentFile(ArqZip,
									BufferZip,
									size) != (int)size)
			{
				unzClose(ArqZip);
				return 1;
			}
			if (unzCloseCurrentFile(ArqZip) != UNZ_OK)
			{
				unzClose(ArqZip);
				return 1;
			}
		}
		unzClose(ArqZip);
	}
*/
//	if (!BufferZip)
	{
		// TRY TO OPEN THE IMAGE FILE
		if (!*writeprotected)
			file = fopen(imagefilename, "rb+");
		if (file == NULL)
		{
			file = fopen(imagefilename, "rb");
			if (file != NULL)
				*writeprotected = 1;
		}
		if ((file == NULL) && createifnecessary)
			file = fopen(imagefilename, "wb+");

		// IF WE AREN'T ABLE TO OPEN THE FILE, RETURN
		if (file == NULL)
			return 1;
		fseek(file, 0, SEEK_END);
		size     = ftell(file); //GetFileSize(file,NULL);
	}

	view     = NULL;
	imageptr = NULL;
	format   = 0xFFFFFFFF;
	if (size > 0)
	{
		// MAP THE FILE INTO MEMORY FOR USE BY THE DETECTION FUNCTIONS

		view = (unsigned char *)malloc(size);
//		if (!BufferZip)
		{
			fseek(file, 0, SEEK_SET);
			fread(view, size, 1, file);
			imageptr = view;
		}
/*		else
		{
			memcpy(view, BufferZip, size);
			imageptr = view;
		}
*/
		if (imageptr)
		{
			// DETERMINE WHETHER THE FILE HAS A 128-BYTE MACBINARY HEADER
			if ((size > 128) &&
					(!*imageptr) &&
					(*(imageptr+1) < 120) &&
					(!*(imageptr+*(imageptr+1)+2)) &&
					(*(imageptr+0x7A) == 0x81) &&
					(*(imageptr+0x7B) == 0x81))
			{
				imageptr += 128;
				size     -= 128;
			}

			// CALL THE DETECTION FUNCTIONS IN ORDER, LOOKING FOR A MATCH
			possibleformat = 0xFFFFFFFF;
			loop           = 0;
			while ((loop < IMAGETYPES) && (format == 0xFFFFFFFF))
			{
				if (*ext && strstr(imagetype[loop].rejectexts,ext))
					++loop;
				else
				{
					DWORD result = imagetype[loop].detect(imageptr,size);
					if (result == 2)
						format = loop;
					else if ((result == 1) && (possibleformat == 0xFFFFFFFF))
						possibleformat = loop++;
					else
						++loop;
				}
			}
			if (format == 0xFFFFFFFF)
				format = possibleformat;

			// CLOSE THE MEMORY MAPPING
			free(view);
		}
	}
	else
	{

		// WE CREATE ONLY DOS ORDER (DO) OR 6656-NIBBLE (NIB) FORMAT FILES
		for (loop = 1; loop <= 4; loop += 3)
			if (*ext && strstr(imagetype[loop].createexts,ext))
			{
				format = loop;
				size2  = imagetype[loop].TamPadrao;
				imageptr = view = 0;
				break;
			}
	}

	// IF THE FILE DOES MATCH A KNOWN FORMAT...
	if (format != 0xFFFFFFFF)
	{

		// CREATE A RECORD FOR THE FILE, AND RETURN AN IMAGE HANDLE
		*imagehandle = (HIMAGE)malloc(sizeof(imageinforec));
		if (*imagehandle)
		{
			BYTE *imagem;

			// Volume
			memset(*imagehandle, 0, sizeof(imageinforec));
			strncpy(((imageinfoptr)*imagehandle)->filename,imagefilename, MAX_PATH);
			((imageinfoptr)*imagehandle)->format         = format;
//			((imageinfoptr)*imagehandle)->file           = file;
			((imageinfoptr)*imagehandle)->offset         = imageptr-view;
			((imageinfoptr)*imagehandle)->writeprotected = *writeprotected;
			((imageinfoptr)*imagehandle)->Mudado         = 0;
			if (size)
			{
				imagem = (BYTE*)malloc(size);
//				if (!BufferZip)  // zip support
				{
					fseek(file, 0, SEEK_SET);
					fread(imagem, size, 1, file);
//					((imageinfoptr)*imagehandle)->arquivozip = NULL; // sinaliza arquivo normal
				}
/*				else
				{
					memcpy(imagem, BufferZip, size);
					((imageinfoptr)*imagehandle)->arquivozip = ZipName;
				}
*/				((imageinfoptr)*imagehandle)->imagem  = imagem;  // Buffer
				((imageinfoptr)*imagehandle)->tamanho = size;    // Tamanho do Buffer
			}
			if (size2)
			{
				imagem = (BYTE*)malloc(size2);
/*				if (!BufferZip)  // zip support
				{
					((imageinfoptr)*imagehandle)->arquivozip = NULL; // sinaliza arquivo normal
				}
				else
				{
					((imageinfoptr)*imagehandle)->arquivozip = ZipName;
				}
*/				((imageinfoptr)*imagehandle)->imagem  = imagem;  // Buffer
				((imageinfoptr)*imagehandle)->tamanho = size2;   // Tamanho do Buffer
			}
			for (track = 0; track < TRACKS; track++)
				((imageinfoptr)*imagehandle)->validtrack[track] = (size > 0);
			return 0;
		}
	}
//	if (BufferZip)
//		free(BufferZip);		// Zip Support
	if (file)
		fclose(file);
	if (!(size > 0))
		unlink(imagefilename);
	return 2;
}

//===========================================================================
void ImageReadTrack (HIMAGE  imagehandle,
                     int     track,
                     int     quartertrack,
                     BYTE*  trackimagebuffer,
                     int    *nibbles)
{
	imageinfoptr ptr = (imageinfoptr)imagehandle;
	
	if (imagetype[ptr->format].read && ptr->validtrack[track])
		imagetype[ptr->format].read(ptr, track, quartertrack, trackimagebuffer, nibbles);
	else
		for (*nibbles = 0; *nibbles < NIBBLES; (*nibbles)++)
			trackimagebuffer[*nibbles] = (BYTE)(rand() & 0xFF);
}

//===========================================================================
void ImageWriteTrack (HIMAGE imagehandle,
                      int    track,
                      int    quartertrack,
                      BYTE* trackimage,
                      int    nibbles)
{
	imageinfoptr ptr = (imageinfoptr)imagehandle;

	if (imagetype[ptr->format].write && !ptr->writeprotected)
	{
		imagetype[ptr->format].write(ptr, track, quartertrack, trackimage, nibbles);
		ptr->validtrack[track] = 1;
		ptr->Mudado = 1;
	}
}

// old Zip code:
/*
		ZIP   *ArqZip;
		struct zipent *Entrada;
		char  NomeArq[MAX_PATH];

		ArqZip = openzip(imagefilename);
		if (!ArqZip)
			return 1;
		Entrada = readzip(ArqZip);
		if (!Entrada)
		{
			closezip(ArqZip);
			return 1;
		}
		strcpy(NomeArq, imagedir);
		strcat(NomeArq, Entrada->name);
		size = Entrada->uncompressed_size;
		BufferZip = (char *)malloc(size);
		if (!BufferZip)
		{
			closezip(ArqZip);
			return 1;
		}

		if (readuncompresszip(ArqZip, Entrada, BufferZip))
		{
			closezip(ArqZip);
			return 1;
		}
		closezip(ArqZip);
*/

// EOF