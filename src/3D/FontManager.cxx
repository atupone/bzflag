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

// BZFlag common header
#include "common.h"

// Interface header
#include "FontManager.h"

// System headers
#include <math.h>
#include <string>
#include <string.h>
#include <sstream>

// Global implementation headers
#include "bzfgl.h"
#include "bzfio.h"
#include "AnsiCodes.h"
#include "StateDatabase.h"
#include "BZDBCache.h"
#include "OpenGLGState.h"
#include "TimeKeeper.h"
#include "TextUtils.h"

// Local implementation headers
#include "ImageFont.h"
#include "TextureFont.h"

// ANSI code GLFloat equivalents - these should line up with the enums in AnsiCodes.h
static GLfloat BrightColors[9][3] =
{
    {1.0f,1.0f,0.0f}, // yellow
    {1.0f,0.0f,0.0f}, // red
    {0.0f,1.0f,0.0f}, // green
    {0.1f,0.2f,1.0f}, // blue
    {1.0f,0.0f,1.0f}, // purple
    {1.0f,1.0f,1.0f}, // white
    {0.5f,0.5f,0.5f}, // grey
    {1.0f,0.5f,0.0f}, // orange (nonstandard)
    {0.0f,1.0f,1.0f}  // cyan
};

GLfloat FontManager::underlineColor[4];
void FontManager::callback(const std::string &, void *)
{
    // set underline color
    const std::string uColor = BZDB.get("underlineColor");
    if (strcasecmp(uColor.c_str(), "text") == 0)
        underlineColor[0] = underlineColor[1] = underlineColor[2] = -1.0f;
    else if (strcasecmp(uColor.c_str(), "cyan") == 0)
        std::copy(BrightColors[CyanColor], BrightColors[CyanColor] + 3, underlineColor);
    else if (strcasecmp(uColor.c_str(), "grey") == 0)
        std::copy(BrightColors[GreyColor], BrightColors[GreyColor] + 3, underlineColor);
}

FontManager::FontManager() : Singleton<FontManager>(),
    opacity(1.0f),
    dimFactor(0.2f),
    darkness(1.0f)
{
    faceNames.clear();
    fontFaces.clear();
    BZDB.addCallback(std::string("underlineColor"), callback, NULL);
    BZDB.touch("underlineColor");
    OpenGLGState::registerContextInitializer(freeContext, initContext,
            (void*)this);
}

FontManager::~FontManager()
{
    clear();
    OpenGLGState::unregisterContextInitializer(freeContext, initContext,
            (void*)this);
    return;
}


void FontManager::freeContext(void* data)
{
    ((FontManager*)data)->clear();
    return;
}


void FontManager::initContext(void* data)
{
    ((FontManager*)data)->rebuild();
    return;
}


void FontManager::clear(void)   // clear all the lists
{
    // destroy all the fonts
    faceNames.clear();
    fontFaces.clear(); // This single call now destroys all maps AND all Fonts
}


void FontManager::rebuild(void) // rebuild all the lists
{
    clear();
    loadAll(fontDirectory);
}


void FontManager::loadAll(std::string directory)
{
    if (directory.size() == 0)
        return;

    // save this in case we have to rebuild
    fontDirectory = directory;

    OSFile file;

    OSDir dir(directory);

    while (dir.getNextFile(file, true))
    {
        std::string ext = file.getExtension();

        if (TextUtils::compare_nocase(ext, "fmt") == 0)
        {
            std::unique_ptr<ImageFont> pFont(new TextureFont);
            if (pFont)
            {
                if (pFont->load(file))
                {
                    std::string  str = TextUtils::toupper(pFont->getFaceName());

                    FontFaceMap::iterator faceItr = faceNames.find(str);

                    int faceID = 0;
                    if (faceItr == faceNames.end())
                    {
                        fontFaces.emplace_back();
                        faceID = (int)fontFaces.size() - 1;
                        faceNames[str] = faceID;
                    }
                    else
                        faceID = faceItr->second;

                    // Move ownership into the map
                    fontFaces[faceID][pFont->getSize()] = std::move(pFont);
                }
                else
                    logDebugMessage(4,"Font Texture load failed: %s\n", file.getOSName().c_str());
            }
        }
    }
}

