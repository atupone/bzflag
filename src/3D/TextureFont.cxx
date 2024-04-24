/* bzflag
 * Copyright (c) 1993-2023 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// Interface headers
#include "TextureFont.h"

// System headers
#include <string>
#include <string.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// Common implementation headers
#include "bzfio.h"
#include "OpenGLAPI.h"

// Local implementation headers
#include "TextureManager.h"

TextureFont::TextureFont()
{
    textureID = -1;
}

TextureFont::~TextureFont()
{
}

void TextureFont::build(void)
{
    preLoadLists();
}

void TextureFont::preLoadLists()
{
    if (texture.size() < 1)
    {
        logDebugMessage(2,"Font %s does not have an associated texture name, not loading\n", texture.c_str());
        return;
    }

    // load up the texture
    TextureManager &tm = TextureManager::instance();
    std::string textureAndDir = "fonts/" + texture;
    textureID = tm.getTextureID(textureAndDir.c_str());

    if (textureID == -1)
    {
        logDebugMessage(2,"Font texture %s has invalid ID\n", texture.c_str());
        return;
    }
    logDebugMessage(4,"Font %s (face %s) has texture ID %d\n", texture.c_str(), faceName.c_str(), textureID);

    // fonts are usually pixel aligned
    tm.setTextureFilter(textureID, OpenGLTexture::Nearest);

    for (int i = 0; i < numberOfCharacters; i++)
    {
        glm::vec2 fontTextur[4];
        glm::vec3 fontVertex[4];

        listIDs[i] = Vertex_Chunk(Vertex_Chunk::VT, 4);
        {
            const float initiX = (float)fontMetrics[i].initialDist;
            const float fFontY = (float)(fontMetrics[i].endY
                                         - fontMetrics[i].startY);
            const float fFontX = (float)(fontMetrics[i].endX
                                         - fontMetrics[i].startX);
            const float startX = (float)fontMetrics[i].startX
                                 / (float)textureXSize;
            const float endX   = (float)fontMetrics[i].endX
                                 / (float)textureXSize;
            const float startY = (float)fontMetrics[i].startY
                                 / (float)textureYSize;
            const float endY   = (float)fontMetrics[i].endY
                                 / (float)textureYSize;

            fontTextur[0] = glm::vec2(startX, 1.0f - startY);
            fontVertex[0] = glm::vec3(initiX, fFontY, 0.0f);

            fontTextur[1] = glm::vec2(startX, 1.0f - endY);
            fontVertex[1] = glm::vec3(initiX, 0.0f, 0.0f);

            fontTextur[2] = glm::vec2(endX, 1.0f - startY);
            fontVertex[2] = glm::vec3(initiX + fFontX, fFontY, 0.0f);

            fontTextur[3] = glm::vec2(endX, 1.0f - endY);
            fontVertex[3] = glm::vec3(initiX + fFontX, 0.0f, 0.0f);
        }
        listIDs[i].textureData(fontTextur);
        listIDs[i].vertexData(fontVertex);
    }

    // create GState
    OpenGLGStateBuilder builder(gstate);
    builder.setTexture(textureID);
    builder.setBlending();
    builder.setAlphaFunc();
    gstate = builder.getState();
}


void TextureFont::free(void)
{
    textureID = -1;
}

void TextureFont::filter(bool dofilter)
{
    TextureManager &tm = TextureManager::instance();
    if (textureID >= 0)
    {
        const OpenGLTexture::Filter type = dofilter ? OpenGLTexture::Max
                                           : OpenGLTexture::Nearest;
        tm.setTextureFilter(textureID, type);
    }
}

void TextureFont::drawString(float scale,
                             const glm::vec3 &color, float alpha,
                             const char *str, int len)
{
    if (!str)
        return;

    if (textureID == -1)
        preLoadLists();

    if (textureID == -1)
        return;

    gstate.setState();

    TextureManager &tm = TextureManager::instance();
    if (!tm.bind(textureID))
        return;

    if (color[0] >= 0)
        glColor(color, alpha);

    glPushMatrix();
    glScalef(scale, scale, 1);

    int charToUse = 0;
    glNormal3f(0.0f, 0.0f, 1.0f);
    for (int i = 0; i < len; i++)
    {
        const char space = ' '; // decimal 32
        if (str[i] < space)
            charToUse = space;
        else if (str[i] > (numberOfCharacters + space))
            charToUse = space;
        else
            charToUse = str[i];

        charToUse -= space;

        if (charToUse)
            listIDs[charToUse].draw(GL_TRIANGLE_STRIP);
        glTranslatef((float)(fontMetrics[charToUse].fullWidth), 0.0f, 0.0f);
    }
    glPopMatrix();
    if (color[0] >= 0)
        glColor4f(1, 1, 1, 1);
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
