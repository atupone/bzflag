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

// Common headers
#include "VBO_Geometry.h"

static int vertexForCylinder(int slices)
{
    return 2 * (slices + 1);
}

VBO_Drawing::VBO_Drawing()
    : simmetricSquareLoopVBOChunk(Vertex_Chunk::V, 4)
    , asimmetricSquareLoopVBOChunk(Vertex_Chunk::V, 4)
    , diamondLoopVBOChunk(Vertex_Chunk::V, 4)
    , diamondTXZVBOChunk(Vertex_Chunk::VT, 4)
    , diamondTXYVBOChunk(Vertex_Chunk::VT, 4)
    , crossVBOChunk(Vertex_Chunk::V, 4)
    , northVBOChunk(Vertex_Chunk::V, 4)
    , simmetricRectVBOChunk(Vertex_Chunk::V, 4)
    , asimmetricRectVBOChunk(Vertex_Chunk::V, 4)
    , asimmetricRectXZVBOChunk(Vertex_Chunk::V, 4)
    , simmetricTexturedRectVBOChunk(Vertex_Chunk::VT, 4)
    , simmetricTexturedRectXZVBOChunk(Vertex_Chunk::VT, 4)
    , asimmetricTexturedRectVBOChunk(Vertex_Chunk::VT, 4)
    , verticalTexturedRectVBOChunk(Vertex_Chunk::VT, 4)
    , isoscelesTriangleXYVBOChunk(Vertex_Chunk::V, 3)
    , triangleVBOChunk(Vertex_Chunk::V, 3)
    , shotLineVBOChunk(Vertex_Chunk::V, 4)
    , viewAngleVBOChunk(Vertex_Chunk::V, 3)
    , outlineCircle20VBOChunk(Vertex_Chunk::V, outlineSides)
    , cylinderNX10VBOChunk(Vertex_Chunk::VN, vertexForCylinder(10))
    , cylinderNX16VBOChunk(Vertex_Chunk::VN, vertexForCylinder(16))
    , cylinderNX24VBOChunk(Vertex_Chunk::VN, vertexForCylinder(24))
    , cylinderNX32VBOChunk(Vertex_Chunk::VN, vertexForCylinder(32))
    , beamVBOChunk(Vertex_Chunk::V, 14)
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

    textur[0] = glm::vec2(0.0f, 0.0f);
    textur[1] = glm::vec2(0.0f, 1.0f);
    textur[2] = glm::vec2(1.0f, 0.0f);
    textur[3] = glm::vec2(1.0f, 1.0f);

    vertex[0] = glm::vec3(0.0f, 0.0f,  1.0f);
    vertex[1] = glm::vec3(1.0f, 0.0f,  1.0f);
    vertex[2] = glm::vec3(0.0f, 0.0f, -1.0f);
    vertex[3] = glm::vec3(1.0f, 0.0f, -1.0f);
    diamondTXZVBOChunk.textureData(textur);
    diamondTXZVBOChunk.vertexData(vertex);

    vertex[0] = glm::vec3(0.0f,  1.0f, 0.0f);
    vertex[1] = glm::vec3(1.0f,  1.0f, 0.0f);
    vertex[2] = glm::vec3(0.0f, -1.0f, 0.0f);
    vertex[3] = glm::vec3(1.0f, -1.0f, 0.0f);
    diamondTXYVBOChunk.textureData(textur);
    diamondTXYVBOChunk.vertexData(vertex);

    asimmetricLineXVBOChunk = Simple2D::buildLine(
                                  glm::vec3(0.0f),
                                  glm::vec3(1.0f, 0.0f, 0.0f));

    asimmetricLineYVBOChunk = Simple2D::buildLine(
                                  glm::vec3(0.0f),
                                  glm::vec3(0.0f, 1.0f, 0.0f));

    simmetricLineXVBOChunk = Simple2D::buildLine(
                                 glm::vec3(-1.0f, 0.0f, 0.0f),
                                 glm::vec3( 1.0f, 0.0f, 0.0f));

    simmetricLineYVBOChunk = Simple2D::buildLine(
                                 glm::vec3(0.0f, -1.0f, 0.0f),
                                 glm::vec3(0.0f,  1.0f, 0.0f));

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

    vertex[0] = glm::vec3(-1.0f, 0.0f, -1.0f);
    vertex[1] = glm::vec3( 1.0f, 0.0f, -1.0f);
    vertex[2] = glm::vec3(-1.0f, 0.0f,  1.0f);
    vertex[3] = glm::vec3( 1.0f, 0.0f,  1.0f);

    simmetricTexturedRectXZVBOChunk.textureData(textur);
    simmetricTexturedRectXZVBOChunk.vertexData(vertex);

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

    vertex[0] = glm::vec3( 0.0f, 0.0f, 1.0f);
    vertex[1] = glm::vec3( 1.0f, 1.0f, 1.0f);
    vertex[2] = glm::vec3(-1.0f, 1.0f, 1.0f);
    triangleVBOChunk.vertexData(vertex);

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

    sphere4VBOChunk  = Quadric::buildSphere(1.0f, 4);
    sphere6VBOChunk  = Quadric::buildSphere(1.0f, 6);
    sphere8VBOChunk  = Quadric::buildSphere(1.0f, 8);
    sphere16VBOChunk = Quadric::buildSphere(1.0f, 16);
    sphere32VBOChunk = Quadric::buildSphere(1.0f, 32);
    buildCylinderNX(10, cylinderNX10VBOChunk);
    buildCylinderNX(16, cylinderNX16VBOChunk);
    buildCylinderNX(24, cylinderNX24VBOChunk);
    buildCylinderNX(32, cylinderNX32VBOChunk);
    int j = 0;
    for (int i = 0; i < 6; i++)
    {
        const float angle = 2.0 * M_PI * float(i) / 6.0;
        auto vec = glm::vec3(0.0f, -cosf(angle), sinf(angle));
        vertex[j++] = vec;
        vec.x = 1.0f;
        vertex[j++] = vec;
    }
    auto vec = glm::vec3(0.0f, -1.0f, 0.0f);
    vertex[j++] = vec;
    vec.x = 1.0f;
    vertex[j++] = vec;
    beamVBOChunk.vertexData(vertex);
}