int FontManager::getFaceID(std::string faceName)
{
    if (faceName.size() == 0)
        return -1;

    faceName = TextUtils::toupper(faceName);

    auto faceItr = faceNames.find(faceName);
    if (faceItr != faceNames.end())
        return faceItr->second;

    // Define fallbacks as a static array to avoid repeated string object creation
    static const char* fallbacks[] = {"DEFAULT", "ARIAL"};

    for (const char* fallback : fallbacks)
    {
        logDebugMessage(4,"Requested font %s not found, trying &s\n", faceName.c_str(), fallback);
        faceItr = faceNames.find(fallback);
        if (faceItr != faceNames.end())
            return faceItr->second;
    }

    // hell we are outta luck, you just get the first one
    logDebugMessage(4,"Requested font %s not found, trying first-loaded\n", faceName.c_str());
    faceItr = faceNames.begin();
    if (faceItr == faceNames.end())
    {
        logDebugMessage(2,"No fonts loaded\n");
        return -1;    // we must have NO fonts, so you are screwed no matter what
    }

    return faceItr->second;
}

int FontManager::getNumFaces(void)
{
    return (int)fontFaces.size();
}

const char* FontManager::getFaceName(int faceID)
{
    if ((faceID < 0) || (faceID > getNumFaces()))
    {
        logDebugMessage(2,"Trying to fetch name for invalid Font Face ID %d\n", faceID);
        return NULL;
    }

    return fontFaces[faceID].begin()->second->getFaceName();
}

