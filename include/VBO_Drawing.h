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

/* interface headers */
#include "Singleton.h"

// Common headers
#include "Vertex_Chunk.h"

#define DRAWER (VBO_Drawing::instance())

class VBO_Drawing: public Singleton<VBO_Drawing>
{
public:
    void point();
    void simmetricSquareLoop();
    void asimmetricSquareLoop();
    void diamondLoop();
    void diamondTexturedXZ();
    void diamondTexturedXY();
    void simmetricLineX();
    void simmetricLineY();
    void asimmetricLineX();
    void asimmetricLineY();
    void cross();
    void north();
    void simmetricRect();
    void asimmetricRect();
    void asimmetricRectXZ();
    void asimmetricTexturedRect();
    void simmetricTexturedRect();
    void simmetricTexturedRectXZ();
    void verticalTexturedRect();
    void isoscelesTriangleXYFilled();
    void isoscelesTriangleXYOutline();
    void triangleFilled();
    void triangleOutline();
    void laggingLine();
    void leadingLine();
    void leadlagLine();
    void viewAngle();
    void outlinedCircle20();
    void sphere(const int slices);
    void cylinderX10();
    void cylinderX16();
    void cylinderX24();
    void cylinderX32();
    void beam();
protected:
    friend class Singleton<VBO_Drawing>;
private:
    VBO_Drawing();
    virtual ~VBO_Drawing() = default;

    void buildCylinderNX(int slices, Vertex_Chunk &vboChunk);

    const int outlineSides = 20;

    Vertex_Chunk simmetricSquareLoopVBOChunk;
    Vertex_Chunk asimmetricSquareLoopVBOChunk;
    Vertex_Chunk diamondLoopVBOChunk;
    Vertex_Chunk diamondTXZVBOChunk;
    Vertex_Chunk diamondTXYVBOChunk;
    Vertex_Chunk asimmetricLineXVBOChunk;
    Vertex_Chunk asimmetricLineYVBOChunk;
    Vertex_Chunk simmetricLineXVBOChunk;
    Vertex_Chunk simmetricLineYVBOChunk;
    Vertex_Chunk crossVBOChunk;
    Vertex_Chunk northVBOChunk;
    Vertex_Chunk simmetricRectVBOChunk;
    Vertex_Chunk asimmetricRectVBOChunk;
    Vertex_Chunk asimmetricRectXZVBOChunk;
    Vertex_Chunk simmetricTexturedRectVBOChunk;
    Vertex_Chunk simmetricTexturedRectXZVBOChunk;
    Vertex_Chunk asimmetricTexturedRectVBOChunk;
    Vertex_Chunk verticalTexturedRectVBOChunk;
    Vertex_Chunk isoscelesTriangleXYVBOChunk;
    Vertex_Chunk triangleVBOChunk;
    Vertex_Chunk shotLineVBOChunk;
    Vertex_Chunk viewAngleVBOChunk;
    Vertex_Chunk outlineCircle20VBOChunk;
    Vertex_Chunk sphere4VBOChunk;
    Vertex_Chunk sphere6VBOChunk;
    Vertex_Chunk sphere8VBOChunk;
    Vertex_Chunk sphere16VBOChunk;
    Vertex_Chunk sphere32VBOChunk;
    Vertex_Chunk cylinderNX10VBOChunk;
    Vertex_Chunk cylinderNX16VBOChunk;
    Vertex_Chunk cylinderNX24VBOChunk;
    Vertex_Chunk cylinderNX32VBOChunk;
    Vertex_Chunk beamVBOChunk;
};


// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
