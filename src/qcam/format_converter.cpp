/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * format_convert.cpp - qcam - Convert buffer to RGB
 */

#include <errno.h>

#include <linux/drm_fourcc.h>

#include <QImage>

#include "format_converter.h"

#define RGBSHIFT		8
#ifndef MAX
#define MAX(a,b)		((a)>(b)?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b)		((a)<(b)?(a):(b))
#endif
#ifndef CLAMP
#define CLAMP(a,low,high)	MAX((low),MIN((high),(a)))
#endif
#ifndef CLIP
#define CLIP(x)			CLAMP(x,0,255)
#endif

int FormatConverter::configure(unsigned int format, unsigned int width,
			       unsigned int height)
{
	switch (format) {
	case DRM_FORMAT_NV12:
		formatFamily_ = NV;
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		nvSwap_ = false;
		break;
	case DRM_FORMAT_NV21:
		formatFamily_ = NV;
		horzSubSample_ = 2;
		vertSubSample_ = 2;
		nvSwap_ = true;
		break;
	case DRM_FORMAT_NV16:
		formatFamily_ = NV;
		horzSubSample_ = 2;
		vertSubSample_ = 1;
		nvSwap_ = false;
		break;
	case DRM_FORMAT_NV61:
		formatFamily_ = NV;
		horzSubSample_ = 2;
		vertSubSample_ = 1;
		nvSwap_ = true;
		break;
	case DRM_FORMAT_NV24:
		formatFamily_ = NV;
		horzSubSample_ = 1;
		vertSubSample_ = 1;
		nvSwap_ = false;
		break;
	case DRM_FORMAT_NV42:
		formatFamily_ = NV;
		horzSubSample_ = 1;
		vertSubSample_ = 1;
		nvSwap_ = true;
		break;
	case DRM_FORMAT_RGB888:
		formatFamily_ = RGB;
		r_pos_ = 2;
		g_pos_ = 1;
		b_pos_ = 0;
		bpp_ = 3;
		break;
	case DRM_FORMAT_BGR888:
		formatFamily_ = RGB;
		r_pos_ = 0;
		g_pos_ = 1;
		b_pos_ = 2;
		bpp_ = 3;
		break;
	case DRM_FORMAT_BGRA8888:
		formatFamily_ = RGB;
		r_pos_ = 1;
		g_pos_ = 2;
		b_pos_ = 3;
		bpp_ = 4;
		break;
	case DRM_FORMAT_VYUY:
		formatFamily_ = YUV;
		y_pos_ = 1;
		cb_pos_ = 2;
		break;
	case DRM_FORMAT_YVYU:
		formatFamily_ = YUV;
		y_pos_ = 0;
		cb_pos_ = 3;
		break;
	case DRM_FORMAT_UYVY:
		formatFamily_ = YUV;
		y_pos_ = 1;
		cb_pos_ = 0;
		break;
	case DRM_FORMAT_YUYV:
		formatFamily_ = YUV;
		y_pos_ = 0;
		cb_pos_ = 1;
		break;
	case DRM_FORMAT_MJPEG:
		formatFamily_ = MJPEG;
		break;
	default:
		return -EINVAL;
	};

	format_ = format;
	width_ = width;
	height_ = height;

	return 0;
}

void FormatConverter::convert(const unsigned char *src, size_t size,
			      QImage *dst)
{
	switch (formatFamily_) {
	case MJPEG:
		dst->loadFromData(src, size, "JPEG");
		break;
	case YUV:
		convertYUV(src, dst->bits());
		break;
	case RGB:
		convertRGB(src, dst->bits());
		break;
	case NV:
		convertNV(src, dst->bits());
		break;
	};
}

static void yuv_to_rgb(int y, int u, int v, int *r, int *g, int *b)
{
	int c = y - 16;
	int d = u - 128;
	int e = v - 128;
	*r = CLIP(( 298 * c           + 409 * e + 128) >> RGBSHIFT);
	*g = CLIP(( 298 * c - 100 * d - 208 * e + 128) >> RGBSHIFT);
	*b = CLIP(( 298 * c + 516 * d           + 128) >> RGBSHIFT);
}