void FontManager::drawString(float x, float y, float z, int faceID, float size,
                             const std::string &text, const float* resetColor)
{
    if (text.empty())
        return;

    if ((faceID < 0) || (faceID >= getNumFaces()))
    {
        logDebugMessage(2,"Trying to draw with invalid Font Face ID %d\n", faceID);
        return;
    }

    ImageFont* pFont = getClosestRealSize(faceID, size, size);

    if (!pFont)
    {
        logDebugMessage(2,"Could not find applicable font size for rendering; font face ID %d, "
                        "requested size %f\n", faceID, size);
        return;
    }

    // Use pre-calculated inverse size to replace division with multiplication
    float scale = size * (float)pFont->getInvSize();

    // filtering is off by default for fonts.
    // if the font is large enough, and the scaling factor
    // is not an integer, then request filtering
    bool filtering = false;
    if ((size > 12.0f) && (fabsf(scale - floorf(scale + 0.5f)) > 0.001f))
    {
        pFont->filter(true);
        filtering = true;
    }
    else
    {
        // no filtering - clamp to aligned coordinates
        x = floorf(x);
        y = floorf(y);
        z = floorf(z);
    }


    /*
     * Colorize text based on ANSI codes embedded in it
     * Break the text every time an ANSI code
     * is encountered and do a separate pFont->drawString code for
     * each segment, with the appropriate color parameter
     */

    // sane defaults
    bool bright = true;
    bool pulsating = false;
    bool underline = false;
    // negatives are invalid, we use them to signal "no change"
    GLfloat color[4] = {-1.0f, -1.0f, -1.0f, opacity};
    if (resetColor != NULL)
    {
        color[0] = resetColor[0] * darkness;
        color[1] = resetColor[1] * darkness;
        color[2] = resetColor[2] * darkness;
    }
    else
        resetColor = BrightColors[WhiteColor];

    const float darkDim = dimFactor * darkness;

    // underline color changes for bright == false
    GLfloat dimUnderlineColor[4] = { underlineColor[0] * darkDim,
                                     underlineColor[1] * darkDim,
                                     underlineColor[2] * darkDim,
                                     opacity
                                   };
    underlineColor[3] = opacity;

    /*
     * ANSI code interpretation is somewhat limited, we only accept values
     * which have been defined in AnsiCodes.h
     */
    int startSend = 0;
    int endSend = (int)text.length();
    bool tookCareOfANSICode = false;
    float width = 0;
    const char* tmpText = text.c_str();

    GLboolean depthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    glDepthMask(0);

    // split string into parts based on the embedded ANSI codes, render each separately
    // there has got to be a faster way to do this
    while (startSend < endSend)
    {
        // pulsate the text, if desired
        if (pulsating)
            getPulseColor(color, color);
        size_t nextEsc = text.find("\033[", startSend);

        // render text
        int len = (int)(nextEsc == std::string::npos) ? (endSend - startSend) : (nextEsc - startSend);
        if (len > 0)
        {
            // get substr width, we may need it a couple times
            width = pFont->getStrLength(scale, &tmpText[startSend], len);
            glPushMatrix();
            glTranslatef(x, y, z);
            pFont->drawString(scale, color, &tmpText[startSend], len);
            if (underline)
            {
                glDisable(GL_TEXTURE_2D);
                glEnable(GL_BLEND);
                if (bright && underlineColor[0] >= 0)
                    glColor4fv(underlineColor);
                else if (underlineColor[0] >= 0)
                    glColor4fv(dimUnderlineColor);
                else if (color[0] >= 0)
                    glColor4fv(color);
                // still have a translated matrix, these coordinates are
                // with respect to the string just drawn
                glBegin(GL_LINES);
                glVertex2f(0.0f, 0.0f);
                glVertex2f(width, 0.0f);
                glEnd();
                glEnable(GL_TEXTURE_2D);
            }
            glPopMatrix();
            // x transform for next substr
            x += width;
        }
        startSend += len;
        if (startSend >= endSend)
            break;
        // we stopped sending text at an ANSI code, find out what it is
        // and do something about it
        size_t mPos = text.find('m', startSend);
        if (mPos != std::string::npos)
        {
            tookCareOfANSICode = false;
            size_t codeLen = (mPos - startSend) + 1;

            // colors
            for (int i = 0; i <= LastColor; i++)
            {
                if (text.compare(startSend, codeLen, ColorStrings[i]) == 0)
                {
                    if (bright)
                    {
                        color[0] = BrightColors[i][0] * darkness;
                        color[1] = BrightColors[i][1] * darkness;
                        color[2] = BrightColors[i][2] * darkness;
                    }
                    else
                    {
                        color[0] = BrightColors[i][0] * darkDim;
                        color[1] = BrightColors[i][1] * darkDim;
                        color[2] = BrightColors[i][2] * darkDim;
                    }
                    tookCareOfANSICode = true;
                    break;
                }
            }
            // didn't find a matching color
            if (!tookCareOfANSICode)
            {
                // settings other than a hardcoded color
                if (text.compare(startSend, codeLen, ANSI_STR_RESET) == 0)
                {
                    bright = true;
                    pulsating = false;
                    underline = false;
                    color[0] = resetColor[0] * darkness;
                    color[1] = resetColor[1] * darkness;
                    color[2] = resetColor[2] * darkness;
                }
                else if (text.compare(startSend, codeLen, ANSI_STR_RESET_FINAL) == 0)
                {
                    bright = false;
                    pulsating = false;
                    underline = false;
                    color[0] = resetColor[0] * darkDim;
                    color[1] = resetColor[1] * darkDim;
                    color[2] = resetColor[2] * darkDim;
                }
                else if (text.compare(startSend, codeLen, ANSI_STR_BRIGHT) == 0)
                    bright = true;
                else if (text.compare(startSend, codeLen, ANSI_STR_DIM) == 0)
                    bright = false;
                else if (text.compare(startSend, codeLen, ANSI_STR_UNDERLINE) == 0)
                    underline = true;
                else if (text.compare(startSend, codeLen, ANSI_STR_PULSATING) == 0)
                    pulsating = true;
                else if (text.compare(startSend, codeLen, ANSI_STR_NO_UNDERLINE) == 0)
                    underline = false;
                else if (text.compare(startSend, codeLen, ANSI_STR_NO_PULSATE) == 0)
                    pulsating = false;
                else if (codeLen > 7 && text.compare(startSend, strlen(ANSI_STR_FG_RGB), ANSI_STR_FG_RGB) == 0)
                {
                    // 24-bit foreground RGB (ISO-8613-3)
                    // format: \033[38;2;<r>;<g>;<b>m
                    const char* p = &tmpText[startSend + 5];

                    // Verify the '2;' prefix exists
                    if (p[0] == '2' && p[1] == ';')
                    {
                        p += 2; // Move to <r>

                        // Helper to safely parse and advance
                        auto safeParse = [&](int &val) -> bool
                        {
                            if (!*p || *p == 'm') return false; // Early termination
                            val = atoi(p);
                            while (*p && isdigit(*p)) p++;      // Skip digits
                            if (*p == ';')
                            {
                                p++;    // Skip delimiter
                                return true;
                            }
                            return (*p == 'm');                 // Final value check
                        };

                        int r = 0, g = 0, b = 0;
                        if (safeParse(r) && safeParse(g) && safeParse(b))
                        {
                            // Clamp values to valid 0-255 range
                            r = std::max(0, std::min(255, r));
                            g = std::max(0, std::min(255, g));
                            b = std::max(0, std::min(255, b));
                        }

                        float factor = (bright ? darkness : darkDim) / 255.0f;
                        color[0] = r * factor;
                        color[1] = g * factor;
                        color[2] = b * factor;
                    }
                }
                else
                    logDebugMessage(2,"ANSI Code %s not supported\n", tmpText);
            }
            startSend = mPos + 1;
        }
        else
            startSend += 2; // skip broken esc
    }

    glDepthMask(depthMask);

    // revert the filtering state
    if (filtering)
        pFont->filter(false);

    return;
}

void FontManager::drawString(float x, float y, float z,
                             const std::string &face, float size,
                             const std::string &text,
                             const float* resetColor)
{
    drawString(x, y, z, getFaceID(face), size, text, resetColor);
}

