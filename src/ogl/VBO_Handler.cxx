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
#include "VBO_Handler.h"

// System headers
#include <iostream>
#include <algorithm>

// Common headers
#include "OpenGLGState.h"

VBO_Handler::VBO_Handler() :
    vboSize(0)
{
    // Registering the GL Context
    OpenGLGState::registerContextInitializer(freeContext, initContext, this);
}

VBO_Handler::~VBO_Handler()
{
    OpenGLGState::unregisterContextInitializer(freeContext, initContext, this);
}

void VBO_Handler::initContext(void* data)
{
    static_cast<VBO_Handler*>(data)->init();
}

void VBO_Handler::freeContext(void* data)
{
    static_cast<VBO_Handler*>(data)->destroy();
}

void VBO_Handler::resize(int newSize)
{
    free(vboSize, newSize - vboSize);
    vboSize = newSize;
}

int VBO_Handler::intAlloc(int Vsize)
{
    MemElement memElement;

    // First check if there is a chunk that fit precisely
    auto it = std::find_if(freeList.begin(), freeList.end(),
                           [&](struct MemElement &item)
    {
        return item.Vsize == Vsize;
    });
    if (it == freeList.end())
        // If not check if there is a chunk big enough
        it = std::find_if(freeList.begin(), freeList.end(),
                          [&](struct MemElement &item)
    {
        return item.Vsize > Vsize;
    });
    if (it == freeList.end())
        // Bad, not enough memory (or too fragmented)
        return -1;

    // Push a new element in the alloc list
    memElement.Vsize    = Vsize;
    memElement.vboIndex = it->vboIndex;
    allocList.push_back(memElement);

    // Reduce the size of the old chunk
    it->Vsize    -= Vsize;
    it->vboIndex += Vsize;;
    // If nothing more from the chunk drop it from the Free List
    if (!it->Vsize)
        freeList.erase(it);

    // return the address of the chunk
    return memElement.vboIndex;
}

int VBO_Handler::alloc(int Vsize)
{
    while (true)
    {
        int index = intAlloc(Vsize);
        if (index >= 0)
            return index;
        resize();
    }
}

void VBO_Handler::free(int vboIndex)
{
    if (vboIndex < 0)
        return;

    // Check if we allocated that index
    auto it = std::find_if(allocList.begin(), allocList.end(),
                           [&](struct MemElement &item)
    {
        return (item.vboIndex == vboIndex);
    });

    if (it == allocList.end())
    {
        // Bad, that index was never allocated
        std::cout << vboName() << " deallocated " << vboIndex <<
                  " never allocated" << std::endl;
        abort();
        return;
    }

    free(vboIndex, it->Vsize);

    allocList.erase(it);
}

void VBO_Handler::free(int index, int size)
{
    MemElement memElement;

    // Save the chunk and drop from the allocated list
    memElement.vboIndex = index;
    memElement.Vsize    = size;

    // Check in the free list for a contiguous previous chunk
    auto it = std::find_if(freeList.begin(), freeList.end(),
                           [&](struct MemElement &item)
    {
        return (item.vboIndex + item.Vsize == memElement.vboIndex);
    });

    if (it != freeList.end())
    {
        memElement.vboIndex = it->vboIndex;
        memElement.Vsize   += it->Vsize;
        freeList.erase(it);
    }

    // Check in the free list for a contiguous successor chunk
    it = std::find_if(freeList.begin(), freeList.end(),
                      [&](struct MemElement &item)
    {
        return (item.vboIndex == memElement.vboIndex + memElement.Vsize);
    });
    if (it != freeList.end())
    {
        memElement.Vsize   += it->Vsize;
        freeList.erase(it);
    }
    freeList.push_back(memElement);
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
