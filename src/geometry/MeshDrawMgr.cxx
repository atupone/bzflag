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

// implementation header
#include "MeshDrawMgr.h"

// System headers
#include <algorithm>
#include <cstdint>

// common headers
#include "MeshDrawInfo.h"
#include "bzfgl.h"

MeshDrawMgr::MeshDrawMgr(const MeshDrawInfo* drawInfo_)
    : drawInfo(drawInfo_)
{
    if (drawInfo == nullptr)
        return;

    auto vertices  = reinterpret_cast<const GLfloat*>(drawInfo->getVertices());
    auto normals   = reinterpret_cast<const GLfloat*>(drawInfo->getNormals());
    auto texcoords = reinterpret_cast<const GLfloat*>(drawInfo->getTexcoords());

    int vertCount = drawInfo->getVertexCount();

    if (vertices != nullptr && vertCount > 0)
    {
        // Select chunk type based on available attribute streams
        auto type = Vertex_Chunk::V;
        if (texcoords && normals)
            type = Vertex_Chunk::VTN;
        else if (texcoords)
            type = Vertex_Chunk::VT;
        else if (normals)
            type = Vertex_Chunk::VN;

        vChunk = Vertex_Chunk(type, vertCount);
        vChunk.vertexData(vertices, vertCount);
        if (normals)
            vChunk.normalData(normals, vertCount);
        if (texcoords)
            vChunk.textureData(texcoords, vertCount);
        vChunk.syncInterlaced();
    }

    lodCount = drawInfo->getLodCount();
    const DrawLod* drawLods = drawInfo->getDrawLods();

    if (lodCount > 0 && drawLods != nullptr)
    {
        for (int l = 0; l < lodCount; ++l)
        {
            if (drawLods[l].count > maxSetsPerLod)
                maxSetsPerLod = drawLods[l].count;
        }

        const size_t totalSlots = static_cast<size_t>(lodCount * maxSetsPerLod);
        setOffsets.resize(totalSlots, 0);
        setCounts.resize(totalSlots, 0);
        setLoadedFlags.resize(totalSlots, 0);
    }
}

bool MeshDrawMgr::isSetUnloaded(int lod, int set) const
{
    if (lod < 0 || lod >= lodCount || set < 0 || set >= maxSetsPerLod)
        return false;

    const size_t slot = static_cast<size_t>(lod * maxSetsPerLod + set);
    return setLoadedFlags[slot] == 0;
}

void MeshDrawMgr::ensureSetLoaded(int lod, int set)
{
    if (drawInfo == nullptr || lod < 0 || lod >= lodCount)
        return;

    const DrawLod* drawLods = drawInfo->getDrawLods();
    if (drawLods == nullptr || set < 0 || set >= drawLods[lod].count)
        return;

    const size_t slot = static_cast<size_t>(lod * maxSetsPerLod + set);
    if (setLoadedFlags[slot] != 0)
        return;

    setLoadedFlags[slot] = 1;

    const DrawSet& drawSet = drawLods[lod].sets[set];
    if (drawSet.count <= 0)
        return;

    uint32_t startOffset = static_cast<uint32_t>(flatChunks.size());
    uint32_t loadedCmds = 0;

    constexpr int STACK_BUF_LIMIT = 1024;

    for (int c = 0; c < drawSet.count; ++c)
    {
        const DrawCmd& cmd = drawSet.cmds[c];

        if (cmd.count <= 0)
            continue;

        Element_Chunk cmdChunk(&vChunk, cmd.count, cmd.drawMode);

        if (cmd.indexType == DrawCmd::DrawIndexUInt)
            cmdChunk.elementData(reinterpret_cast<const GLuint*>(cmd.indices), cmd.count);
        else if (cmd.indexType == DrawCmd::DrawIndexUShort)
        {
            const unsigned short* src = static_cast<const unsigned short*>(cmd.indices);
            if (cmd.count <= STACK_BUF_LIMIT)
            {
                GLuint stackIndices[STACK_BUF_LIMIT];
                for (int i = 0; i < cmd.count; ++i)
                    stackIndices[i] = static_cast<GLuint>(src[i]);
                cmdChunk.elementData(stackIndices, cmd.count);
            }
            else
            {
                std::vector<GLuint> cmdIndices;
                cmdIndices.reserve(cmd.count);
                for (int i = 0; i < cmd.count; ++i)
                    cmdIndices.push_back(static_cast<GLuint>(src[i]));
                cmdChunk.elementData(cmdIndices.data(), cmd.count);
            }
        }

        flatChunks.push_back(std::move(cmdChunk));
        loadedCmds++;
    }

    setOffsets[slot] = startOffset;
    setCounts[slot] = loadedCmds;
}


void MeshDrawMgr::executeSet(int lod, int set, bool useNormals, bool useTexcoords)
{
    size_t count = 0;
    const Element_Chunk* chunks = getSetChunks(lod, set, count);
    if (chunks == nullptr || count == 0)
        return;

    const AnimationInfo* animInfo = drawInfo->getAnimationInfo();
    if (animInfo != nullptr)
    {
        glPushMatrix();
        glRotatef(animInfo->angle, 0.0f, 0.0f, 1.0f);
    }

    vChunk.enableArrays(useTexcoords, useNormals, false);

    for (size_t i = 0; i < count; ++i)
        chunks[i].glDrawElements();

    if (animInfo != nullptr)
        glPopMatrix();

    return;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
