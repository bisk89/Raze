/*
** bitmap.h
**
**---------------------------------------------------------------------------
** Copyright 2008 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/


#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <algorithm>
#include "palentry.h"
#include <string.h>
#include <stdlib.h>
#include "templates.h"

struct FCopyInfo;

struct FClipRect
{
	int x, y, width, height;

	bool Intersect(int ix, int iy, int iw, int ih);
};

typedef int blend_t;

enum
{
	BLENDBITS = 16,
	BLENDUNIT = (1<<BLENDBITS)
};

enum ColorType
{
	CF_RGB,
	CF_RGBT,
	CF_RGBA,
	CF_IA,
	CF_CMYK,
	CF_YCbCr,
	CF_BGR,
	CF_BGRA,
	CF_I16,
	CF_RGB555,
	CF_PalEntry
};


class FBitmap
{
protected:
	uint8_t *data;
	int Width;
	int Height;
	int Pitch;
	bool FreeBuffer;


public:
	FBitmap()
	{
		data = NULL;
		Width = Height = 0;
		Pitch = 0;
		FreeBuffer = false;
	}

	FBitmap(uint8_t *buffer, int pitch, int width, int height)
	{
		data = buffer;

		Pitch = pitch;
		Width = width;
		Height = height;
		FreeBuffer = false;
	}

	FBitmap(const FBitmap &other) = delete;	// disallow because in nearly all cases this creates an unwanted copy.

	FBitmap(FBitmap &&other)
	{
		data = other.data;
		Pitch = other.Pitch;
		Width = other.Width;
		Height = other.Height;
		FreeBuffer = other.FreeBuffer;
		other.data = nullptr;
		other.FreeBuffer = false;
	}

	FBitmap &operator=(const FBitmap &other) = delete;	// disallow because in nearly all cases this creates an unwanted copy. Use Copy instead.

	FBitmap &operator=(FBitmap &&other)
	{
		if (data != nullptr && FreeBuffer) delete[] data;
		data = other.data;
		Pitch = other.Pitch;
		Width = other.Width;
		Height = other.Height;
		FreeBuffer = other.FreeBuffer;
		other.data = nullptr;
		other.FreeBuffer = false;
		return *this;
	}

	void Copy(const FBitmap &other, bool deep = true)
	{
		if (data != nullptr && FreeBuffer) delete[] data;
		Pitch = other.Pitch;
		Width = other.Width;
		Height = other.Height;
		FreeBuffer = deep;
		if (deep)
		{
			data = new uint8_t[Pitch * Height];
			memcpy(data, other.data, Pitch * Height);
		}
		else
		{
			data = other.data;
		}
	}

	
	~FBitmap()
	{
		Destroy();
	}

	void Destroy()
	{
		if (data != NULL && FreeBuffer) delete [] data;
		data = NULL;
		FreeBuffer = false;
	}

	bool Create (int w, int h)
	{
		Pitch = w*4;
		Width = w;
		Height = h;
		data = new uint8_t[4*w*h];
		memset(data, 0, 4*w*h);
		FreeBuffer = true;
		return data != NULL;
	}

	int GetHeight() const
	{
		return Height;
	}

	int GetWidth() const
	{
		return Width;
	}

	int GetPitch() const
	{
		return Pitch;
	}

	const uint8_t *GetPixels() const
	{
		return data;
	}

	uint8_t *GetPixels()
	{
		return data;
	}


	void CopyPixelDataRGB(int originx, int originy, const uint8_t *patch, int srcwidth,
								int srcheight, int step_x, int step_y, int rotate, int ct,
		/* for PNG tRNS */		int r=0, int g=0, int b=0);
	void CopyPixelData(int originx, int originy, const uint8_t * patch, int srcwidth, int srcheight,
								int step_x, int step_y, int rotate, PalEntry * palette);


	void Blit(int originx, int originy, const FBitmap &src)
	{
		CopyPixelDataRGB(originx, originy, src.GetPixels(), src.GetWidth(), src.GetHeight(), 4, src.GetWidth()*4, 0, CF_BGRA);
	}

};

//===========================================================================
// 
// True color conversion classes for the different pixel formats
// used by the supported texture formats
//
//===========================================================================
struct cRGB
{
	static __forceinline unsigned char R(const unsigned char * p) { return p[0]; }
	static __forceinline unsigned char G(const unsigned char * p) { return p[1]; }
	static __forceinline unsigned char B(const unsigned char * p) { return p[2]; }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t x, uint8_t y, uint8_t z) { return 255; }
	static __forceinline int Gray(const unsigned char * p) { return (p[0]*77 + p[1]*143 + p[2]*36)>>8; }
};

struct cRGBT
{
	static __forceinline unsigned char R(const unsigned char * p) { return p[0]; }
	static __forceinline unsigned char G(const unsigned char * p) { return p[1]; }
	static __forceinline unsigned char B(const unsigned char * p) { return p[2]; }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t r, uint8_t g, uint8_t b) { return (p[0] != r || p[1] != g || p[2] != b) ? 255 : 0; }
	static __forceinline int Gray(const unsigned char * p) { return (p[0]*77 + p[1]*143 + p[2]*36)>>8; }
};

struct cRGBA
{
	enum
	{
		RED = 0,
		GREEN = 1,
		BLUE = 1,
		ALPHA = 3
	};
	static __forceinline unsigned char R(const unsigned char * p) { return p[0]; }
	static __forceinline unsigned char G(const unsigned char * p) { return p[1]; }
	static __forceinline unsigned char B(const unsigned char * p) { return p[2]; }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t x, uint8_t y, uint8_t z) { return p[3]; }
	static __forceinline int Gray(const unsigned char * p) { return (p[0]*77 + p[1]*143 + p[2]*36)>>8; }
};

