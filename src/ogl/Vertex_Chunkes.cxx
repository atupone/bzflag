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
#include "Vertex_Chunkes.h"
#include "VBO_Vertex.h"

Vertex_Chunkes::Vertex_Chunkes():
    drawCount(0),
    myVBO(nullptr)
{
}

Vertex_Chunkes::~Vertex_Chunkes()
{
    delete [] first;
    delete [] count;
}


void Vertex_Chunkes::setChunkes(std::vector<Vertex_Chunk> &chunkes)
{
    if (drawCount)
    {
        delete [] first;
        first = nullptr;
        delete [] count;
        count = nullptr;
        myVBO = nullptr;
    }

    drawCount = chunkes.size();
    if (!drawCount)
        return;

    first = new GLint[drawCount];
    count = new GLsizei[drawCount];
    myVBO = chunkes[0].myVBO;

    GLint   *firstP = first;
    GLsizei *countP = count;

    for (auto &vbo : chunkes)
    {
        *firstP++ = vbo.index;
        *countP++  = vbo.indexSize;
    }
}

void Vertex_Chunkes::draw(GLenum mode)
{
    if (myVBO)
        myVBO->enableArrays();

    ::glMultiDrawArrays(mode, first, count, drawCount);
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
