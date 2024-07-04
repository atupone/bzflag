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
#include "SphereSceneNode.h"

// system headers
#include <math.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

// common implementation headers
#include "SceneRenderer.h"
#include "StateDatabase.h"
#include "BZDBCache.h"
#include "OpenGLMaterial.h"
#include "TextureManager.h"
#include "VBO_Drawing.h"
#include "OpenGLAPI.h"

// local implementation headers
#include "ViewFrustum.h"


/******************************************************************************/

//
// SphereSceneNode
//

SphereSceneNode::SphereSceneNode(const glm::vec3 &pos, GLfloat _radius)
{
    transparent = false;

    OpenGLGStateBuilder builder(gstate);
    builder.disableCulling();
    gstate = builder.getState();

    setColor(1.0f, 1.0f, 1.0f, 1.0f);

    // position sphere
    move(pos, _radius);

    return;
}


SphereSceneNode::~SphereSceneNode()
{
    // do nothing
}


void SphereSceneNode::setColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    color = glm::vec4(r, g, b, a);
    transparent = (color[3] != 1.0f);
}


void SphereSceneNode::setColor(const glm::vec4 &rgba)
{
    color = rgba;
    transparent = (color[3] != 1.0f);
}


void SphereSceneNode::move(const glm::vec3 &pos, GLfloat _radius)
{
    radius = _radius;
    setCenter(pos);
    setRadius(radius * radius);
}


void SphereSceneNode::notifyStyleChange()
{
    OpenGLGStateBuilder builder(gstate);
    if (transparent)
    {
        {
            builder.setBlending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            builder.setStipple(1.0f);
            builder.setNeedsSorting(true);
        }
    }
    else
    {
        builder.resetBlending();
        builder.setStipple(1.0f);
        builder.setNeedsSorting(false);
    }
    gstate = builder.getState();
}


/******************************************************************************/

//
// SphereLodSceneNode
//


float SphereLodSceneNode::lodPixelsSqr[sphereLods];
int SphereLodSceneNode::listTriangleCount[sphereLods];

static int calcTriCount(int slices, int stacks)
{
    const int trifans = 2 * slices;
    const int quads = 2 * (slices * (stacks - 2));
    return (trifans + quads);
}

SphereLodSceneNode::SphereLodSceneNode(const glm::vec3 &pos, GLfloat _radius) :
    SphereSceneNode(pos, _radius),
    renderNode(this)
{

    lodPixelsSqr[0] = 80.0f * 80.0f;
    listTriangleCount[0] = calcTriCount(32, 32);

    lodPixelsSqr[1] = 40.0f * 40.0f;
    listTriangleCount[1] = calcTriCount(16, 16);

    lodPixelsSqr[2] = 20.0f * 20.0f;
    listTriangleCount[2] = calcTriCount(8, 8);

    lodPixelsSqr[3] = 10.0f * 10.0f;
    listTriangleCount[3] = calcTriCount(6, 6);

    lodPixelsSqr[4] = 5.0f * 5.0f;
    listTriangleCount[4] = calcTriCount(4, 4);

    OpenGLGStateBuilder builder(gstate);
    builder.disableCulling();
    gstate = builder.getState();

    setColor(1.0f, 1.0f, 1.0f, 1.0f);

    // position sphere
    move(pos, _radius);

    inside = false;
    shockWave = false;

    renderNode.setLod(0);

    // adjust the gstate for this type of sphere
    builder.setCulling(GL_BACK);
    builder.setShading(GL_SMOOTH);
    const auto spec = glm::vec3(1.0f);
    const auto emis = glm::vec3(0.0f);
    OpenGLMaterial glmat(spec, emis, 64.0f);
    builder.setMaterial(glmat);
    gstate = builder.getState();
    return;
}


SphereLodSceneNode::~SphereLodSceneNode()
{
    return;
}


void SphereLodSceneNode::setShockWave(bool value)
{
    shockWave = value;
    if (BZDBCache::texture && false)   //FIXME
    {
        OpenGLGStateBuilder builder(gstate);
        TextureManager &tm = TextureManager::instance();
        int texId = tm.getTextureID("mesh");
        builder.setTexture(texId);
        gstate = builder.getState();
    }
    return;
}


