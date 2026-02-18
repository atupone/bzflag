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

#include "BzfMedia.h"
#include "TimeKeeper.h"
#include "MediaFile.h"
#include <string.h>
#include <string>
#include <stdio.h>

BzfMedia::BzfMedia() : mediaDir(DEFAULT_MEDIA_DIR) {}
BzfMedia::~BzfMedia() {}

double          BzfMedia::stopwatch(bool start)
{
    static TimeKeeper prev;
    if (start)
    {
        prev = TimeKeeper::getCurrent();
        return 0.0;
    }
    else
        return (double)(TimeKeeper::getCurrent() - prev);
}

std::string     BzfMedia::getMediaDirectory() const
{
    return mediaDir;
}

void            BzfMedia::setMediaDirectory(const std::string& _dir)
{
    mediaDir = _dir;
}

std::unique_ptr<unsigned char[]> BzfMedia::readImage(const std::string& filename,
        int& width, int& height, int& depth) const
{
    std::unique_ptr<unsigned char[]> image;

    // try mediaDir/filename
    std::string name = makePath(mediaDir, filename);
    image = doReadImage(name, width, height, depth);
    if (image) return image;

    // try filename as is
    image = doReadImage(filename, width, height, depth);
    if (image) return image;

#if defined(INSTALL_DATA_DIR)
    // try standard-mediaDir/filename
    name = makePath(INSTALL_DATA_DIR, filename);
    image = doReadImage(name, width, height, depth);
    if (image) return image;
#endif

    // try mediaDir/filename with replaced extension
    name = replaceExtension(makePath(mediaDir, filename), getImageExtension());
    image = doReadImage(name, width, height, depth);
    if (image) return image;

    // try filename with replaced extension
    name = replaceExtension(filename, getImageExtension());
    image = doReadImage(name, width, height, depth);
    if (image) return image;

#if defined(INSTALL_DATA_DIR)
    // try standard-mediaDir/filename with replaced extension
    name = makePath(INSTALL_DATA_DIR, filename);
    name = replaceExtension(name, getImageExtension());
    image = doReadImage(name, width, height, depth);
    if (image) return image;
#endif

    // try data/filename
    name = makePath(DEFAULT_MEDIA_DIR, filename);
    image = doReadImage(name, width, height, depth);
    if (image) return image;

    // try data/filename with replaced extension
    name = replaceExtension(makePath(DEFAULT_MEDIA_DIR, filename), getImageExtension());
    image = doReadImage(name, width, height, depth);
    if (image) return image;

    // try ../data/filename
    name = "../";
    name += DEFAULT_MEDIA_DIR;
    name = makePath(name, filename);
    image = doReadImage(name, width, height, depth);
    if (image) return image;

    // try ../data/filename with replaced extension
    name = "../";
    name += DEFAULT_MEDIA_DIR;
    name = replaceExtension(makePath(name, filename), getImageExtension());
    image = doReadImage(name, width, height, depth);
    if (image) return image;

    // try ../../data/filename
    name = "../../";
    name += DEFAULT_MEDIA_DIR;
    name = makePath(name, filename);
    image = doReadImage(name, width, height, depth);
    if (image) return image;

    // try ../../data/filename with replaced extension
    name = "../../";
    name += DEFAULT_MEDIA_DIR;
    name = replaceExtension(makePath(name, filename), getImageExtension());
    image = doReadImage(name, width, height, depth);
    if (image) return image;

#ifndef DEBUG
    std::cout << "Unable to locate [" << filename << "] image file (media dir is set to " << mediaDir << ")" << std::endl;
#endif

    return NULL;
}

std::unique_ptr<float[]> BzfMedia::readSound(const std::string& filename,
        int& numFrames, int& rate) const
{
    std::unique_ptr<float[]> sound;

    // try mediaDir/filename
    std::string name = makePath(mediaDir, filename);
    sound = doReadSound(name, numFrames, rate);
    if (sound) return sound;

    // try filename as is
    sound = doReadSound(filename, numFrames, rate);
    if (sound) return sound;

#if defined(INSTALL_DATA_DIR)
    // try standard-mediaDir/filename
    name = makePath(INSTALL_DATA_DIR, filename);
    sound = doReadSound(name, numFrames, rate);
    if (sound) return sound;
#endif

    // try mediaDir/filename with replaced extension
    name = replaceExtension(makePath(mediaDir, filename), getSoundExtension());
    sound = doReadSound(name, numFrames, rate);
    if (sound) return sound;

    // try filename with replaced extension
    name = replaceExtension(filename, getSoundExtension());
    sound = doReadSound(name, numFrames, rate);
    if (sound) return sound;

#if defined(INSTALL_DATA_DIR)
    // try mediaDir/filename with replaced extension
    name = makePath(INSTALL_DATA_DIR, filename);
    name = replaceExtension(name, getSoundExtension());
    sound = doReadSound(name, numFrames, rate);
    if (sound) return sound;
#endif

    // try mediaDir/filename
    name = makePath(DEFAULT_MEDIA_DIR, filename);
    sound = doReadSound(name, numFrames, rate);
    if (sound) return sound;

    // try mediaDir/filename with replaced extension
    name = replaceExtension(makePath(DEFAULT_MEDIA_DIR, filename), getSoundExtension());
    sound = doReadSound(name, numFrames, rate);
    if (sound) return sound;

    // try mediaDir/filename
    name = "../";
    name += DEFAULT_MEDIA_DIR;
    name = makePath(name, filename);
    sound = doReadSound(name, numFrames, rate);
    if (sound) return sound;

    // try mediaDir/filename with replaced extension
    name = "../";
    name += DEFAULT_MEDIA_DIR;
    name = replaceExtension(makePath(name, filename), getSoundExtension());
    sound = doReadSound(name, numFrames, rate);
    if (sound) return sound;

    // try mediaDir/filename
    name = "../../";
    name += DEFAULT_MEDIA_DIR;
    name = makePath(name, filename);
    sound = doReadSound(name, numFrames, rate);
    if (sound) return sound;

    // try mediaDir/filename with replaced extension
    name = "../../";
    name += DEFAULT_MEDIA_DIR;
    name = replaceExtension(makePath(name, filename), getSoundExtension());
    sound = doReadSound(name, numFrames, rate);
    if (sound) return sound;

#ifndef DEBUG
    std::cout << "Unable to locate [" << filename << "] audio file (media dir is set to " << mediaDir << ")" << std::endl;
#endif

    return NULL;
}

