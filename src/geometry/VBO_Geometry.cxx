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

// Own Interface
#include "VBO_Geometry.h"

// System Headers
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

void Quadric::buildCylinder(float baseRadius, float topRadius,
                            float height, int slices,
                            std::vector<glm::vec3> &vertex,
                            std::vector<glm::vec3> &normal)
{
    /* Compute length (needed for normal calculations) */
    float deltaRadius = baseRadius - topRadius;
    float length      = sqrt(deltaRadius * deltaRadius + height * height);

    float zNormal       = deltaRadius / length;
    float xyNormalRatio = height / length;

    normal.clear();
    vertex.clear();

    auto normalV = glm::vec3(0.0f, xyNormalRatio, zNormal);
    normal.push_back(normalV);
    normal.push_back(normalV);
    vertex.push_back(glm::vec3(0.0f, baseRadius, 0.0f));
    vertex.push_back(glm::vec3(0.0f, topRadius,  height));
    for (int i = 1; i < slices; i++)
    {
        float angle = (float)(2 * M_PI * i / slices);
        float sinCache = sin(angle);
        float cosCache = cos(angle);
        normalV = glm::vec3(xyNormalRatio * sinCache, xyNormalRatio * cosCache, zNormal);

        normal.push_back(normalV);
        normal.push_back(normalV);
        vertex.push_back(glm::vec3(baseRadius * sinCache, baseRadius * cosCache, 0.0f));
        vertex.push_back(glm::vec3(topRadius  * sinCache, topRadius  * cosCache, height));
    }
    normalV = glm::vec3(0.0f, xyNormalRatio, zNormal);
    normal.push_back(normalV);
    normal.push_back(normalV);
    vertex.push_back(glm::vec3(0.0f, baseRadius, 0.0f));
    vertex.push_back(glm::vec3(0.0f, topRadius,  height));
}

Vertex_Chunk Quadric::buildCylinder(
    float baseRadius, float topRadius, float height, int slices)
{
    Vertex_Chunk vboIndex;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec3> vertex;

    buildCylinder(baseRadius, topRadius, height, slices, vertex, normal);
    vboIndex = Vertex_Chunk(Vertex_Chunk::VN, vertex.size());
    vboIndex.normalData(normal);
    vboIndex.vertexData(vertex);
    return vboIndex;
}

Vertex_Chunk Quadric::buildDisk(float outerRadius, int slices)
{

    std::vector<glm::vec3> vertex;

    auto v = glm::vec3(0.0, 0.0, 0.0);
    vertex.push_back(v);

    v.y = outerRadius;
    vertex.push_back(v);

    for (int i = slices - 1; i >= 1; i--)
    {
        float angle = M_PI * 2.0f * i / slices;
        v.x = outerRadius * sinf(angle);
        v.y = outerRadius * cosf(angle);
        vertex.push_back(v);
    }

    v.x = 0.0f;
    v.y = outerRadius;
    vertex.push_back(v);

    Vertex_Chunk vboIndex(Vertex_Chunk::V, vertex.size());
    vboIndex.vertexData(vertex);

    return vboIndex;
}

Vertex_Chunk Quadric::buildSphere(float radius, int slices)
{
    const int maxSlices = 32;
    glm::vec3 vertex[2 * maxSlices * (maxSlices + 1)];
    glm::vec3 normal[2 * maxSlices * (maxSlices + 1)];
    glm::vec2 textur[2 * maxSlices * (maxSlices + 1)];
    glm::vec3 *pVertex = vertex;
    glm::vec3 *pNormal = normal;
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
        *pNormal   = glm::vec3(0.0f, sintemp2, zHigh);
        *pVertex++ = radius * *pNormal++;

        *pTextur++ = glm::vec2(1, tLow);
        *pNormal   = glm::vec3(0.0f, sintemp1, zLow);
        *pVertex++ = radius * *pNormal++;

        for (i = 1; i < slices; i++)
        {
            float angleT = (float)(2 * M_PI * i / slices);
            float sinCache = sin(angleT);
            float cosCache = cos(angleT);
            float x = sinCache * sintemp2;
            float y = cosCache * sintemp2;
            float s = 1 - (float) i / slices;

            *pTextur++ = glm::vec2(s, tHigh);
            *pNormal   = glm::vec3(x, y, zHigh);
            *pVertex++ = radius * *pNormal++;

            x = sinCache * sintemp1;
            y = cosCache * sintemp1;
            *pTextur++ = glm::vec2(s, tLow);
            *pNormal   = glm::vec3(x, y, zLow);
            *pVertex++ = radius * *pNormal++;
        }

        *pTextur++ = glm::vec2(0, tHigh);
        *pNormal   = glm::vec3(0.0f, sintemp2, zHigh);
        *pVertex++ = radius * *pNormal++;

        *pTextur++ = glm::vec2(0, tLow);
        *pNormal   = glm::vec3(0.0f, sintemp2, zLow);
        *pVertex++ = radius * *pNormal++;
    }

    Vertex_Chunk vboChunk = Vertex_Chunk(Vertex_Chunk::VTN, pVertex - vertex);
    vboChunk.normalData(normal);
    vboChunk.vertexData(vertex);
    vboChunk.textureData(textur);

    return vboChunk;
}