void VBO_Drawing::buildCylinderNX(int slices, Vertex_Chunk &vboChunk)
{
    const int maxSlices = 32;
    glm::vec3 vertex[2 * (maxSlices + 1)];
    glm::vec3 normal[2 * (maxSlices + 1)];
    glm::vec3 *pVertex = vertex;
    glm::vec3 *pNormal = normal;
    int   i;
    float angle;
    glm::vec2 currentVartex;

    currentVartex = glm::vec2(0.0f, 1.0f);
    *pNormal++ = glm::vec3(0.0f, currentVartex);
    *pVertex++ = glm::vec3(0.0f, currentVartex);
    *pNormal++ = glm::vec3(0.0f, currentVartex);
    *pVertex++ = glm::vec3(1.0f, currentVartex);
    for (i = 1; i < slices; i++)
    {
        angle = (float)(2 * M_PI * i / slices);
        currentVartex = glm::vec2(sin(angle), cos(angle));
        *pNormal++ = glm::vec3(0.0f, currentVartex);
        *pVertex++ = glm::vec3(0.0f, currentVartex);
        *pNormal++ = glm::vec3(0.0f, currentVartex);
        *pVertex++ = glm::vec3(1.0f, currentVartex);
    }
    currentVartex = glm::vec2(0.0f, 1.0f);
    *pNormal++ = glm::vec3(0.0f, currentVartex);
    *pVertex++ = glm::vec3(0.0f, currentVartex);
    *pNormal++ = glm::vec3(0.0f, currentVartex);
    *pVertex++ = glm::vec3(1.0f, currentVartex);
    vboChunk.normalData(normal);
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

void VBO_Drawing::diamondTexturedXZ()
{
    diamondTXZVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::diamondTexturedXY()
{
    diamondTXYVBOChunk.draw(GL_TRIANGLE_STRIP);
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

void VBO_Drawing::simmetricTexturedRect()
{
    simmetricTexturedRectVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::simmetricTexturedRectXZ()
{
    simmetricTexturedRectXZVBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::asimmetricTexturedRect()
{
    asimmetricTexturedRectVBOChunk.draw(GL_TRIANGLE_STRIP);
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

void VBO_Drawing::triangleFilled()
{
    triangleVBOChunk.draw(GL_TRIANGLES);
}

void VBO_Drawing::triangleOutline()
{
    triangleVBOChunk.draw(GL_LINE_STRIP);
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
    else if (slices <= 16)
        sphere16VBOChunk.draw(GL_TRIANGLE_STRIP);
    else
        sphere32VBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::cylinderX10()
{
    cylinderNX10VBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::cylinderX16()
{
    cylinderNX16VBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::cylinderX24()
{
    cylinderNX24VBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::cylinderX32()
{
    cylinderNX32VBOChunk.draw(GL_TRIANGLE_STRIP);
}

void VBO_Drawing::beam()
{
    beamVBOChunk.draw(GL_TRIANGLE_STRIP);
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=4 tabstop=4
