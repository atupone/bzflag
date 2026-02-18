/* bzflag
 * Copyright (c) 1993-2025 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// common header
#include "common.h"

// interface header
#include "BzPNG.h"

// system headers
#include <string>
#include <sstream>
#include <errno.h>
#include <string.h>
#include <memory>

// common headers
#include "Pack.h"
#include <zlib.h>


using namespace BzPNG;


//============================================================================//
//============================================================================//
//
// BzPNG
//

static void writeChunk(std::stringstream& ss, const char type[4],
                       const void* data, size_t len)
{
    uint32_t count;
    nboPackUInt(&count, len);
    ss.write((const char*) &count, sizeof(uint32_t));

    ss.write((const char*) type, 4);

    ss.write((const char*) data, len);

    uint32_t crc = crc32(0, NULL, 0);
    crc = crc32(crc, (const Bytef*)type, 4);
    crc = crc32(crc, (const Bytef*)data, len);

    uint32_t crcOut;
    nboPackUInt(&crcOut, crc);
    ss.write((const char*) &crcOut, sizeof(uint32_t));
}


std::string BzPNG::create(const std::vector<Chunk>& extraChunks,
                          size_t sx, size_t sy, size_t cc, const unsigned char* pixels)
{
    uint8_t colorType;
    switch (cc)
    {
    case 1:
    {
        colorType = 0;    // greyscale
        break;
    }
    case 2:
    {
        colorType = 4;    // greyscale + alpha
        break;
    }
    case 3:
    {
        colorType = 2;    // RGB
        break;
    }
    case 4:
    {
        colorType = 6;    // RGBA
        break;
    }
    default:
    {
        return "ERROR: bad channel count";
    }
    }

    std::stringstream ss;

    // the header
    const char header[] = "\211PNG\r\n\032\n";
    ss.write(header, strlen(header));

    // IHDR chunk
    char IHDR[13];
    void* p = IHDR;
    p = nboPackUInt(p, sx);
    p = nboPackUInt(p, sy);
    p = nboPackUByte(p,  8); // bit depth
    p = nboPackUByte(p,  colorType);
    p = nboPackUByte(p,  0); // compression
    p = nboPackUByte(p,  0); // filter
    p = nboPackUByte(p,  0); // interlace
    writeChunk(ss, "IHDR", IHDR, sizeof(IHDR));

    // Extra chunks
    for (const auto &chunk : extraChunks)
    {
        if (chunk.type.size() != 4)
            return "ERROR: bad chunk type size";

        writeChunk(ss, chunk.type.data(), chunk.data.data(),
                   chunk.data.size());
    }

    // IDAT chunk
    const uint32_t xbytes = (cc * sx);
    const uint32_t datLen = (sy * xbytes) + (sy);

    // Use unique_ptr for automatic memory management
    std::unique_ptr<Bytef[]> dat(new Bytef[datLen]);
    void* d = dat.get();

    for (uint32_t y = 0; y < sy; y++)
    {
        d = nboPackUByte(d, 0); // prepend the scanline filter type, 0 = none
        const uint32_t iy = (sy - y - 1); // flip the vertical
        d = nboPackString(d, pixels + (iy * xbytes), xbytes);
    }

    uLongf zLen = compressBound(datLen);
    std::unique_ptr<Bytef[]> zDat(new Bytef[zLen]);

    compress2(zDat.get(), &zLen, dat.get(), datLen, 9);
    writeChunk(ss, "IDAT", zDat.get(), zLen);

    // IEND chunk
    writeChunk(ss, "IEND", "", 0);

    return ss.str();
}


bool BzPNG::save(const std::string& filename,
                 const std::vector<Chunk>& extraChunks,
                 size_t sx, size_t sy, size_t cc, const unsigned char* pixels)
{
    const std::string pngData = create(extraChunks, sx, sy, cc, pixels);

    if ((pngData.size() < 5) || (pngData.substr(0, 5) == "ERROR"))
        return false;

    FILE* f = fopen(filename.c_str(), "wb");
    if (f == NULL)
    {
        printf("savePNG: failed for %s (%s)\n", filename.c_str(), strerror(errno));
        return false;
    }

    if (fwrite(pngData.data(), 1, pngData.size(), f) != pngData.size())
    {
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