void FormatConverter::convertNV(const unsigned char *src, unsigned char *dst)
{
	unsigned int c_stride = width_ * (2 / horzSubSample_);
	unsigned int c_inc = horzSubSample_ == 1 ? 2 : 0;
	unsigned int cb_pos = nvSwap_ ? 1 : 0;
	unsigned int cr_pos = nvSwap_ ? 0 : 1;
	const unsigned char *src_c = src + width_ * height_;
	int r, g, b;

	for (unsigned int y = 0; y < height_; y++) {
		const unsigned char *src_y = src + y * width_;
		const unsigned char *src_cb = src_c + (y / vertSubSample_) *
					      c_stride + cb_pos;
		const unsigned char *src_cr = src_c + (y / vertSubSample_) *
					      c_stride + cr_pos;

		for (unsigned int x = 0; x < width_; x += 2) {
			yuv_to_rgb(*src_y, *src_cb, *src_cr, &r, &g, &b);
			dst[0] = b;
			dst[1] = g;
			dst[2] = r;
			dst[3] = 0xff;
			src_y++;
			src_cb += c_inc;
			src_cr += c_inc;
			dst += 4;

			yuv_to_rgb(*src_y, *src_cb, *src_cr, &r, &g, &b);
			dst[0] = b;
			dst[1] = g;
			dst[2] = r;
			dst[3] = 0xff;
			src_y++;
			src_cb += 2;
			src_cr += 2;
			dst += 4;
		}
	}
}

void FormatConverter::convertRGB(const unsigned char *src, unsigned char *dst)
{
	unsigned int x, y;
	int r, g, b;

	for (y = 0; y < height_; y++) {
		for (x = 0; x < width_; x++) {
			r = src[bpp_ * x + r_pos_];
			g = src[bpp_ * x + g_pos_];
			b = src[bpp_ * x + b_pos_];

			dst[4 * x + 0] = b;
			dst[4 * x + 1] = g;
			dst[4 * x + 2] = r;
			dst[4 * x + 3] = 0xff;
		}

		src += width_ * bpp_;
		dst += width_ * 4;
	}
}

void FormatConverter::convertYUV(const unsigned char *src, unsigned char *dst)
{
	unsigned int src_x, src_y, dst_x, dst_y;
	unsigned int src_stride;
	unsigned int dst_stride;
	unsigned int cr_pos;
	int r, g, b, y, cr, cb;

	cr_pos = (cb_pos_ + 2) % 4;
	src_stride = width_ * 2;
	dst_stride = width_ * 4;

	for (src_y = 0, dst_y = 0; dst_y < height_; src_y++, dst_y++) {
		for (src_x = 0, dst_x = 0; dst_x < width_; ) {
			cb = src[src_y * src_stride + src_x * 4 + cb_pos_];
			cr = src[src_y * src_stride + src_x * 4 + cr_pos];

			y = src[src_y * src_stride + src_x * 4 + y_pos_];
			yuv_to_rgb(y, cb, cr, &r, &g, &b);
			dst[dst_y * dst_stride + 4 * dst_x + 0] = b;
			dst[dst_y * dst_stride + 4 * dst_x + 1] = g;
			dst[dst_y * dst_stride + 4 * dst_x + 2] = r;
			dst[dst_y * dst_stride + 4 * dst_x + 3] = 0xff;
			dst_x++;

			y = src[src_y * src_stride + src_x * 4 + y_pos_ + 2];
			yuv_to_rgb(y, cb, cr, &r, &g, &b);
			dst[dst_y * dst_stride + 4 * dst_x + 0] = b;
			dst[dst_y * dst_stride + 4 * dst_x + 1] = g;
			dst[dst_y * dst_stride + 4 * dst_x + 2] = r;
			dst[dst_y * dst_stride + 4 * dst_x + 3] = 0xff;
			dst_x++;

			src_x++;
		}
	}
}
