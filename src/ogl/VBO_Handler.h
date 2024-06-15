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

#include "common.h"

// system interface headers
#include <list>
#include <string>

// Manage the allocation of memory inside the GL buffers
// You can allocate/deallocate memory from this class
class VBO_Handler
{
public:
    VBO_Handler();
    virtual ~VBO_Handler();

    // Called to initialize/destroy the buffer
    virtual void init() = 0;
    virtual void destroy() = 0;

    // Name of the Buffer (only for diagnostic)
    virtual std::string vboName() = 0;

    // This method allocate Vsize elements and return
    // the index of the first element
    // It will search in the free memory list a chunk big
    // enough to contain the requested size and remove the chunk
    // or resize it, from the free list.
    // It will add in the allocate memory list the new chunk
    int alloc(int Vsize);
    // This method free the elements pointed by vboIndex
    // the vboIndex should come from a previous alloc
    // This search in the allocate memory list the chunk that start
    // at vboIndex, remove it from the allocate List and add
    // to the free memeory list. It recompat that, in case
    void free(int vboIndex);

    // Resize buffers
    void resize(int newSize);

    // Resize all vectors
    virtual void resize() = 0;

protected:
    // Size of the buffer (in terms of elements)
    int  vboSize;

private:
    // Those are callback for GL Context creation/free
    // Called by OpenGLGState::
    static void initContext(void* data);
    static void freeContext(void* data);

    int intAlloc(int Vsize);

    // Chunk of memory present in the buffer
    struct MemElement
    {
        int vboIndex;
        int Vsize;
    };

    // Add the memory chunk index, size to free List
    void free(int index, int size);
    // Free Memory List
    // At the beginning it contains only a chunk of memory
    // from 0 to vboSize
    std::list<MemElement> freeList;
    // Allocate Memory List
    // At the start it is empty
    std::list<MemElement> allocList;
};

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4