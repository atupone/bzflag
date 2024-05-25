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
#include "VBO_Quadric.h"

// System Headers
#include <glm/vec3.hpp>

void buildCylinder(float baseRadius, float topRadius, float height, int slices,
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

Vertex_Chunk buildCylinder(
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

Vertex_Chunk buildDisk(float outerRadius, int slices)
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


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
