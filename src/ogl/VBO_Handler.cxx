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
    OpenGLGState::registerContextInitializer(freeContext, initContext, this);
    freeVBOList.push_back({0, vboSize});
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

int VBO_Handler::vboAlloc(int Vsize)
{
    MemElement memElement;

    // First check if there is a chunk that fit precisely
    auto it = std::find_if(freeVBOList.begin(), freeVBOList.end(),
                           [&](struct MemElement &item)
    {
        return item.Vsize == Vsize;
    });
    if (it == freeVBOList.end())
        // If not check if there is a chunk big enough
        it = std::find_if(freeVBOList.begin(), freeVBOList.end(),
                          [&](struct MemElement &item)
    {
        return item.Vsize > Vsize;
    });
    if (it == freeVBOList.end())
        // Bad, not enough memory (or too fragmented)
        return -1;

    // Push a new element in the alloc list
    memElement.Vsize    = Vsize;
    memElement.vboIndex = it->vboIndex;
    alloVBOList.push_back(memElement);

    // Reduse the size of the old chunk
    it->Vsize    -= Vsize;
    it->vboIndex += Vsize;;
    // If nothing more drop from the Free List
    if (!it->Vsize)
        freeVBOList.erase(it);

    // return the address of the chunk
    return memElement.vboIndex;
}

void VBO_Handler::vboFree(int vboIndex)
{
    if (vboIndex < 0)
        return;

    // Check if we allocated that index
    auto it = std::find_if(alloVBOList.begin(), alloVBOList.end(),
                           [&](struct MemElement &item)
    {
        return (item.vboIndex == vboIndex);
    });

    if (it == alloVBOList.end())
    {
        // Bad, that index was never allocated
        std::cout << vboName() << " deallocated " << vboIndex <<
                  " never allocated" << std::endl;
        abort();
        return;
    }

    free(vboIndex, it->Vsize);

    alloVBOList.erase(it);
}

void VBO_Handler::free(int index, int size)
{
    MemElement memElement;

    // Save the chunk and drop from the allocated list
    memElement.vboIndex = index;
    memElement.Vsize    = size;

    // Check in the free list for a contiguous previous chunk
    auto it = std::find_if(freeVBOList.begin(), freeVBOList.end(),
                           [&](struct MemElement &item)
    {
        return (item.vboIndex + item.Vsize == memElement.vboIndex);
    });

    if (it != freeVBOList.end())
    {
        memElement.vboIndex = it->vboIndex;
        memElement.Vsize   += it->Vsize;
        freeVBOList.erase(it);
    }

    // Check in the free list for a contiguous successor chunk
    it = std::find_if(freeVBOList.begin(), freeVBOList.end(),
                      [&](struct MemElement &item)
    {
        return (item.vboIndex == memElement.vboIndex + memElement.Vsize);
    });
    if (it != freeVBOList.end())
    {
        memElement.Vsize   += it->Vsize;
        freeVBOList.erase(it);
    }
    freeVBOList.push_back(memElement);
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
