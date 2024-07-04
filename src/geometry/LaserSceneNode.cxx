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
#include "LaserSceneNode.h"

// system headers
#include <math.h>

// common implementation headers
#include "StateDatabase.h"
#include "BZDBCache.h"
#include "OpenGLAPI.h"
#include "VBO_Geometry.h"
#include "VBO_Drawing.h"

// FIXME (SceneRenderer.cxx is in src/bzflag)
#include "SceneRenderer.h"

const GLfloat       LaserRadius = 0.1f;

LaserSceneNode::LaserSceneNode(const glm::vec3 &pos, const glm::vec3 &forward) :
    texturing(false),
    renderNode(this)
{
    // prepare rendering info
    const float xy_dist = hypotf(forward.x, forward.y);
    azimuth = (float)(180.0 / M_PI*atan2f(forward[1], forward[0]));
    elevation = (float)(-180.0 / M_PI * atan2f(forward.z, xy_dist));
    length = hypotf(xy_dist, forward.z);

    // setup sphere
    setCenter(pos);
    setRadius(length * length);

    OpenGLGStateBuilder builder(gstate);
    builder.disableCulling();
    gstate = builder.getState();

    first = false;
    setColor(1,1,1);
    setCenterColor(1,1,1);
}

void LaserSceneNode::setColor(float r, float g, float b)
{
    color = glm::vec4(r, g, b, 1.0f);
}


void LaserSceneNode::setCenterColor(float r, float g, float b)
{
    centerColor = glm::vec4(r, g, b, 1.0f);
}

LaserSceneNode::~LaserSceneNode()
{
    // do nothing
}

void            LaserSceneNode::setTexture(const int texture)
{
    OpenGLGStateBuilder builder(gstate);
    builder.setTexture(texture);
    builder.enableTexture(texture>=0);
    gstate = builder.getState();
}

bool            LaserSceneNode::cull(const ViewFrustum&) const
{
    // no culling
    return false;
}

void            LaserSceneNode::notifyStyleChange()
{
    texturing = BZDBCache::texture;
    OpenGLGStateBuilder builder(gstate);
    builder.enableTexture(texturing);
    {
        // add in contribution from laser
        builder.setBlending(GL_SRC_ALPHA, GL_ONE);
        builder.setSmoothing(BZDB.isTrue("smooth"));
    }
    gstate = builder.getState();
}

void            LaserSceneNode::addRenderNodes(
    SceneRenderer& renderer)
{
    renderer.addRenderNode(&renderNode, &gstate);
}

//
// LaserSceneNode::LaserRenderNode
//

Vertex_Chunk LaserSceneNode::LaserRenderNode::sphere12;
Vertex_Chunk LaserSceneNode::LaserRenderNode::sphere32;
Vertex_Chunk LaserSceneNode::LaserRenderNode::laserChunk;

LaserSceneNode::LaserRenderNode::LaserRenderNode(
    const LaserSceneNode* _sceneNode) :
    sceneNode(_sceneNode)
{
    // initialize geometry if first instance
    static bool init = false;
    if (!init)
    {
        init = true;
        sphere12 = Quadric::buildSphere(0.5f, 12);
        sphere32 = Quadric::buildSphere(0.5f, 32);

        glm::vec3 v[6];
        glm::vec2 t[6];

        laserChunk = Vertex_Chunk(Vertex_Chunk::VT, 6);
        t[0] = glm::vec2(0.5f,  0.5f);
        v[0] = glm::vec3(0.0f,  0.0f,  0.0f);
        t[1] = glm::vec2(0.0f,  0.0f);
        v[1] = glm::vec3(0.0f,  0.0f,  1.0f);
        t[2] = t[1];
        v[2] = glm::vec3(0.0f,  1.0f,  0.0f);
        t[3] = t[1];
        v[3] = glm::vec3(0.0f,  0.0f, -1.0f);
        t[4] = t[1];
        v[4] = glm::vec3(0.0f, -1.0f,  0.0f);
        t[5] = t[1];
        v[5] = glm::vec3(0.0f,  0.0f,  1.0f);
        laserChunk.textureData(t);
        laserChunk.vertexData(v);
    }
}

LaserSceneNode::LaserRenderNode::~LaserRenderNode()
{
    // do nothing
}

const glm::vec3 &LaserSceneNode::LaserRenderNode::getPosition() const
{
    return sceneNode->getSphere();
}

void LaserSceneNode::LaserRenderNode::render()
{
    const bool blackFog = RENDERER.isFogActive();
    if (blackFog)
        glSetFogColor(glm::vec4(0.0f));

    glPushMatrix();
    const auto &sphere = getPosition();
    glTranslate(sphere);
    glRotatef(sceneNode->azimuth, 0.0f, 0.0f, 1.0f);
    glRotatef(sceneNode->elevation, 0.0f, 1.0f, 0.0f);
    if (RENDERER.useQuality() >= 3)
        renderGeoLaser();
    else
        renderFlatLaser();
    glPopMatrix();

    if (blackFog)
        glSetFogColor(RENDERER.getFogColor());
}

void LaserSceneNode::LaserRenderNode::renderGeoLaser()
{
    const float len = sceneNode->length;

    glDisable(GL_TEXTURE_2D);

    auto coreColor = sceneNode->centerColor;
    auto mainColor = sceneNode->color;

    coreColor.a     = 0.85f;
    mainColor.a     = 0.125f;

    if (!colorOverride)
        glColor(coreColor);
    glPushMatrix();
    glScalef(len, 0.0625f, 0.0625f);
    DRAWER.cylinderX10();
    glPopMatrix();
    addTriangleCount(20);

    if (!colorOverride)
        glColor(mainColor);
    glPushMatrix();
    glScalef(len, 0.1f, 0.1f);
    DRAWER.cylinderX16();
    glPopMatrix();
    addTriangleCount(32);

    glPushMatrix();
    glScalef(len, 0.2f, 0.2f);
    DRAWER.cylinderX24();
    glPopMatrix();
    addTriangleCount(48);

    glPushMatrix();
    glScalef(len, 0.4f, 0.4f);
    DRAWER.cylinderX32();
    glPopMatrix();
    addTriangleCount(64);

    if (sceneNode->first)
    {
        sphere32.draw(GL_TRIANGLE_STRIP);
        addTriangleCount(32 * 32 * 2);
    }
    else
    {
        sphere12.draw(GL_TRIANGLE_STRIP);
        addTriangleCount(12 * 12 * 2);
    }

    glEnable(GL_TEXTURE_2D);
}


void LaserSceneNode::LaserRenderNode::renderFlatLaser()
{
    const float len = sceneNode->length;

    if (sceneNode->texturing)
    {
        glScalef(len, 1.0f, 1.0f);
        if (!colorOverride)
            ::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        laserChunk.draw(GL_TRIANGLE_FAN);
        // 6 verts -> 4 tris

        DRAWER.diamondTexturedXZ();
        DRAWER.diamondTexturedXY();
        // 8 verts -> 4 tris

        addTriangleCount(8);
    }

    else
    {
        // draw beam
        if (!colorOverride)
            glColor4f(1.0f, 0.25f, 0.0f, 0.85f);
        glScalef(len, LaserRadius, LaserRadius);
        DRAWER.beam();
        // 14 verts -> 12 tris

        // also draw a line down the middle (so the beam is visible even
        // if very far away).  this will also give the beam an extra bright
        // center.
        DRAWER.asimmetricLineX();
        // count 1 line as 1 tri

        addTriangleCount(13);
    }
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
