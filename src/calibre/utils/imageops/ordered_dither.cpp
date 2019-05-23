/*
 *  Copyright 1999-2019 ImageMagick Studio LLC
 *
 *  Licensed under the ImageMagick License (the "License"); you may not use
 *  this file except in compliance with the License.  You may obtain a copy
 *  of the License at
 *
 *    https://imagemagick.org/script/license.php

 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 *  License for the specific language governing permissions and limitations
 *  under the License.
 */

#include "imageops.h"

// Just in case, as I don't want to deal with MSVC madness...
#if defined _MSC_VER && _MSC_VER < 1700
typedef unsigned __int8 uint8_t;
#define UINT8_MAX _UI8_MAX
typedef unsigned __int32 uint32_t;
#else
#include <cstdint>
#endif

// Quantize an 8-bit color value down to a palette of 16 evenly spaced colors, using an ordered 8x8 dithering pattern.
// With a grayscale input, this happens to match the eInk palette perfectly ;).
// If the input is not grayscale, and the output fb is not grayscale either,
// this usually still happens to match the eInk palette after the EPDC's own quantization pass.
// c.f., https://en.wikipedia.org/wiki/Ordered_dithering
// & https://github.com/ImageMagick/ImageMagick/blob/ecfeac404e75f304004f0566557848c53030bad6/MagickCore/threshold.c#L1627
// NOTE: As the references imply, this is straight from ImageMagick,
//       with only minor simplifications to enforce Q8 & avoid fp maths.
static uint8_t
    dither_o8x8(int x, int y, uint8_t v)
{
	// c.f., https://github.com/ImageMagick/ImageMagick/blob/ecfeac404e75f304004f0566557848c53030bad6/config/thresholds.xml#L107
	static const uint8_t threshold_map_o8x8[] = { 1,  49, 13, 61, 4,  52, 16, 64, 33, 17, 45, 29, 36, 20, 48, 32,
						      9,  57, 5,  53, 12, 60, 8,  56, 41, 25, 37, 21, 44, 28, 40, 24,
						      3,  51, 15, 63, 2,  50, 14, 62, 35, 19, 47, 31, 34, 18, 46, 30,
						      11, 59, 7,  55, 10, 58, 6,  54, 43, 27, 39, 23, 42, 26, 38, 22 };

	// Constants:
	// Quantum = 8; Levels = 16; map Divisor = 65
	// QuantumRange = 0xFF
	// QuantumScale = 1.0 / QuantumRange
	//
	// threshold = QuantumScale * v * ((L-1) * (D-1) + 1)
	// NOTE: The initial computation of t (specifically, what we pass to DIV255) would overflow an uint8_t.
	//       With a Q8 input value, we're at no risk of ever underflowing, so, keep to unsigned maths.
	//       Technically, an uint16_t would be wide enough, but it gains us nothing,
	//       and requires a few explicit casts to make GCC happy ;).
	uint32_t t = DIV255(v * ((15U << 6) + 1U));
	// level = t / (D-1);
	uint32_t l = (t >> 6);
	// t -= l * (D-1);
	t = (t - (l << 6));

	// map width & height = 8
	// c = ClampToQuantum((l+(t >= map[(x % mw) + mw * (y % mh)])) * QuantumRange / (L-1));
	uint32_t q = ((l + (t >= threshold_map_o8x8[(x & 7U) + 8U * (y & 7U)])) * 17);
	// NOTE: We're doing unsigned maths, so, clamping is basically MIN(q, UINT8_MAX) ;).
	//       The only overflow we should ever catch should be for a few black (v = 0xFF) input pixels
	//       that get shifted to the next step (i.e., q = 272 (0xFF + 17)).
	return (q > UINT8_MAX ? UINT8_MAX : reinterpret_cast<uint8_t>(q);
}

QImage ordered_dither(const QImage &image) { // {{{
    ScopedGILRelease PyGILRelease;
    QImage img = image;
    QRgb *row = NULL, *pixel = NULL;
    int y = 0, x = 0, width = img.width(), height = img.height();
    uint8_t gray = 0, dithered = 0;

    // We're running behind blend_image, so, we should only ever be fed RGB32 as input...
    if (img.format() != QImage::Format_RGB32) {
        img = img.convertToFormat(QImage::Format_RGB32);
        if (img.isNull()) throw std::bad_alloc();
    }

    for (y = 0; y < height; y++) {
        row = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (x = 0, pixel = row; x < width; x++, pixel++) {
            // We're running behind grayscale_image, so R = G = B
            gray = qRed(*pixel);
            dithered = dither_o8x8(x, y, gray);
            *pixel = qRgb(dithered, dithered, dithered);
        }
    }
    return img;
} // }}}