void SphereLodSceneNode::addRenderNodes(SceneRenderer& renderer)
{
    const ViewFrustum& view = renderer.getViewFrustum();
    const auto &s = getSphere();
    const auto &e = view.getEye();

    float distSqr = glm::distance2(e, s);
    if (distSqr <= 0.0f)
        distSqr = 1.0e-6f;

    const float lpp = renderer.getLengthPerPixel();
    float ppl;
    if (lpp <= 0.0f)
        ppl = +MAXFLOAT;
    else
        ppl = 1.0f / lpp;
    const float r = getRadius2();
    const float pixelsSqr = (r * ppl * ppl) / distSqr;

    int lod;
    for (lod = 0; lod < (sphereLods - 1); lod++)
    {
        if (lodPixelsSqr[lod] < pixelsSqr)
            break;
    }
    renderNode.setLod(lod);

    inside = (distSqr < r);

    renderer.addRenderNode(&renderNode, &gstate);

    return;
}


void SphereLodSceneNode::addShadowNodes(SceneRenderer&)
{
    return;
}


//
// SphereLodSceneNode::SphereLodRenderNode
//

SphereLodSceneNode::SphereLodRenderNode::SphereLodRenderNode(
    const SphereLodSceneNode* _sceneNode) :
    sceneNode(_sceneNode)
{
    return;
}


SphereLodSceneNode::SphereLodRenderNode::~SphereLodRenderNode()
{
    return;
}

const glm::vec3 &SphereLodSceneNode::SphereLodRenderNode::getPosition() const
{
    return sceneNode->getSphere();
}

void SphereLodSceneNode::SphereLodRenderNode::setLod(int _lod)
{
    lod = _lod;
    return;
}


static inline void drawFullScreenRect()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    DRAWER.simmetricRect();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    return;
}


void SphereLodSceneNode::SphereLodRenderNode::render()
{
    const GLfloat radius = sceneNode->radius;
    const auto &sphere = getPosition();

    glEnable(GL_CLIP_PLANE0);

    glEnable(GL_RESCALE_NORMAL);

    const bool transparent = sceneNode->transparent;

    const int lod2slices[] = {32, 16, 8, 6, 4};

    const int slices = lod2slices[lod];

    glPushMatrix();
    {
        glTranslatef(sphere[0], sphere[1], sphere[2]);
        glScalef(radius, radius, radius);

        // invert the color within contained volume
        if (sceneNode->shockWave)
        {
            if (transparent)
            {
                {
                    glDisable(GL_BLEND);
                }
            }
            glDisable(GL_LIGHTING);

            glLogicOp(GL_INVERT);
            glEnable(GL_COLOR_LOGIC_OP);
            {
                glCullFace(GL_FRONT);
                DRAWER.sphere(slices);
                addTriangleCount(listTriangleCount[lod]);
                glCullFace(GL_BACK);
                if (!sceneNode->inside)
                {
                    DRAWER.sphere(slices);
                    addTriangleCount(listTriangleCount[lod]);
                }
                else
                {
                    drawFullScreenRect();
                    addTriangleCount(2);
                }
            }
            glDisable(GL_COLOR_LOGIC_OP);

            if (transparent)
            {
                {
                    glEnable(GL_BLEND);
                }
            }
            glEnable(GL_LIGHTING);
        }

        // draw the surface
        if (!colorOverride)
            glColor(sceneNode->color);
        {
            glCullFace(GL_FRONT);
            DRAWER.sphere(slices);
            addTriangleCount(listTriangleCount[lod]);
        }
        glCullFace(GL_BACK);
        if (!sceneNode->inside)
        {
            DRAWER.sphere(slices);
            addTriangleCount(listTriangleCount[lod]);
        }
        else
        {
            glDisable(GL_LIGHTING);
            drawFullScreenRect();
            glEnable(GL_LIGHTING);
            addTriangleCount(2);
        }
    }
    glPopMatrix();

    glDisable(GL_RESCALE_NORMAL);

    glDisable(GL_CLIP_PLANE0);

    return;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