float FontManager::getStrLength(int faceID, float size, const std::string &text,
                                bool alreadyStripped)
{
    if (text.empty())
        return 0.0f;

    if ((faceID < 0) || (faceID > getNumFaces()))
    {
        logDebugMessage(2,"Trying to find length of string for invalid Font Face ID %d\n", faceID);
        return 0.0f;
    }

    ImageFont* pFont = getClosestRealSize(faceID, size, size);

    if (!pFont)
    {
        logDebugMessage(2,"Could not find applicable font size for sizing; font face ID %d, "
                        "requested size %f\n", faceID, size);
        return 0.0f;
    }

    float scale = size * (float)pFont->getInvSize();

    // don't include ansi codes in the length, but allow outside funcs to skip this step
    if (alreadyStripped)
        return pFont->getStrLength(scale, text);

    // Zero-allocation length calculation
    float strippedLength = 0.0f;
    const char* rawPtr = text.c_str();
    const size_t totalLen = text.length();
    size_t cursor = 0;

    while (cursor < totalLen)
    {
        // Find the next ANSI escape sequence
        size_t nextEsc = text.find("\033[", cursor);
        size_t segmentLen = (nextEsc == std::string::npos) ? (totalLen - cursor) : (nextEsc - cursor);

        // Add the width of the printable segment
        if (segmentLen > 0)
        {
            strippedLength += pFont->getStrLength(scale, &rawPtr[cursor], (int)segmentLen);
            cursor += segmentLen;
        }

        // Skip the ANSI code entirely
        if (cursor < totalLen)
        {
            size_t mPos = text.find('m', cursor);
            if (mPos != std::string::npos)
                cursor = mPos + 1; // Advance past the 'm'
            else
                cursor += 2; // Broken escape, skip the "\033["
        }
    }
    return strippedLength;
}

float FontManager::getStrLength(const std::string &face, float size,
                                const std::string &text, bool alreadyStripped)
{
    return getStrLength(getFaceID(face), size, text, alreadyStripped);
}

float FontManager::getStrHeight(int faceID, float size,
                                const std::string & UNUSED(text))
{
    // don't scale tiny fonts
    getClosestRealSize(faceID, size, size);

    return (size * 1.5f);
}

float FontManager::getStrHeight(std::string face, float size,
                                const std::string &text)
{
    return getStrHeight(getFaceID(face), size, text);
}

void FontManager::unloadAll(void)
{
    for (auto& fontSizeMap : fontFaces)
    {
        for (const auto& pair : fontSizeMap)
            pair.second->free();
    }
}

ImageFont* FontManager::getClosestSize(int faceID, float size, bool bigger)
{
    const FontSizeMap &sizes = fontFaces[faceID];

    if (sizes.empty())
        return nullptr;

    const int rsize = int(size + 0.5f);

    FontSizeMap::const_iterator itr = sizes.lower_bound(rsize);
    if (bigger)
    {
        if (itr == sizes.end())
            --itr;
    }
    else
    {
        if (itr != sizes.begin() && (itr == sizes.end() || itr->first != rsize))
            --itr;
    }

    return itr->second.get();
}

ImageFont*    FontManager::getClosestRealSize(int faceID, float desiredSize, float &actualSize)
{
    /*
     * tiny fonts scale poorly, this function will return the nearest unscaled size of a font
     * if the font is too tiny to scale, and a scaled size if it's big enough.
     */

    ImageFont* font = getClosestSize(faceID, desiredSize, true);
    if (desiredSize < 14.0f)
    {
        // get the next biggest font size from requested
        if (!font)
        {
            logDebugMessage(2,"Could not find applicable font size for sizing; font face ID %d, "
                            "requested size %f\n", faceID, desiredSize);
            return NULL;
        }
        actualSize = (float)font->getSize();
    }
    else
        actualSize = desiredSize;
    return font;
}

void        FontManager::getPulseColor(const GLfloat *color, GLfloat *pulseColor) const
{
    float pulseTime = (float)TimeKeeper::getCurrent().getSeconds();

    // depth is how dark it should get (1.0 is to black)
    const float pulseDepth = BZDBCache::pulseDepth;
    // rate is how fast it should pulsate (smaller is faster)
    const float pulseRate = BZDBCache::pulseRate;

    // Replace fmodf with a linear progress calculation
    float progress = (pulseTime / pulseRate);
    progress -= (int)progress; // Faster than fmodf for positive time values

    // Convert 0->1 ramp into 0->1->0 pulse (Triangle Wave)
    float pulseFactor = (progress > 0.5f) ? (2.0f - 2.0f * progress) : (2.0f * progress);

    // Apply depth
    pulseFactor = pulseDepth * pulseFactor + (1.0f - pulseDepth);

    pulseColor[0] = color[0] * pulseFactor;
    pulseColor[1] = color[1] * pulseFactor;
    pulseColor[2] = color[2] * pulseFactor;
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
