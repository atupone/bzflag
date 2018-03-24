/* bzflag
 * Copyright (c) 2019-2019 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// Interface header
#include "VBO_Drawing.h"

// System header
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

static int vertexForCylinder(int slices)
{
    return 2 * (slices + 1);
}

static int vertexForDisk(int slices)
{
    return slices + 2;
}

VBO_Drawing::VBO_Drawing()
    : simmetricSquareLoopVBOChunk(Vertex_Chunk::V, 4)
    , asimmetricSquareLoopVBOChunk(Vertex_Chunk::V, 4)
    , diamondLoopVBOChunk(Vertex_Chunk::V, 4)
    , asimmetricLineXVBOChunk(Vertex_Chunk::V, 2)
    , asimmetricLineYVBOChunk(Vertex_Chunk::V, 2)
    , simmetricLineXVBOChunk(Vertex_Chunk::V, 2)
    , simmetricLineYVBOChunk(Vertex_Chunk::V, 2)
    , asimmetricLineZVBOChunk(Vertex_Chunk::V, 2)
    , crossVBOChunk(Vertex_Chunk::V, 4)
    , northVBOChunk(Vertex_Chunk::V, 4)
    , simmetricRectVBOChunk(Vertex_Chunk::V, 4)
    , asimmetricRectVBOChunk(Vertex_Chunk::V, 4)
    , asimmetricRectXZVBOChunk(Vertex_Chunk::V, 4)
    , rectSXAZVBOChunk(Vertex_Chunk::V, 4)
    , cubeSXYAZVBOChunk(Vertex_Chunk::V, 10)
    , simmetricTexturedRectVBOChunk(Vertex_Chunk::VT, 4)
    , asimmetricTexturedRectVBOChunk(Vertex_Chunk::VT, 4)
    , asimmetricTexturedRectXZVBOChunk(Vertex_Chunk::VT, 4)
    , verticalTexturedRectVBOChunk(Vertex_Chunk::VT, 4)
    , isoscelesTriangleXYVBOChunk(Vertex_Chunk::V, 3)
    , shotLineVBOChunk(Vertex_Chunk::V, 4)
    , viewAngleVBOChunk(Vertex_Chunk::V, 3)
    , outlineCircle20VBOChunk(Vertex_Chunk::V, outlineSides)
    , cylinder8VBOChunk(Vertex_Chunk::VN, vertexForCylinder(8))
    , cylinder10VBOChunk(Vertex_Chunk::VN, vertexForCylinder(10))
    , cylinder16VBOChunk(Vertex_Chunk::VN, vertexForCylinder(16))
    , cylinder24VBOChunk(Vertex_Chunk::VN, vertexForCylinder(24))
    , cylinder32VBOChunk(Vertex_Chunk::VN, vertexForCylinder(32))
    , disk8VBOChunk(Vertex_Chunk::V, vertexForDisk(8))
{

    glm::vec3 vertex[20];
    glm::vec2 textur[4];

    vertex[0] = glm::vec3(-1.0f, -1.0f, 0.0f);
    vertex[1] = glm::vec3(+1.0f, -1.0f, 0.0f);
    vertex[2] = glm::vec3(+1.0f, +1.0f, 0.0f);
    vertex[3] = glm::vec3(-1.0f, +1.0f, 0.0f);
    simmetricSquareLoopVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    vertex[1] = glm::vec3(1.0f, 0.0f, 0.0f);
    vertex[2] = glm::vec3(1.0f, 1.0f, 0.0f);
    vertex[3] = glm::vec3(0.0f, 1.0f, 0.0f);
    asimmetricSquareLoopVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(-1.0f,  0.0f, 0.0f);
    vertex[1] = glm::vec3( 0.0f, -1.0f, 0.0f);
    vertex[2] = glm::vec3( 1.0f,  0.0f, 0.0f);
    vertex[3] = glm::vec3( 0.0f, +1.0f, 0.0f);
    diamondLoopVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    vertex[1] = glm::vec3(1.0f, 0.0f, 0.0f);
    asimmetricLineXVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    vertex[1] = glm::vec3(0.0f, 1.0f, 0.0f);
    asimmetricLineYVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(-1.0f, 0.0f, 0.0f);
    vertex[1] = glm::vec3( 1.0f, 0.0f, 0.0f);
    simmetricLineXVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(0.0f, -1.0f, 0.0f);
    vertex[1] = glm::vec3(0.0f,  1.0f, 0.0f);
    simmetricLineYVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    vertex[1] = glm::vec3(0.0f, 0.0f, 1.0f);
    asimmetricLineZVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(-1.0f,  0.0f, 0.0f);
    vertex[1] = glm::vec3( 1.0f,  0.0f, 0.0f);
    vertex[2] = glm::vec3( 0.0f, -1.0f, 0.0f);
    vertex[3] = glm::vec3( 0.0f,  1.0f, 0.0f);
    crossVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(-1.0f, -1.0f, 0.0f);
    vertex[1] = glm::vec3(-1.0f,  1.0f, 0.0f);
    vertex[2] = glm::vec3( 1.0f, -1.0f, 0.0f);
    vertex[3] = glm::vec3( 1.0f,  1.0f, 0.0f);
    northVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(-1.0f, -1.0f, 0.0f);
    vertex[1] = glm::vec3(+1.0f, -1.0f, 0.0f);
    vertex[2] = glm::vec3(-1.0f, +1.0f, 0.0f);
    vertex[3] = glm::vec3(+1.0f, +1.0f, 0.0f);
    simmetricRectVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    vertex[1] = glm::vec3(1.0f, 0.0f, 0.0f);
    vertex[2] = glm::vec3(0.0f, 1.0f, 0.0f);
    vertex[3] = glm::vec3(1.0f, 1.0f, 0.0f);
    asimmetricRectVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    vertex[1] = glm::vec3(1.0f, 0.0f, 0.0f);
    vertex[2] = glm::vec3(0.0f, 0.0f, 1.0f);
    vertex[3] = glm::vec3(1.0f, 0.0f, 1.0f);
    asimmetricRectXZVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(-1.0f, 0.0f, 0.0f);
    vertex[1] = glm::vec3( 1.0f, 0.0f, 0.0f);
    vertex[2] = glm::vec3(-1.0f, 0.0f, 1.0f);
    vertex[3] = glm::vec3( 1.0f, 0.0f, 1.0f);
    rectSXAZVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(-1.0f,  0.0f, 0.0f);
    vertex[1] = glm::vec3(-1.0f,  0.0f, 1.0f);
    vertex[2] = glm::vec3( 0.0f, -1.0f, 0.0f);
    vertex[3] = glm::vec3( 0.0f, -1.0f, 1.0f);
    vertex[4] = glm::vec3( 1.0f,  0.0f, 0.0f);
    vertex[5] = glm::vec3( 1.0f,  0.0f, 1.0f);
    vertex[6] = glm::vec3( 0.0f,  1.0f, 0.0f);
    vertex[7] = glm::vec3( 0.0f,  1.0f, 1.0f);
    vertex[8] = glm::vec3(-1.0f,  0.0f, 0.0f);
    vertex[9] = glm::vec3(-1.0f,  0.0f, 1.0f);
    cubeSXYAZVBOChunk.vertexData(vertex);

    textur[0] = glm::vec2(0.0f, 0.0f);
    textur[1] = glm::vec2(1.0f, 0.0f);
    textur[2] = glm::vec2(0.0f, 1.0f);
    textur[3] = glm::vec2(1.0f, 1.0f);

    vertex[0] = glm::vec3(-1.0f, -1.0f, 0.0f);
    vertex[1] = glm::vec3( 1.0f, -1.0f, 0.0f);
    vertex[2] = glm::vec3(-1.0f,  1.0f, 0.0f);
    vertex[3] = glm::vec3( 1.0f,  1.0f, 0.0f);

    simmetricTexturedRectVBOChunk.textureData(textur);
    simmetricTexturedRectVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    vertex[1] = glm::vec3(1.0f, 0.0f, 0.0f);
    vertex[2] = glm::vec3(0.0f, 1.0f, 0.0f);
    vertex[3] = glm::vec3(1.0f, 1.0f, 0.0f);

    asimmetricTexturedRectVBOChunk.textureData(textur);
    asimmetricTexturedRectVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    vertex[1] = glm::vec3(1.0f, 1.0f, 0.0f);
    vertex[2] = glm::vec3(0.0f, 2.0f, 0.0f);
    isoscelesTriangleXYVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(0.0f, 0.0f, 0.0f);
    vertex[1] = glm::vec3(1.0f, 0.0f, 0.0f);
    vertex[2] = glm::vec3(0.0f, 0.0f, 1.0f);
    vertex[3] = glm::vec3(1.0f, 0.0f, 1.0f);

    asimmetricTexturedRectXZVBOChunk.textureData(textur);
    asimmetricTexturedRectXZVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3( 0.0f,  0.0f, 0.0f);
    vertex[1] = glm::vec3( 1.0f,  1.0f, 0.0f);
    vertex[2] = glm::vec3( 0.0f,  0.0f, 0.0f);
    vertex[3] = glm::vec3(-1.0f, -1.0f, 0.0f);
    shotLineVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(-1.0f,  1.0f, 0.0f);
    vertex[1] = glm::vec3( 0.0f,  0.0f, 0.0f);
    vertex[2] = glm::vec3( 1.0f,  1.0f, 0.0f);
    viewAngleVBOChunk.vertexData(vertex);

    textur[0] = glm::vec2(0.0f, 1.0f);
    textur[1] = glm::vec2(0.0f, 0.0f);
    textur[2] = glm::vec2(1.0f, 1.0f);
    textur[3] = glm::vec2(1.0f, 0.0f);

    vertex[0] = glm::vec3(0.0f, 0.0f, +1.0f);
    vertex[1] = glm::vec3(0.0f, 1.0f, +1.0f);
    vertex[2] = glm::vec3(0.0f, 0.0f, -1.0f);
    vertex[3] = glm::vec3(0.0f, 1.0f, -1.0f);

    verticalTexturedRectVBOChunk.textureData(textur);
    verticalTexturedRectVBOChunk.vertexData(vertex);

    // draw circle of current radius
    for (int i = 0; i < outlineSides; i++)
    {
        const float angle = (float)(2.0 * M_PI * double(i) / double(outlineSides));
        vertex[i] = glm::vec3(cosf(angle), sinf(angle), 0.0f);
    }
    outlineCircle20VBOChunk.vertexData(vertex);

    buildSphere(4, sphere4VBOChunk);
    buildSphere(6, sphere6VBOChunk);
    buildSphere(8, sphere8VBOChunk);
    buildSphere(12, sphere12VBOChunk);
    buildSphere(16, sphere16VBOChunk);
    buildSphere(32, sphere32VBOChunk);
    buildCylinder(8, cylinder8VBOChunk);
    buildCylinder(10, cylinder10VBOChunk);
    buildCylinder(16, cylinder16VBOChunk);
    buildCylinder(24, cylinder24VBOChunk);
    buildCylinder(32, cylinder32VBOChunk);
    buildDisk(8, disk8VBOChunk);
}

void VBO_Drawing::buildSphere(int slices, Vertex_Chunk &vboChunk)
{
    const int maxSlices = 32;
    glm::vec3 vertex[2 * maxSlices * (maxSlices + 1)];
    glm::vec2 textur[2 * maxSlices * (maxSlices + 1)];
    glm::vec3 *pVertex = vertex;
    glm::vec2 *pTextur = textur;
    int i,j;
    float zHigh;
    float zLow = -1;
    float sintemp2;
    float sintemp1 = 0;
    float tHigh;
    float tLow = 0;

    for (j = slices - 1; j >= 0; j--)
    {
        zHigh    = zLow;
        sintemp2 = sintemp1;
        tHigh    = tLow;

        float percent = (float) j / slices;
        float angleZ  = (float)(M_PI * percent);
        zLow     = cos(angleZ);
        sintemp1 = sin(angleZ);
        tLow     = 1 - percent;

        *pTextur++ = glm::vec2(1, tHigh);
        *pVertex++ = glm::vec3(0.0f, sintemp2, zHigh);

        *pTextur++ = glm::vec2(1, tLow);
        *pVertex++ = glm::vec3(0.0f, sintemp1, zLow);

        for (i = 1; i < slices; i++)
        {
            float angleT = (float)(2 * M_PI * i / slices);
            float sinCache = sin(angleT);
            float cosCache = cos(angleT);
            float x = sinCache * sintemp2;
            float y = cosCache * sintemp2;
            float s = 1 - (float) i / slices;

            *pTextur++ = glm::vec2(s, tHigh);
            *pVertex++ = glm::vec3(x, y, zHigh);

            x = sinCache * sintemp1;
            y = cosCache * sintemp1;
            *pTextur++ = glm::vec2(s, tLow);
            *pVertex++ = glm::vec3(x, y, zLow);
        }

        *pTextur++ = glm::vec2(0, tHigh);
        *pVertex++ = glm::vec3(0.0f, sintemp2, zHigh);

        *pTextur++ = glm::vec2(0, tLow);
        *pVertex++ = glm::vec3(0.0f, sintemp2, zLow);
    }

    vboChunk = Vertex_Chunk(Vertex_Chunk::VTN, pVertex - vertex);
    vboChunk.normalData(vertex);
    vboChunk.vertexData(vertex);
    vboChunk.textureData(textur);
}

void VBO_Drawing::buildCylinder(int slices, Vertex_Chunk &vboChunk)
{
    const int maxSlices = 32;
    glm::vec3 vertex[2 * (maxSlices + 1)];
    glm::vec3 normal[2 * (maxSlices + 1)];
    glm::vec3 *pVertex = vertex;
    glm::vec3 *pNormal = normal;
    int   i;
    float angle;
    float sinCache;
    float cosCache;
    glm::vec2 currentVartex;

    currentVartex = glm::vec2(0.0f, 1.0f);
    *pNormal++ = glm::vec3(currentVartex, 0.0f);
    *pVertex++ = glm::vec3(currentVartex, 0.0f);
    *pNormal++ = glm::vec3(currentVartex, 0.0f);
    *pVertex++ = glm::vec3(currentVartex, 1.0f);
    for (i = 1; i < slices; i++)
    {
        angle = (float)(2 * M_PI * i / slices);
        sinCache  = sin(angle);
        cosCache  = cos(angle);
        currentVartex = glm::vec2(sinCache, cosCache);
        *pNormal++ = glm::vec3(currentVartex, 0.0f);
        *pVertex++ = glm::vec3(currentVartex, 0.0f);
        *pNormal++ = glm::vec3(currentVartex, 0.0f);
        *pVertex++ = glm::vec3(currentVartex, 1.0f);
    }
    currentVartex = glm::vec2(0.0f, 1.0f);
    *pNormal++ = glm::vec3(currentVartex, 0.0f);
    *pVertex++ = glm::vec3(currentVartex, 0.0f);
    *pNormal++ = glm::vec3(currentVartex, 0.0f);
    *pVertex++ = glm::vec3(currentVartex, 1.0f);
    vboChunk.normalData(normal);
    vboChunk.vertexData(vertex);
}

void VBO_Drawing::buildDisk(int slices, Vertex_Chunk &vboChunk)
{
    const int maxSlices = 8;
    glm::vec3 vertex[maxSlices + 2];
    glm::vec3 *pVertex = vertex;
    /* Triangle strip for inner polygons */
    *pVertex++ = glm::vec3(0.0f, 0.0f, 0.0f);
    *pVertex++ = glm::vec3(0.0f, 1.0f, 0.0f);
    for (int i = slices - 1; i > 0; i--)
    {
        float angle = (float)(2 * M_PI * i / slices);
        *pVertex++ = glm::vec3(sin(angle), cos(angle), 0.0f);
    }
    *pVertex++ = glm::vec3(0.0f, 1.0f, 0.0f);
    vboChunk.vertexData(vertex);
}

