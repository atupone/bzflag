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

// interface header
#include "FlagWarpSceneNode.h"

// system headers
#include <stdlib.h>
#include <math.h>
#include <glm/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

// common implementation headers
#include "StateDatabase.h"
#include "BZDBCache.h"
#include "OpenGLAPI.h"

// local implementation headers
#include "ViewFrustum.h"

// FIXME (SceneRenderer.cxx is in src/bzflag)
#include "SceneRenderer.h"

const float     FlagWarpSize =  7.5;        // meters
const glm::vec4 FlagWarpSceneNode::color[7] =
{
    { 0.25, 1.0,  0.25, 0.5f },
    { 0.25, 0.25, 1.0,  0.5f },
    { 1.0,  0.0,  1.0,  0.5f },
    { 1.0,  0.25, 0.25, 0.5f },
    { 1.0,  0.5,  0.0,  0.5f },
    { 1.0,  1.0,  0.0,  0.5f },
    { 1.0,  1.0,  1.0,  0.5f }
};

FlagWarpSceneNode::FlagWarpSceneNode(const glm::vec3 &pos) :
    renderNode(this)
{
    move(pos);
    setRadius(1.25f * FlagWarpSize * FlagWarpSize);
    size = 1.0f;
}

FlagWarpSceneNode::~FlagWarpSceneNode()
{
    // do nothing
}

void            FlagWarpSceneNode::setSizeFraction(GLfloat _size)
{
    size = _size;
}

void FlagWarpSceneNode::move(const glm::vec3 &pos)
{
    setCenter(pos);
}

GLfloat FlagWarpSceneNode::getDistance(const glm::vec3 &eye) const
{
    // shift position of warp down a little because a flag and it's warp
    // are at the same position but we want the warp to appear below the
    // flag.
    auto mySphere = getSphere();
    mySphere.z -= 0.2f;
    return glm::distance2(eye, mySphere);
}

void            FlagWarpSceneNode::notifyStyleChange()
{
    OpenGLGStateBuilder builder(gstate);
    {
        builder.setBlending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        builder.setStipple(1.0f);
    }
    gstate = builder.getState();
}

void            FlagWarpSceneNode::addRenderNodes(
    SceneRenderer& renderer)
{
    renderer.addRenderNode(&renderNode, &gstate);
}

//
// FlagWarpSceneNode::FlagWarpRenderNode
//

glm::vec3 FlagWarpSceneNode::FlagWarpRenderNode::ring[12];
Vertex_Chunk FlagWarpSceneNode::FlagWarpRenderNode::warp;

FlagWarpSceneNode::FlagWarpRenderNode::FlagWarpRenderNode(
    const FlagWarpSceneNode* _sceneNode) :
    sceneNode(_sceneNode)
{
    static bool init = true;
    if (init)
    {
        init = false;
        for (int i = 0; i < 12; i++)
        {
            ring[i][0] = cosf((float)(2.0 * M_PI * double(i) / 12.0));
            ring[i][1] = sinf((float)(2.0 * M_PI * double(i) / 12.0));
            ring[i][2] = 0.0f;
        }
        warp = Vertex_Chunk(Vertex_Chunk::V, 14);
    }
}

FlagWarpSceneNode::FlagWarpRenderNode::~FlagWarpRenderNode()
{
    // do nothing
}

const glm::vec3 &FlagWarpSceneNode::FlagWarpRenderNode::getPosition() const
{
    return sceneNode->getSphere();
}

void            FlagWarpSceneNode::FlagWarpRenderNode::render()
{
    // make a perturbed ring
    glm::vec3 geom[12];
    for (int i = 0; i < 12; i++)
        geom[i] = FlagWarpSize * glm::linearRand(0.9f, 1.1f) * ring[i];

    const auto &sphere = getPosition();
    glPushMatrix();
    glTranslate(sphere);

    float shft;

    glm::vec3 v[14];
    v[0] = glm::vec3(0.0f);
    v[1] = geom[0];
    if (sphere[2] > RENDERER.getViewFrustum().getEye()[2])
    {
        shft = -0.01f;
        v[2]  = geom[11];
        v[3]  = geom[10];
        v[4]  = geom[9];
        v[5]  = geom[8];
        v[6]  = geom[7];
        v[7]  = geom[6];
        v[8]  = geom[5];
        v[9]  = geom[4];
        v[10] = geom[3];
        v[11] = geom[2];
        v[12] = geom[1];
    }
    else
    {
        shft = 0.01f;
        v[2]  = geom[1];
        v[3]  = geom[2];
        v[4]  = geom[3];
        v[5]  = geom[4];
        v[6]  = geom[5];
        v[7]  = geom[6];
        v[8]  = geom[7];
        v[9]  = geom[8];
        v[10] = geom[9];
        v[11] = geom[10];
        v[12] = geom[11];
    }
    v[13] = geom[0];
    warp.vertexData(v);

    {
        float s    = sceneNode->size;
        float oldS = 1.0f;
        for (int i = 0; i < 7; i++)
        {
            if (s < 0.0f) break;
            if (!colorOverride)
                glColor(color[i]);
            float ratio = s / oldS;
            glScalef(ratio, ratio, ratio);
            warp.draw(GL_TRIANGLE_FAN);
            // 14 verts -> 12 tris
            addTriangleCount(12);
            glTranslatef(0.0f, 0.0f, shft / s);
            oldS = s;
            s   -= 0.05f;
        }
    }

    glPopMatrix();
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
