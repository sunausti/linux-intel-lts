// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef BITMAP_H
#define BITMAP_H
#include <linux/bug.h>
#include <linux/ctype.h>
#include <linux/iosys-map.h>

typedef struct tagBITMAPFILEHEADER {
	uint8_t bfType[2];
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
}__attribute__((packed)) bmp_fileheader_t;

typedef struct tagBITMAPINFOHEADER {
	uint32_t biSize;
	uint32_t biWidth;
	uint32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXPelsPerMeter;
	uint32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} __attribute__((packed)) bmp_infoheader_t;

typedef struct bitmap {
	uint32_t width;
	uint32_t height;
	uint8_t * image_bytes;
	uint8_t * buf;
	uint32_t total_size;
	uint32_t bpp;
	void *fw;
}bitmap_t;

typedef struct palette {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
}palette_t;

void bitmap_create(bitmap_t *bitmap, const uint8_t *buf, int size);

int bitmap_load(struct drm_device *dev, bitmap_t* bitmap, const char *name);

void bitmap_to_dmabuf(bitmap_t *bmp, struct iosys_map *map, uint32_t pitches_0);

void bitmap_release(bitmap_t* bitmap);
#endif
