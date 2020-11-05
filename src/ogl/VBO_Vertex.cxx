/* bzflag
 * Copyright (c) 1993-2018 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// interface header
#include "VBO_Vertex.h"

// System headers
#include <cstring>

bool VBO_Vertex::textureEnabled = false;
bool VBO_Vertex::normalEnabled  = false;
bool VBO_Vertex::colorEnabled   = false;
GLuint VBO_Vertex::actVerts     = 0;
GLuint VBO_Vertex::actNorms     = 0;
GLuint VBO_Vertex::actTxcds     = 0;
GLuint VBO_Vertex::actColrs     = 0;
GLuint VBO_Vertex::actIntrs     = 0;
GLuint VBO_Vertex::actBounded   = 0;

VBO_Vertex::VBO_Vertex(
    bool handleTexture_,
    bool handleNormal_,
    bool handleColor_,
    bool interlaced_) :
    VBO_Handler(),
    handleTexture(handleTexture_), handleNormal(handleNormal_),
    handleColor(handleColor_), verts(0), txcds(0), norms(0), colrs(0),
    vbosReady(false), interlaced(interlaced_)
{
    interlaced = false;
    interlacedSize = 3;
    strideTexture = interlacedSize;
    if (handleTexture)
        interlacedSize += 2;
    strideNormal = interlacedSize;
    if (handleNormal)
        interlacedSize += 3;
    strideColor = interlacedSize;
    if (handleColor)
        interlacedSize += 4;
}

void VBO_Vertex::resize()
{
    int newSize = vboSize < 256 ? 256 : 2 * vboSize;
    VBO_Handler::resize(newSize);
    if (!hostedVertices.empty())
        hostedVertices.resize(3 * vboSize);
    if (!hostedTextures.empty())
        hostedTextures.resize(2 * vboSize);
    if (!hostedNormals.empty())
        hostedNormals.resize(3 * vboSize);
    if (!hostedColors.empty())
        hostedColors.resize(4 * vboSize);
    if (!hostedInterlaced.empty())
        hostedInterlaced.resize(interlacedSize * vboSize);
    if (!vbosReady)
        return;
    destroy();
    init();
}

void VBO_Vertex::init()
{
    vbosReady = true;
    if (!vboSize)
        return;
    if (interlaced)
    {
        glGenBuffers(1, &intrs);
        bindBuffer(intrs);
        glBufferData(
            GL_ARRAY_BUFFER,
            vboSize * interlacedSize * sizeof(GLfloat),
            hostedInterlaced.data(),
            GL_DYNAMIC_DRAW);
    }
    else
    {
        glGenBuffers(1, &verts);
        if (handleTexture)
            glGenBuffers(1, &txcds);
        if (handleNormal)
            glGenBuffers(1, &norms);
        if (handleColor)
            glGenBuffers(1, &colrs);

        bindBuffer(verts);
        glBufferData(
            GL_ARRAY_BUFFER,
            vboSize * sizeof(GLfloat[3]),
            hostedVertices.data(),
            GL_DYNAMIC_DRAW);
        if (handleTexture)
        {
            bindBuffer(txcds);
            glBufferData(
                GL_ARRAY_BUFFER,
                vboSize * sizeof(GLfloat[2]),
                hostedTextures.data(),
                GL_DYNAMIC_DRAW);
        }
        if (handleNormal)
        {
            bindBuffer(norms);
            glBufferData(
                GL_ARRAY_BUFFER,
                vboSize * sizeof(GLfloat[3]),
                hostedNormals.data(),
                GL_DYNAMIC_DRAW);
        }
        if (handleColor)
        {
            bindBuffer(colrs);
            glBufferData(
                GL_ARRAY_BUFFER,
                vboSize * sizeof(GLfloat[4]),
                hostedColors.data(),
                GL_DYNAMIC_DRAW);
        }
    }
}

void VBO_Vertex::destroy()
{
    vbosReady = false;
    if (!vboSize)
        return;
    if (interlaced)
        deleteBuffer(intrs);
    else
    {
        deleteBuffer(verts);
        if (handleTexture)
            deleteBuffer(txcds);
        if (handleNormal)
            deleteBuffer(norms);
        if (handleColor)
            deleteBuffer(colrs);
    }
}

std::string VBO_Vertex::vboName()
{
    std::string name = "V";
    if (handleTexture)
        name += "T";
    if (handleNormal)
        name += "N";
    if (handleColor)
        name += "C";
    return name;
}

void VBO_Vertex::vertexData(int index, int size, const GLfloat vertices[])
{
    if (interlaced)
    {
        if (hostedInterlaced.empty())
            hostedInterlaced.resize(interlacedSize * vboSize);
        auto dataP = &hostedInterlaced[index * interlacedSize];
        auto vertP = vertices;
        for (int i = 0; i < size; i++)
        {
            memcpy(dataP, vertP, sizeof(GLfloat[3]));
            dataP += interlacedSize;
            vertP += 3;
        }
    }
    else
    {
        if (hostedVertices.empty())
            hostedVertices.resize(3 * vboSize);
        memcpy(&hostedVertices[index * 3],
               vertices,
               size * sizeof(GLfloat[3]));
    }
    if (!vbosReady)
        return;
    if (interlaced)
    {
        bindBuffer(intrs);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            index * interlacedSize * sizeof(GLfloat),
            size * interlacedSize * sizeof(GLfloat),
            &hostedInterlaced[index * interlacedSize]);
    }
    else
    {
        bindBuffer(verts);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            index * sizeof(GLfloat[3]),
            size * sizeof(GLfloat[3]),
            vertices);
    }
}

void VBO_Vertex::textureData(int index, int size, const GLfloat textures[])
{
    if (!handleTexture)
        return;
    if (interlaced)
    {
        if (hostedInterlaced.empty())
            hostedInterlaced.resize(interlacedSize * vboSize);
        auto dataP = &hostedInterlaced[index * interlacedSize];
        auto textP = textures;
        dataP += strideTexture;
        for (int i = 0; i < size; i++)
        {
            memcpy(dataP, textP, sizeof(GLfloat[2]));
            dataP += interlacedSize;
            textP += 2;
        }
    }
    else
    {
        if (hostedTextures.empty())
            hostedTextures.resize(2 * vboSize);
        memcpy(&hostedTextures[index * 2],
               textures,
               size * sizeof(GLfloat[2]));
    }
    if (!vbosReady)
        return;
    if (interlaced)
    {
        bindBuffer(intrs);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            index * interlacedSize * sizeof(GLfloat),
            size * interlacedSize * sizeof(GLfloat),
            &hostedInterlaced[index * interlacedSize]);
    }
    else
    {
        bindBuffer(txcds);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            index * sizeof(GLfloat[2]),
            size * sizeof(GLfloat[2]),
            textures);
    }
}

void VBO_Vertex::normalData(int index, int size, const GLfloat normals[])
{
    if (!handleNormal)
        return;
    if (interlaced)
    {
        if (hostedInterlaced.empty())
            hostedInterlaced.resize(interlacedSize * vboSize);
        auto dataP = &hostedInterlaced[index * interlacedSize];
        auto normP = normals;
        dataP += strideNormal;
        for (int i = 0; i < size; i++)
        {
            memcpy(dataP, normP, sizeof(GLfloat[3]));
            dataP += interlacedSize;
            normP += 3;
        }
    }
    else
    {
        if (hostedNormals.empty())
            hostedNormals.resize(3 * vboSize);
        memcpy(&hostedNormals[index * 3],
               normals,
               size * sizeof(GLfloat[3]));
    }
    if (!vbosReady)
        return;
    if (interlaced)
    {
        bindBuffer(intrs);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            index * interlacedSize * sizeof(GLfloat),
            size * interlacedSize * sizeof(GLfloat),
            &hostedInterlaced[index * interlacedSize]);
    }
    else
    {
        bindBuffer(norms);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            index * sizeof(GLfloat[3]),
            size * sizeof(GLfloat[3]),
            normals);
    }
}

void VBO_Vertex::colorData(int index, int size, const GLfloat colors[])
{
    if (!handleColor)
        return;
    if (interlaced)
    {
        if (hostedInterlaced.empty())
            hostedInterlaced.resize(interlacedSize * vboSize);
        auto dataP = &hostedInterlaced[index * interlacedSize];
        auto coloP = colors;
        dataP += strideColor;
        for (int i = 0; i < size; i++)
        {
            memcpy(dataP, coloP, sizeof(GLfloat[4]));
            dataP += interlacedSize;
            coloP += 4;
        }
    }
    else
    {
        if (hostedColors.empty())
            hostedColors.resize(4 * vboSize);
        memcpy(&hostedColors[index * 4],
               colors,
               size * sizeof(GLfloat[4]));
    }
    if (!vbosReady)
        return;
    if (interlaced)
    {
        bindBuffer(intrs);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            index * interlacedSize * sizeof(GLfloat),
            size * interlacedSize * sizeof(GLfloat),
            &hostedInterlaced[index * interlacedSize]);
    }
    else
    {
        bindBuffer(colrs);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            index * sizeof(GLfloat[4]),
            size * sizeof(GLfloat[4]),
            colors);
    }
}

void VBO_Vertex::enableArrays(bool texture, bool normal, bool color)
{
    enableVertex();
    if (texture && handleTexture)
        enableTextures();
    else
        disableTextures();
    if (normal && handleNormal)
        enableNormals();
    else
        disableNormals();
    if (color && handleColor)
        enableColors();
    else
        disableColors();
}

void VBO_Vertex::enableArrays()
{
    enableArrays(handleTexture, handleNormal, handleColor);
}

void VBO_Vertex::enableVertexOnly()
{
    enableVertex();
    disableTextures();
    disableNormals();
    disableColors();
}

void VBO_Vertex::enableVertex()
{
    if (interlaced)
    {
        if (actIntrs == intrs)
            return;
        actIntrs = intrs;
        bindBuffer(intrs);
        glVertexPointer(3, GL_FLOAT, interlacedSize * sizeof(GLfloat), 0);
        glTexCoordPointer(2, GL_FLOAT,
                          interlacedSize * sizeof(GLfloat),
                          (void *)(strideTexture * sizeof(GLfloat)));
        glNormalPointer(GL_FLOAT,
                        interlacedSize * sizeof(GLfloat),
                        (void *)(strideNormal * sizeof(GLfloat)));
        glColorPointer(4, GL_FLOAT,
                       interlacedSize * sizeof(GLfloat),
                       (void *)(strideColor * sizeof(GLfloat)));
    }
    else
    {
        if (actVerts == verts)
            return;
        actVerts = verts;
        bindBuffer(verts);
        glVertexPointer(3, GL_FLOAT, 0, 0);
    }
}

void VBO_Vertex::disableTextures()
{
    if (textureEnabled)
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    textureEnabled = false;
}

void VBO_Vertex::enableTextures()
{
    if (!textureEnabled)
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        textureEnabled = true;
    }
    if (actTxcds == txcds)
        return;
    actTxcds = txcds;
    if (!interlaced)
    {
        bindBuffer(txcds);
        glTexCoordPointer(2, GL_FLOAT, 0, 0);
    }
}

void VBO_Vertex::disableNormals()
{
    if (normalEnabled)
        glDisableClientState(GL_NORMAL_ARRAY);
    normalEnabled = false;
}

void VBO_Vertex::enableNormals()
{
    if (!normalEnabled)
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        normalEnabled = true;
    }
    if (actNorms == norms)
        return;
    actNorms = norms;
    if (!interlaced)
    {
        bindBuffer(norms);
        glNormalPointer(GL_FLOAT, 0, 0);
    }
}

void VBO_Vertex::disableColors()
{
    if (colorEnabled)
        glDisableClientState(GL_COLOR_ARRAY);
    colorEnabled = false;
}

void VBO_Vertex::enableColors()
{
    if (!colorEnabled)
    {
        glEnableClientState(GL_COLOR_ARRAY);
        colorEnabled = true;
    }
    if (actColrs == colrs)
        return;
    actColrs = colrs;
    if (!interlaced)
    {
        bindBuffer(colrs);
        glColorPointer(4, GL_FLOAT, 0, 0);
    }
}

void VBO_Vertex::bindBuffer(GLuint buffer)
{
    if (actBounded == buffer)
        return;
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    actBounded = buffer;
}

void VBO_Vertex::deleteBuffer(GLuint buffer)
{
    glDeleteBuffers(1, &buffer);
    if (actBounded == buffer)
        actBounded = 0;
    if (interlaced)
    {
        if (actIntrs == buffer)
            actIntrs = 0;
    }
    else
    {
        if (actVerts == buffer)
            actVerts = 0;
        if (actNorms == buffer)
            actNorms = 0;
        if (actTxcds == buffer)
            actTxcds = 0;
        if (actColrs == buffer)
            actColrs = 0;
    }
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
