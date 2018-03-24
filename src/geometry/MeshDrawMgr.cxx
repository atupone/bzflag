/* bzflag
 * Copyright (c) 1993-2023 Tim Riker
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
#include <array>

// common headers
#include "bzfgl.h"
#include "OpenGLGState.h"
#include "MeshDrawInfo.h"
#include "bzfio.h" // for DEBUGx()

MeshDrawMgr::MeshDrawMgr(const MeshDrawInfo* drawInfo_)
    : drawInfo(drawInfo_)
    , vboVertexChunk(Vertex_Chunk::VTN, drawInfo->getCornerCount())
{
    if ((drawInfo == nullptr) || !drawInfo->isValid())
    {
        printf("MeshDrawMgr: invalid drawInfo\n");
        fflush(stdout);
        return;
    }
    else
    {
        logDebugMessage(4,"MeshDrawMgr: initializing\n");
        fflush(stdout);
    }

    auto lodCount = drawInfo->getLodCount();
    lodLists.resize(lodCount);

    makeLists();
}




inline void MeshDrawMgr::rawExecuteCommands(int lod, int set)
{
    auto drawLods = drawInfo->getDrawLods();
    const DrawLod& drawLod = drawLods[lod];
    const DrawSet& drawSet = drawLod.sets[set];
    const int cmdCount = drawSet.count;
    int fillP = 0;
    for (int i = 0; i < cmdCount; i++)
    {
        const DrawCmd& cmd = drawSet.cmds[i];
        lodLists[lod][set].glDrawElements(cmd.drawMode, cmd.count, fillP);
        fillP += cmd.count;
    }
    return;
}


void MeshDrawMgr::executeSet(int lod, int set, bool useNormals, bool useTexcoords)
{
    // FIXME (what is broken?)
    const AnimationInfo* animInfo = drawInfo->getAnimationInfo();
    if (animInfo != nullptr)
    {
        glPushMatrix();
        glRotatef(animInfo->angle, 0.0f, 0.0f, 1.0f);
    }

    {
        vboVertexChunk.enableArrays(useTexcoords, useNormals, false);

        rawExecuteCommands(lod, set);

    }

    if (animInfo != nullptr)
        glPopMatrix();

    return;
}


void MeshDrawMgr::makeLists()
{

    const auto &vertices  = drawInfo->getVertices();
    const auto &normals   = drawInfo->getNormals();
    const auto &texcoords = drawInfo->getTexcoords();

    vboVertexChunk.vertexData(vertices);
    vboVertexChunk.textureData(texcoords);
    vboVertexChunk.normalData(normals);

    auto curDrawLod = drawInfo->getDrawLods();

    // Working array of indices
    std::vector<GLuint> indices;

    for (auto &item : lodLists)
    {
        const DrawLod& drawLod = *(curDrawLod++);
        item.resize(drawLod.count);
        for (auto set = 0; set < drawLod.count; set++)
        {
            const DrawSet& drawSet = drawLod.sets[set];
            const int cmdCount = drawSet.count;

            // Number of used elements of the array
            unsigned int fillP = 0;

            for (int i = 0; i < cmdCount; i++)
                fillP += drawSet.cmds[i].count;

            indices.resize(fillP);

            // Iterator for traversing the above array.
            auto indexP = indices.begin();

            for (int i = 0; i < cmdCount; i++)
            {
                const DrawCmd& cmd = drawSet.cmds[i];

                // for each DrawCmd, append its indices to our array
                if (cmd.indexType == GL_UNSIGNED_SHORT)
                {
                    auto src = static_cast<GLushort*>(cmd.indices);
                    indexP = std::transform(src, src + cmd.count, indexP,
                                            [&](GLushort p)
                    {
                        return p + vboVertexChunk.getIndex();
                    });
                }
                else if (cmd.indexType == GL_UNSIGNED_INT)
                {
                    auto src = static_cast<GLuint*>(cmd.indices);
                    indexP = std::transform(src, src + cmd.count, indexP,
                                            [&](GLuint p)
                    {
                        return p + vboVertexChunk.getIndex();
                    });
                }
            }
            item[set] = Element_Chunk(fillP);
            item[set].elementData(indices.data(), fillP);
        }
    }

    return;
}


/******************************************************************************/

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
