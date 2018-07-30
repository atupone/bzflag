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
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string.h>

// Common implementation headers
#include "bzfgl.h"
#include "bzfio.h"
#include "OpenGLGState.h"

// Local implementation headers
#include "TextureManager.h"

TextureFont::TextureFont()
{
    textureID = -1;
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

        float deltaX = (float)fontMetrics[i].initialDist;
        float fFontY = (float)(fontMetrics[i].endY - fontMetrics[i].startY);
        float fFontX = (float)(fontMetrics[i].endX - fontMetrics[i].startX);
        float startX = (float)fontMetrics[i].startX / (float)textureXSize;
        float endX   = (float)fontMetrics[i].endX / (float)textureXSize;
        float startY = 1.0f - (float)fontMetrics[i].startY / (float)textureYSize;
        float endY   = 1.0f - (float)fontMetrics[i].endY / (float)textureYSize;

        fontTextur[0] = glm::vec2(startX, startY);
        fontTextur[1] = glm::vec2(startX, endY);
        fontTextur[2] = glm::vec2(endX,   startY);
        fontTextur[3] = glm::vec2(endX,   endY);

        fontVertex[0] = glm::vec3(deltaX,          fFontY, 0.0f);
        fontVertex[1] = glm::vec3(deltaX,          0.0f,   0.0f);
        fontVertex[2] = glm::vec3(deltaX + fFontX, fFontY, 0.0f);
        fontVertex[3] = glm::vec3(deltaX + fFontX, 0.0f,   0.0f);

        listIDs[i] = Vertex_Chunk(Vertex_Chunk::VT, 4);
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

void TextureFont::drawString(float scale, GLfloat color[4], const char *str,
                             int len)
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
        glColor4f(color[0], color[1], color[2], color[3]);

    glPushMatrix();
    glScalef(scale, scale, 1);

    glNormal3f(0.0f, 0.0f, 1.0f);
    int charToUse = 0;
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
    if (color[0] >= 0)
        glColor4f(1, 1, 1, 1);
    glPopMatrix();
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