std::string     BzfMedia::makePath(const std::string& dir,
                                   const std::string& filename) const
{
    if ((dir.length() == 0) || filename[0] == '/') return filename;
    std::string path = dir;
    path += "/";
    path += filename;
    return path;
}

std::string     BzfMedia::replaceExtension(
    const std::string& pathname,
    const std::string& extension) const
{
    std::string newName;

    const int extPosition = findExtension(pathname);
    if (extPosition == 0)
        newName = pathname;
    else
        newName = pathname.substr(0, extPosition);

    newName += ".";
    newName += extension;
    return newName;
}

int         BzfMedia::findExtension(const std::string& pathname) const
{
    int dot = pathname.rfind(".");
    int slash = pathname.rfind("/");
    return ((slash > dot) ? 0 : dot);
}

std::string     BzfMedia::getImageExtension() const
{
    return std::string("png");
}

std::string     BzfMedia::getSoundExtension() const
{
    return std::string("wav");
}

std::unique_ptr<unsigned char[]> BzfMedia::doReadImage(const std::string& filename,
        int& dx, int& dy, int&) const
{
    return MediaFile::readImage( filename, &dx, &dy );
}

int16_t         BzfMedia::getShort(const void* ptr)
{
    const unsigned char* data = (const unsigned char*)ptr;
    return ((int16_t)data[0] << 8) + (int16_t)data[1];
}

uint16_t        BzfMedia::getUShort(const void* ptr)
{
    const unsigned char* data = (const unsigned char*)ptr;
    return ((uint16_t)data[0] << 8) + (uint16_t)data[1];
}

int32_t         BzfMedia::getLong(const void* ptr)
{
    const unsigned char* data = (const unsigned char*)ptr;
    return ((int32_t)data[0] << 24) + ((int32_t)data[1] << 16) +
           ((int32_t)data[2] << 8) + (int32_t)data[3];
}

bool            BzfMedia::doReadVerbatim(FILE* file,
        int dx, int dy, int dz,
        unsigned char* image)
{
    // zero image data
    memset(image, 0, 4 * dx * dy);

    // how many channels to read?
    if (dz > 4)
        dz = 4;

    // read each channel one after the other
    unsigned char row[4096];
    for (int k = 0; k < dz; ++k)
    {
        unsigned char* dst = image + k;
        for (int j = 0; j < dy; ++j)
        {
            // read raw data
            if (fread(row, dx, 1, file) != 1)
                return false;

            // reformat
            for (int i = 0; i < dx; ++i)
            {
                *dst = row[i];
                dst += 4;
            }
        }
    }
    return true;
}

bool            BzfMedia::doReadRLE(FILE* file,
                                    int dx, int dy, int dz,
                                    unsigned char* image)
{
    // zero image data
    memset(image, 0, 4 * dx * dy);

    // how many channels to read?
    if (dz > 4)
        dz = 4;

    // read offset tables
    const int tableSize = dy * dz;
    std::unique_ptr<uint32_t[]> startTable(new uint32_t[tableSize]);
    std::unique_ptr<uint32_t[]> lengthTable(new uint32_t[tableSize]);

    if (fread(startTable.get(), 4 * tableSize, 1, file) != 1)
        return false;

    if (fread(lengthTable.get(), 4 * tableSize, 1, file) != 1)
        return false;

    // convert offset tables to proper endianness
    for (int n = 0; n < tableSize; ++n)
    {
        startTable[n]  = getLong(startTable.get() + n);
        lengthTable[n] = getLong(lengthTable.get() + n);
    }

    // read each channel one after the other
    unsigned char row[4096];
    for (int k = 0; k < dz; ++k)
    {
        unsigned char* dst = image + k;
        for (int j = 0; j < dy; ++j)
        {
            // read raw data
            const int32_t length = lengthTable[j + k * dy];
            if (fseek(file, startTable[j + k * dy], SEEK_SET) < 0 ||
                    fread(row, length, 1, file) != 1)
                return false;

            // decode
            unsigned char* src = row;
            while (1)
            {
                // check for error in image
                if (src - row >= length)
                    return false;

                // get next code
                const unsigned char type = *src++;
                int count = (int)(type & 0x7f);

                // zero code means end of row
                if (count == 0)
                    break;

                if (type & 0x80)
                {
                    // copy count pixels
                    while (count--)
                    {
                        *dst = *src++;
                        dst += 4;
                    }
                }
                else
                {
                    // repeat pixel count times
                    const unsigned char pixel = *src++;
                    while (count--)
                    {
                        *dst = pixel;
                        dst += 4;
                    }
                }
            }
        }
    }

    return true;
}

std::unique_ptr<float[]> BzfMedia::doReadSound(const std::string&, int&, int&) const
{
    return NULL;
}

// Setting Audio Driver
void    BzfMedia::setDriver(std::string)
{
}

// Setting Audio Device
void    BzfMedia::setDevice(std::string)
{
}

void BzfMedia::audioDriver(std::string& driverName)
{
    driverName = "";
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
