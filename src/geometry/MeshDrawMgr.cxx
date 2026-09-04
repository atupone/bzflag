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

// common headers
#include "bzfgl.h"
#include "MeshDrawInfo.h"


inline void MeshDrawMgr::rawExecuteCommands(int lod, int set)
{
    auto drawLods = drawInfo->getDrawLods();
    const DrawLod& drawLod = drawLods[lod];
    const DrawSet& drawSet = drawLod.sets[set];
    const int cmdCount = drawSet.count;
    for (int i = 0; i < cmdCount; i++)
    {
        const DrawCmd& cmd = drawSet.cmds[i];
        glDrawElements(cmd.drawMode, cmd.count, cmd.indexType, cmd.indices);
    }
    return;
}


void MeshDrawMgr::executeSet(int lod, int set, bool useNormals, bool useTexcoords)
{
    const AnimationInfo* animInfo = drawInfo->getAnimationInfo();
    if (animInfo != nullptr)
    {
        glPushMatrix();
        glRotatef(animInfo->angle, 0.0f, 0.0f, 1.0f);
    }

    auto vertices  = reinterpret_cast<GLfloat const*>(drawInfo->getVertices());
    auto normals   = reinterpret_cast<GLfloat const*>(drawInfo->getNormals());
    auto texcoords = reinterpret_cast<GLfloat const*>(drawInfo->getTexcoords());

    glVertexPointer(3, GL_FLOAT, 0, vertices);

    if (useNormals)
        glNormalPointer(GL_FLOAT, 0, normals);
    else
        glDisableClientState(GL_NORMAL_ARRAY);
    if (useTexcoords)
        glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    else
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    rawExecuteCommands(lod, set);

    if (!useNormals)
        glEnableClientState(GL_NORMAL_ARRAY);
    if (!useTexcoords)
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    if (animInfo != nullptr)
        glPopMatrix();

    return;
}


void MeshDrawMgr::executeSetGeometry(int lod, int set)
{
    const AnimationInfo* animInfo = drawInfo->getAnimationInfo();
    if (animInfo != NULL)
    {
        glPushMatrix();
        glRotatef(animInfo->angle, 0.0f, 0.0f, 1.0f);
    }

    auto vertices = reinterpret_cast<GLfloat const *>(drawInfo->getVertices());

    glVertexPointer(3, GL_FLOAT, 0, vertices);
    rawExecuteCommands(lod, set);

    if (animInfo != NULL)
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
