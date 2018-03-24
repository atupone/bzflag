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
    void simmetricLineX();
    void simmetricLineY();
    void asimmetricLineX();
    void asimmetricLineY();
    void asimmetricLineZ();
    void cross();
    void north();
    void simmetricRect();
    void asimmetricRect();
    void asimmetricRectXZ();
    void rectSXAZ();
    void cubeSXYAZ();
    void asimmetricTexturedRect();
    void asimmetricTexturedRectXZ();
    void simmetricTexturedRect();
    void verticalTexturedRect();
    void isoscelesTriangleXYFilled();
    void isoscelesTriangleXYOutline();
    void laggingLine();
    void leadingLine();
    void leadlagLine();
    void viewAngle();
    void outlinedCircle20();
    void sphere(const int slices);
    void cylinder8();
    void cylinder10();
    void cylinder16();
    void cylinder24();
    void cylinder32();
    void disk8();
protected:
    friend class Singleton<VBO_Drawing>;
private:
    VBO_Drawing();
    virtual ~VBO_Drawing() = default;

    void buildSphere(int slices, Vertex_Chunk &vboChunk);
    void buildCylinder(int slices, Vertex_Chunk &vboChunk);
    void buildDisk(int slices, Vertex_Chunk &vboChunk);

    const int outlineSides = 20;

    Vertex_Chunk simmetricSquareLoopVBOChunk;
    Vertex_Chunk asimmetricSquareLoopVBOChunk;
    Vertex_Chunk diamondLoopVBOChunk;
    Vertex_Chunk asimmetricLineXVBOChunk;
    Vertex_Chunk asimmetricLineYVBOChunk;
    Vertex_Chunk simmetricLineXVBOChunk;
    Vertex_Chunk simmetricLineYVBOChunk;
    Vertex_Chunk asimmetricLineZVBOChunk;
    Vertex_Chunk crossVBOChunk;
    Vertex_Chunk northVBOChunk;
    Vertex_Chunk simmetricRectVBOChunk;
    Vertex_Chunk asimmetricRectVBOChunk;
    Vertex_Chunk asimmetricRectXZVBOChunk;
    Vertex_Chunk rectSXAZVBOChunk;
    Vertex_Chunk cubeSXYAZVBOChunk;
    Vertex_Chunk simmetricTexturedRectVBOChunk;
    Vertex_Chunk asimmetricTexturedRectVBOChunk;
    Vertex_Chunk asimmetricTexturedRectXZVBOChunk;
    Vertex_Chunk verticalTexturedRectVBOChunk;
    Vertex_Chunk isoscelesTriangleXYVBOChunk;
    Vertex_Chunk shotLineVBOChunk;
    Vertex_Chunk viewAngleVBOChunk;
    Vertex_Chunk outlineCircle20VBOChunk;
    Vertex_Chunk sphere4VBOChunk;
    Vertex_Chunk sphere6VBOChunk;
    Vertex_Chunk sphere8VBOChunk;
    Vertex_Chunk sphere12VBOChunk;
    Vertex_Chunk sphere16VBOChunk;
    Vertex_Chunk sphere32VBOChunk;
    Vertex_Chunk cylinder8VBOChunk;
    Vertex_Chunk cylinder10VBOChunk;
    Vertex_Chunk cylinder16VBOChunk;
    Vertex_Chunk cylinder24VBOChunk;
    Vertex_Chunk cylinder32VBOChunk;
    Vertex_Chunk disk8VBOChunk;
};


// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
