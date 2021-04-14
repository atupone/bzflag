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
#include "VBO_Drawing.h"
#include "PlayingShader.h"

// FIXME (SceneRenderer.cxx is in src/bzflag)
#include "SceneRenderer.h"

const GLfloat       LaserRadius = 0.1f;

LaserSceneNode::LaserSceneNode(const GLfloat pos[3], const GLfloat forward[3]) :
    texturing(false),
    renderNode(this)
{
    // prepare rendering info
    azimuth = (float)(180.0 / M_PI*atan2f(forward[1], forward[0]));
    elevation = (float)(-180.0 / M_PI*atan2f(forward[2], hypotf(forward[0],forward[1])));
    length = hypotf(forward[0], hypotf(forward[1], forward[2]));

    // setup sphere
    setCenter(glm::make_vec3(pos));
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
    // add in contribution from laser
    builder.setBlending(GL_SRC_ALPHA, GL_ONE);
    builder.setSmoothing(BZDB.isTrue("smooth"));
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

Vertex_Chunk LaserSceneNode::LaserRenderNode::laserTexture1;
Vertex_Chunk LaserSceneNode::LaserRenderNode::laserTexture2;
Vertex_Chunk LaserSceneNode::LaserRenderNode::laserNoTexture1;

LaserSceneNode::LaserRenderNode::LaserRenderNode(
    const LaserSceneNode* _sceneNode) :
    sceneNode(_sceneNode)
{
    // initialize geometry if first instance
    static bool init = false;
    if (!init)
    {
        init = true;
        glm::vec2 geom[6];
        for (int i = 0; i < 6; i++)
        {
            geom[i][0] = -LaserRadius * cosf((float)(2.0 * M_PI * double(i) / 6.0));
            geom[i][1] =  LaserRadius * sinf((float)(2.0 * M_PI * double(i) / 6.0));
        }
        glm::vec3 v[14];
        glm::vec2 t[10];
        v[ 0] = glm::vec3(0.0f, geom[0]);
        v[ 1] = glm::vec3(1.0f, geom[0]);
        v[ 2] = glm::vec3(0.0f, geom[1]);
        v[ 3] = glm::vec3(1.0f, geom[1]);
        v[ 4] = glm::vec3(0.0f, geom[2]);
        v[ 5] = glm::vec3(1.0f, geom[2]);
        v[ 6] = glm::vec3(0.0f, geom[3]);
        v[ 7] = glm::vec3(1.0f, geom[3]);
        v[ 8] = glm::vec3(0.0f, geom[4]);
        v[ 9] = glm::vec3(1.0f, geom[4]);
        v[10] = glm::vec3(0.0f, geom[5]);
        v[11] = glm::vec3(1.0f, geom[5]);
        v[12] = glm::vec3(0.0f, geom[0]);
        v[13] = glm::vec3(1.0f, geom[0]);
        laserNoTexture1 = Vertex_Chunk(Vertex_Chunk::V, 14);
        laserNoTexture1.vertexData(v);

        t[0] = glm::vec2(0.5f,  0.5f);
        v[0] = glm::vec3(  0.0f,  0.0f,  0.0f);
        t[1] = glm::vec2(0.0f,  0.0f);
        v[1] = glm::vec3(  0.0f,  0.0f,  1.0f);
        t[2] = glm::vec2(0.0f,  0.0f);
        v[2] = glm::vec3(  0.0f,  1.0f,  0.0f);
        t[3] = glm::vec2(0.0f,  0.0f);
        v[3] = glm::vec3(  0.0f,  0.0f, -1.0f);
        t[4] = glm::vec2(0.0f,  0.0f);
        v[4] = glm::vec3(  0.0f, -1.0f,  0.0f);
        t[5] = glm::vec2(0.0f,  0.0f);
        v[5] = glm::vec3(  0.0f,  0.0f,  1.0f);
        laserTexture1 = Vertex_Chunk(Vertex_Chunk::VT, 6);
        laserTexture1.textureData(t);
        laserTexture1.vertexData(v);

        t[0] = glm::vec2(0.0f,  0.0f);
        v[0] = glm::vec3(0.0f,  0.0f,  1.0f);
        t[1] = glm::vec2(0.0f,  1.0f);
        v[1] = glm::vec3(1.0f,  0.0f,  1.0f);
        t[2] = glm::vec2(1.0f,  0.0f);
        v[2] = glm::vec3(0.0f,  0.0f, -1.0f);
        t[3] = glm::vec2(1.0f,  1.0f);
        v[3] = glm::vec3(1.0f,  0.0f, -1.0f);

        // Degenerate triangles
        t[4] = glm::vec2(1.0f,  1.0f);
        v[4] = glm::vec3(1.0f,  0.0f, -1.0f);
        t[5] = glm::vec2(0.0f,  0.0f);
        v[5] = glm::vec3(0.0f,  1.0f,  0.0f);

        t[6] = glm::vec2(0.0f,  0.0f);
        v[6] = glm::vec3(0.0f,  1.0f,  0.0f);
        t[7] = glm::vec2(0.0f,  1.0f);
        v[7] = glm::vec3(1.0f,  1.0f,  0.0f);
        t[8] = glm::vec2(1.0f,  0.0f);
        v[8] = glm::vec3(0.0f, -1.0f,  0.0f);
        t[9] = glm::vec2(1.0f,  1.0f);
        v[9] = glm::vec3(1.0f, -1.0f,  0.0f);
        laserTexture2 = Vertex_Chunk(Vertex_Chunk::VT, 10);
        laserTexture2.textureData(t);
        laserTexture2.vertexData(v);
    }
}

LaserSceneNode::LaserRenderNode::~LaserRenderNode()
{
    // do nothing
}

void LaserSceneNode::LaserRenderNode::render()
{
    const bool blackFog = RENDERER.isFogActive();
    if (blackFog)
        glFogfv(GL_FOG_COLOR, glm::value_ptr(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)));

    glPushMatrix();
    const auto &sphere = sceneNode->getCenter();
    glTranslatef(sphere[0], sphere[1], sphere[2]);
    glRotatef(sceneNode->azimuth, 0.0f, 0.0f, 1.0f);
    glRotatef(sceneNode->elevation, 0.0f, 1.0f, 0.0f);
    if (RENDERER.useQuality() >= 3)
        renderGeoLaser();
    else
        renderFlatLaser();
    glPopMatrix();

    if (blackFog)
        glFogfv(GL_FOG_COLOR, glm::value_ptr(RENDERER.getFogColor()));
}