Vertex_Chunk Simple2D::buildLine(const glm::vec3 &start, const glm::vec3 &end)
{
    glm::vec3 v[2];
    v[0] = start;
    v[1] = end;

    Vertex_Chunk vboChunk = Vertex_Chunk(Vertex_Chunk::V, 2);
    vboChunk.vertexData(v);

    return vboChunk;
}

Vertex_Chunk Simple2D::buildXYDiamond(glm::vec3 offset, float dim)
{
    glm::vec3 v[4];
    v[0] = glm::vec3( 0.0f, -1.0f, 0.0f) * dim + offset;
    v[1] = glm::vec3( 1.0f,  0.0f, 0.0f) * dim + offset;
    v[2] = glm::vec3(-1.0f,  0.0f, 0.0f) * dim + offset;
    v[3] = glm::vec3( 0.0f,  1.0f, 0.0f) * dim + offset;
    Vertex_Chunk vboChunk = Vertex_Chunk(Vertex_Chunk::V, 4);
    vboChunk.vertexData(v);

    return vboChunk;
}

Vertex_Chunk Simple2D::buildLeftTriangle(glm::vec3 offset, float dim)
{
    glm::vec3 v[3];
    v[1] = glm::vec3( 0.0f,  1.0f, 0.0f) * dim + offset;
    v[2] = glm::vec3(-1.0f,  0.0f, 0.0f) * dim + offset;
    v[0] = glm::vec3( 0.0f, -1.0f, 0.0f) * dim + offset;

    Vertex_Chunk vboChunk = Vertex_Chunk(Vertex_Chunk::V, 3);
    vboChunk.vertexData(v);

    return vboChunk;
}

Vertex_Chunk Simple2D::buildRightTriangle(glm::vec3 offset, float dim)
{
    glm::vec3 v[3];
    v[0] = glm::vec3( 0.0f, -1.0f, 0.0f) * dim + offset;
    v[1] = glm::vec3( 1.0f,  0.0f, 0.0f) * dim + offset;
    v[2] = glm::vec3( 0.0f,  1.0f, 0.0f) * dim + offset;
    Vertex_Chunk vboChunk = Vertex_Chunk(Vertex_Chunk::V, 3);
    vboChunk.vertexData(v);

    return vboChunk;
}

Vertex_Chunk Simple2D::buildTexRectXZ(float width, float base, float height)
{
    glm::vec2 t[4];
    glm::vec3 v[4];

    t[0] = glm::vec2(0.0f,  0.0f);
    v[0] = glm::vec3(0.0f,  0.0f, base);
    t[1] = glm::vec2(1.0f,  0.0f);
    v[1] = glm::vec3(width, 0.0f, base);
    t[2] = glm::vec2(0.0f,  1.0f);
    v[2] = glm::vec3(0.0f,  0.0f, base + height);
    t[3] = glm::vec2(1.0f,  1.0f);
    v[3] = glm::vec3(width, 0.0f, base + height);

    Vertex_Chunk vboChunk = Vertex_Chunk(Vertex_Chunk::VT, 4);
    vboChunk.vertexData(v);
    vboChunk.textureData(t);

    return vboChunk;
}



// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