void VBO_Drawing::simmetricSquareLoop()
{
    simmetricSquareLoopVBOChunk.draw(GL_LINE_LOOP);
}

void VBO_Drawing::asimmetricSquareLoop()
{
    asimmetricSquareLoopVBOChunk.draw(GL_LINE_LOOP);
}

void VBO_Drawing::diamondLoop()
{
    diamondLoopVBOChunk.draw(GL_LINE_LOOP);
}

void VBO_Drawing::asimmetricLineX()
{
    asimmetricLineXVBOChunk.draw(GL_LINES);
}

void VBO_Drawing::asimmetricLineY()
{
    asimmetricLineYVBOChunk.draw(GL_LINES);
}

void VBO_Drawing::simmetricLineX()
{
    simmetricLineXVBOChunk.draw(GL_LINES);
}

void VBO_Drawing::simmetricLineY()
{
    simmetricLineYVBOChunk.draw(GL_LINES);
}

void VBO_Drawing::asimmetricLineZ()
{
    asimmetricLineZVBOChunk.draw(GL_LINES);
}

void VBO_Drawing::cross()
{
    crossVBOChunk.draw(GL_LINES);
}

void VBO_Drawing::north()
{
    northVBOChunk.draw(GL_LINE_STRIP);
}

void VBO_Drawing::simmetricRect()
{
    simmetricRectVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::asimmetricRect()
{
    asimmetricRectVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::asimmetricRectXZ()
{
    asimmetricRectXZVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::rectSXAZ()
{
    rectSXAZVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::cubeSXYAZ()
{
    cubeSXYAZVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::simmetricTexturedRect()
{
    simmetricTexturedRectVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::asimmetricTexturedRect()
{
    asimmetricTexturedRectVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::asimmetricTexturedRectXZ()
{
    asimmetricTexturedRectXZVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::verticalTexturedRect()
{
    verticalTexturedRectVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::isoscelesTriangleXYFilled()
{
    isoscelesTriangleXYVBOChunk.draw(GL_TRIANGLES);
}

void VBO_Drawing::isoscelesTriangleXYOutline()
{
    isoscelesTriangleXYVBOChunk.draw(GL_LINE_LOOP);
}

void VBO_Drawing::point()
{
    shotLineVBOChunk.enableArrays();
    shotLineVBOChunk.glDrawArrays(GL_POINTS, 1);
}

void VBO_Drawing::laggingLine()
{
    shotLineVBOChunk.enableArrays();
    shotLineVBOChunk.glDrawArrays(GL_LINES, 2, 2);
}

void VBO_Drawing::leadingLine()
{
    shotLineVBOChunk.enableArrays();
    shotLineVBOChunk.glDrawArrays(GL_LINES, 2);
}

void VBO_Drawing::leadlagLine()
{
    shotLineVBOChunk.draw(GL_LINES);
}

void VBO_Drawing::viewAngle()
{
    viewAngleVBOChunk.draw(GL_LINE_STRIP);
}

void VBO_Drawing::outlinedCircle20()
{
    outlineCircle20VBOChunk.draw(GL_LINE_LOOP);
}

void VBO_Drawing::sphere(const int slices)
{
    if (slices <= 4)
        sphere4VBOChunk.draw(GL_TRIANGLE_STRIP);
    else if (slices <= 6)
        sphere6VBOChunk.draw(GL_TRIANGLE_STRIP);
    else if (slices <= 8)
        sphere8VBOChunk.draw(GL_TRIANGLE_STRIP);
    else if (slices <= 12)
        sphere12VBOChunk.draw(GL_TRIANGLE_STRIP);
    else if (slices <= 16)
        sphere16VBOChunk.draw(GL_TRIANGLE_STRIP);
    else
        sphere32VBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::cylinder8()
{
    cylinder8VBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::cylinder10()
{
    cylinder10VBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::cylinder16()
{
    cylinder16VBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::cylinder24()
{
    cylinder24VBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::cylinder32()
{
    cylinder32VBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::disk8()
{
    disk8VBOChunk.draw(GL_TRIANGLE_FAN);
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=4 tabstop=4
