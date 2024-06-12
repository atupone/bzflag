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

#pragma once

// Common headers
#include "Vertex_Chunk.h"

class Quadric
{
public:
    static Vertex_Chunk buildDisk(float outerRadius, int slices);
    static Vertex_Chunk buildSphere(float radius, int slices);
    static Vertex_Chunk buildCylinder(
        float baseRadius, float topRadius, float height, int slices);
private:
    static void buildCylinder(float baseRadius, float topRadius,
                              float height, int slices,
                              std::vector<glm::vec3> &vertex,
                              std::vector<glm::vec3> &normal);
};

class Simple2D
{
public:
    static Vertex_Chunk buildLine(const glm::vec3 &start,
                                  const glm::vec3 &end);
    static Vertex_Chunk buildXYDiamond(glm::vec3 offset, float dim);
    static Vertex_Chunk buildLeftTriangle(glm::vec3 offset, float dim);
    static Vertex_Chunk buildRightTriangle(glm::vec3 offset, float dim);
    static Vertex_Chunk buildTexRectXZ(float width,
                                       float base,
                                       float height);
};


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