struct cIA
{
	static __forceinline unsigned char R(const unsigned char * p) { return p[0]; }
	static __forceinline unsigned char G(const unsigned char * p) { return p[0]; }
	static __forceinline unsigned char B(const unsigned char * p) { return p[0]; }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t x, uint8_t y, uint8_t z) { return p[1]; }
	static __forceinline int Gray(const unsigned char * p) { return p[0]; }
};

struct cCMYK
{
	static __forceinline unsigned char R(const unsigned char * p) { return p[3] - (((256-p[0])*p[3]) >> 8); }
	static __forceinline unsigned char G(const unsigned char * p) { return p[3] - (((256-p[1])*p[3]) >> 8); }
	static __forceinline unsigned char B(const unsigned char * p) { return p[3] - (((256-p[2])*p[3]) >> 8); }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t x, uint8_t y, uint8_t z) { return 255; }
	static __forceinline int Gray(const unsigned char * p) { return (R(p)*77 + G(p)*143 + B(p)*36)>>8; }
};

struct cYCbCr
{
	static __forceinline unsigned char R(const unsigned char * p) { return clamp((int)(p[0] + 1.40200 * (int(p[2]) - 0x80)), 0, 255); }
	static __forceinline unsigned char G(const unsigned char * p) { return clamp((int)(p[0] - 0.34414 * (int(p[1] - 0x80)) - 0.71414 * (int(p[2]) - 0x80)), 0, 255); }
	static __forceinline unsigned char B(const unsigned char * p) { return clamp((int)(p[0] + 1.77200 * (int(p[1]) - 0x80)), 0, 255); }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t x, uint8_t y, uint8_t z) { return 255; }
	static __forceinline int Gray(const unsigned char * p) { return (R(p) * 77 + G(p) * 143 + B(p) * 36) >> 8; }
};

struct cBGR
{
	static __forceinline unsigned char R(const unsigned char * p) { return p[2]; }
	static __forceinline unsigned char G(const unsigned char * p) { return p[1]; }
	static __forceinline unsigned char B(const unsigned char * p) { return p[0]; }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t x, uint8_t y, uint8_t z) { return 255; }
	static __forceinline int Gray(const unsigned char * p) { return (p[2]*77 + p[1]*143 + p[0]*36)>>8; }
};

struct cBGRA
{
	enum
	{
		RED = 2,
		GREEN = 1,
		BLUE = 0,
		ALPHA = 3
	};
	static __forceinline unsigned char R(const unsigned char * p) { return p[2]; }
	static __forceinline unsigned char G(const unsigned char * p) { return p[1]; }
	static __forceinline unsigned char B(const unsigned char * p) { return p[0]; }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t x, uint8_t y, uint8_t z) { return p[3]; }
	static __forceinline int Gray(const unsigned char * p) { return (p[2]*77 + p[1]*143 + p[0]*36)>>8; }
};

struct cARGB
{
	enum
	{
		RED = 1,
		GREEN = 2,
		BLUE = 3,
		ALPHA = 0
	};
	static __forceinline unsigned char R(const unsigned char * p) { return p[1]; }
	static __forceinline unsigned char G(const unsigned char * p) { return p[2]; }
	static __forceinline unsigned char B(const unsigned char * p) { return p[3]; }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t x, uint8_t y, uint8_t z) { return p[0]; }
	static __forceinline int Gray(const unsigned char * p) { return (p[1]*77 + p[2]*143 + p[3]*36)>>8; }
};

struct cI16
{
	static __forceinline unsigned char R(const unsigned char * p) { return p[1]; }
	static __forceinline unsigned char G(const unsigned char * p) { return p[1]; }
	static __forceinline unsigned char B(const unsigned char * p) { return p[1]; }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t x, uint8_t y, uint8_t z) { return 255; }
	static __forceinline int Gray(const unsigned char * p) { return p[1]; }
};

struct cRGB555
{
	static __forceinline unsigned char R(const unsigned char * p) { return (((*(uint16_t*)p)&0x1f)<<3); }
	static __forceinline unsigned char G(const unsigned char * p) { return (((*(uint16_t*)p)&0x3e0)>>2); }
	static __forceinline unsigned char B(const unsigned char * p) { return (((*(uint16_t*)p)&0x7c00)>>7); }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t x, uint8_t y, uint8_t z) { return 255; }
	static __forceinline int Gray(const unsigned char * p) { return (R(p)*77 + G(p)*143 + B(p)*36)>>8; }
};

struct cPalEntry
{
	static __forceinline unsigned char R(const unsigned char * p) { return ((PalEntry*)p)->r; }
	static __forceinline unsigned char G(const unsigned char * p) { return ((PalEntry*)p)->g; }
	static __forceinline unsigned char B(const unsigned char * p) { return ((PalEntry*)p)->b; }
	static __forceinline unsigned char A(const unsigned char * p, uint8_t x, uint8_t y, uint8_t z) { return ((PalEntry*)p)->a; }
	static __forceinline int Gray(const unsigned char * p) { return (R(p)*77 + G(p)*143 + B(p)*36)>>8; }
};

struct bCopy
{
	static __forceinline void OpC(uint8_t &d, uint8_t s, uint8_t a) { d = s; }
	static __forceinline void OpA(uint8_t &d, uint8_t s) { d = s; }
	static __forceinline bool ProcessAlpha0() { return false; }
};


#endif