const glm::vec3 LaserSceneNode::LaserRenderNode::getPosition() const
{
    return sceneNode->getCenter();
}

void LaserSceneNode::LaserRenderNode::renderGeoLaser()
{
    const float len = sceneNode->length;
    glRotatef(90, 0.0f, 1.0f, 0.0f);

    SHADER.setTexturing(false);

    glm::vec4 coreColor = sceneNode->centerColor;
    coreColor.a     = 0.85f;
    glm::vec4 mainColor = sceneNode->color;
    mainColor.a     = 0.125f;

    if (!colorOverride)
        glColor4f(coreColor.r, coreColor.g, coreColor.b, coreColor.a);
    glPushMatrix();
    glScalef(0.0625f, 0.0625f, len);
    DRAWER.cylinder10();
    glPopMatrix();
    addTriangleCount(20);

    if (!colorOverride)
        glColor4f(mainColor.r, mainColor.g, mainColor.b, mainColor.a);
    glPushMatrix();
    glScalef(0.1f, 0.1f, len);
    DRAWER.cylinder16();
    glPopMatrix();
    addTriangleCount(32);

    glPushMatrix();
    glScalef(0.2f, 0.2f, len);
    DRAWER.cylinder24();
    glPopMatrix();
    addTriangleCount(48);

    glPushMatrix();
    glScalef(0.4f, 0.4f, len);
    DRAWER.cylinder32();
    glPopMatrix();
    addTriangleCount(64);

    glPushMatrix();
    glScalef(0.5f, 0.5f, 0.5f);
    if (sceneNode->first)
    {
        DRAWER.sphere(32);
        addTriangleCount(32 * 32 * 2);
    }
    else
    {
        DRAWER.sphere(12);
        addTriangleCount(12 * 12 * 2);
    }
    glPopMatrix();

    SHADER.setTexturing(true);
}


void LaserSceneNode::LaserRenderNode::renderFlatLaser()
{
    const float len = sceneNode->length;
    glScalef(len, 1.0f, 1.0f);

    if (sceneNode->texturing)
    {
        if (!colorOverride)
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        laserTexture1.draw(GL_TRIANGLE_FAN);
        // 6 verts -> 4 tris
        laserTexture2.draw(GL_TRIANGLE_STRIP);
        // 8 verts -> 4 tris

        addTriangleCount(8);
    }

    else
    {
        // draw beam
        if (!colorOverride)
            glColor4f(1.0f, 0.25f, 0.0f, 0.85f);

        laserNoTexture1.draw(GL_TRIANGLE_STRIP);
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
