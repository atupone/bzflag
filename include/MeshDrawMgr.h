/* bzflag
 * Copyright (c) 1993-2025 Tim Riker
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

// 1st
#include "common.h"

// System headers
#include <vector>
#include <cstdint>

// Common headers
#include "Vertex_Chunk.h"
#include "Element_Chunk.h"

class MeshDrawMgr
{
public:
    MeshDrawMgr(const class MeshDrawInfo* drawInfo_);
    ~MeshDrawMgr() = default;

    void executeSet(int lod, int set, bool useNormals = true, bool useTexcoords = true);

private:
    void ensureSetLoaded(int lod, int set);

    // Fast O(1) inline retrieval: direct pointer + count slice
    inline const Element_Chunk* getSetChunks(int lod, int set, size_t& count)
    {
        if (lod < 0 || lod >= lodCount)
        {
            count = 0;
            return nullptr;
        }

        const size_t slot = static_cast<size_t>(lod * maxSetsPerLod + set);
        if (slot >= setOffsets.size())
        {
            count = 0;
            return nullptr;
        }

        uint32_t offset = setOffsets[slot];
        count = setCounts[slot];

        // Lazy-load chunk geometry on demand if not ready
        if (count == 0 && isSetUnloaded(lod, set))
        {
            ensureSetLoaded(lod, set);
            offset = setOffsets[slot];
            count = setCounts[slot];
        }

        return (count > 0) ? &flatChunks[offset] : nullptr;
    }

    bool isSetUnloaded(int lod, int set) const;

    const MeshDrawInfo* drawInfo = nullptr;
    Vertex_Chunk  vChunk;

    int lodCount = 0;
    int maxSetsPerLod = 0;

    // Contiguous memory pool for all chunks
    std::vector<Element_Chunk> flatChunks;

    // Flat indexing lookup: slot = (lod * maxSetsPerLod) + set
    std::vector<uint32_t> setOffsets;
    std::vector<uint32_t> setCounts;
    std::vector<uint8_t>  setLoadedFlags;
};


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
