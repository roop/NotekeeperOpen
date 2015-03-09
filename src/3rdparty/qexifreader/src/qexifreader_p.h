/*
  This file is part of QExifReader and is licensed under the MIT License

  Copyright (C) 2013 Roopesh Chander <roop@forwardbias.in>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject
  to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef QEXIFREADER_P_H
#define QEXIFREADER_P_H

#include <QString>

// JPEG format markers
#define JPEG_SOI 0xffd8  // First word of JPEG file
#define JPEG_SOS 0xffda  // Start of actual image data
#define JPEG_EOI 0xffd9  // Last word of JPEG file
#define JPEG_APP1 0xffe1 // Start of EXIF segment

// Some tags
#define EXIF_SUB_IFD_OFFSET_TAG 0x8769
#define JPEG_THUMBNAIL_OFFSET_TAG 0x0201
#define JPEG_THUMBNAIL_SIZE_BYTES_TAG 0x0202

int bytesPerComponentForDataFormat[] = { 0, 1, 1, 2, 4, 8, 1,
                                            1, 2, 4, 8, 4, 8 };

static QString stringForTag(quint16 tag)
{
    switch (tag) {
    case 0x010f: return "Make";
    case 0x0110: return "Model";
    case 0x0112: return "Orientation";
    case 0x0132: return "DateTime";
    default: return QString();
    }
}

#endif // QEXIFREADER_P_H
