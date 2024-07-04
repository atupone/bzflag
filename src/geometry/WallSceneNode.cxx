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

// interface headers
#include "WallSceneNode.h"

// system headers
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

// common implementation headers
#include "StateDatabase.h"
#include "BZDBCache.h"
#include "mathRoutine.h"
#include "OpenGLAPI.h"

// local implementation headers
#include "ViewFrustum.h"

// FIXME (SceneRenderer.cxx is in src/bzflag)
#include "SceneRenderer.h"

WallSceneNode::WallSceneNode() : numLODs(0),
    elementAreas(NULL),
    style(0)
{
    dynamicColor = NULL;
    setColor(1.0f, 1.0f, 1.0f);
    setModulateColor(1.0f, 1.0f, 1.0f);
    setLightedColor(1.0f, 1.0f, 1.0f);
    setLightedModulateColor(1.0f, 1.0f, 1.0f);
    useColorTexture = false;
    noCulling = false;
    noSorting = false;
    isBlended = false;
    wantBlending = false;
    wantSphereMap = false;
    alphaThreshold = 0.0f;
    return;
}

WallSceneNode::~WallSceneNode()
{
    // free element area table
    delete[] elementAreas;
}

const glm::vec4 *WallSceneNode::getPlane() const
{
    return &plane;
}

void            WallSceneNode::setNumLODs(int num, float* areas)
{
    numLODs = num;
    elementAreas = areas;
}

void WallSceneNode::setPlane(const glm::vec4 &_plane)
{
    const float n = bzInverseSqrt(glm::length2(glm::vec3(_plane)));

    // store normalized plane equation
    plane = n * _plane;
}

bool            WallSceneNode::cull(const ViewFrustum& frustum) const
{
    // cull if eye is behind (or on) plane
    const auto eye = glm::vec4(frustum.getEye(), 1.0f);
    const float eyedot = glm::dot(eye, plane);
    if (eyedot <= 0.0f)
        return true;

    // if the Visibility culler tells us that we're
    // fully visible, then skip the rest of these tests
    if (octreeState == OctreeVisible)
        return false;

    // get signed distance of wall center to each frustum side.
    // if more than radius outside then cull
    const int planeCount = frustum.getPlaneCount();
    int i;
    float d[6], d2[6];
    const auto mySphere = glm::vec4(getSphere(), 1.0f);
    const float myRadius = getRadius2();
    bool inside = true;
    for (i = 0; i < planeCount; i++)
    {
        const auto &norm = frustum.getSide(i);
        d[i] = glm::dot(mySphere, norm);
        if (d[i] < 0.0f)
        {
            d2[i] = d[i] * d[i];
            if (d2[i] > myRadius)
                return true;
            inside = false;
        }
    }

    // see if center of wall is inside each frustum side
    if (inside)
        return false;

    // most complicated test:  for sides sphere is behind, see if
    // center is beyond radius times the sine of the angle between
    // the normals, or equivalently:
    //    distance^2 > radius^2 * (1 - cos^2)
    // if so the wall is outside the view frustum
    for (i = 0; i < planeCount; i++)
    {
        if (d[i] >= 0.0f)
            continue;
        const auto norm = glm::vec3(frustum.getSide(i));
        const GLfloat c = glm::dot(norm, glm::vec3(plane));
        if (d2[i] > myRadius * (1.0f - c*c))
            return true;
    }

    // probably visible
    return false;
}

int         WallSceneNode::pickLevelOfDetail(
    const SceneRenderer& renderer) const
{
    if (!BZDBCache::tessellation)
        return 0;

    int bestLOD = 0;

    const auto &mySphere = getSphere();
    const float myRadius = getRadius2();
    const int numLights = renderer.getNumLights();
    for (int i = 0; i < numLights; i++)
    {
        auto pos = renderer.getLight(i).getPosition();
        pos.w = 1.0f;

        // get signed distance from plane
        GLfloat pd = glm::dot(pos, plane);

        // ignore if behind wall
        if (pd < 0.0f) continue;

        // get squared distance from center of wall
        GLfloat ld = glm::distance2(glm::vec3(pos), mySphere);

        // pick representative distance
        GLfloat d = (ld > 1.5f * myRadius) ? ld : pd * pd;

        // choose lod based on distance and element areas;
        int j;
        for (j = 0; j < numLODs - 1; j++)
            if (elementAreas[j] < d)
                break;

        // use new lod if more detailed
        if (j > bestLOD) bestLOD = j;
    }

    // FIXME -- if transient texture warper is active then possibly
    // bump up LOD if view point is close to wall.

    // limit lod to maximum allowed
    if (bestLOD > BZDBCache::maxLOD) bestLOD = (int)BZDBCache::maxLOD;

    // return highest level required -- note that we don't care about
    // the view point because, being flat, the wall would always
    // choose the lowest LOD for any view.
    return bestLOD;
}

GLfloat WallSceneNode::getDistance(const glm::vec3 &eye) const
{
    const auto myEye = glm::vec4(eye, 1.0f);
    const GLfloat d = glm::dot(plane, myEye);
    return d * d;
}

void            WallSceneNode::setColor(
    GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    color = glm::vec4(r, g, b, a);
}

void WallSceneNode::setDynamicColor(const glm::vec4 *rgba)
{
    dynamicColor = rgba;
    return;
}

void            WallSceneNode::setBlending(bool blend)
{
    wantBlending = blend;
    return;
}

void            WallSceneNode::setSphereMap(bool sphereMapping)
{
    wantSphereMap = sphereMapping;
    return;
}

void WallSceneNode::setColor(const glm::vec4 &rgba)
{
    color = rgba;
}

void            WallSceneNode::setModulateColor(
    GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    modulateColor = glm::vec4(r, g, b, a);
}

void WallSceneNode::setModulateColor(const glm::vec4 &rgba)
{
    modulateColor = rgba;
}

void            WallSceneNode::setLightedColor(
    GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    lightedColor = glm::vec4(r, g, b, a);
}

void WallSceneNode::setLightedColor(const glm::vec4 &rgba)
{
    lightedColor = rgba;
}

void            WallSceneNode::setLightedModulateColor(
    GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    lightedModulateColor = glm::vec4(r, g, b, a);
}

void WallSceneNode::setLightedModulateColor(
    const glm::vec4 &rgba)
{
    lightedModulateColor = rgba;
}

void            WallSceneNode::setAlphaThreshold(float thresh)
{
    alphaThreshold = thresh;
}

void            WallSceneNode::setNoCulling(bool value)
{
    noCulling = value;
}

void            WallSceneNode::setNoSorting(bool value)
{
    noSorting = value;
}

void            WallSceneNode::setMaterial(const OpenGLMaterial& mat)
{
    OpenGLGStateBuilder builder(gstate);
    builder.setMaterial(mat);
    gstate = builder.getState();
}

void            WallSceneNode::setTexture(const int tex)
{
    OpenGLGStateBuilder builder(gstate);
    builder.setTexture(tex);
    gstate = builder.getState();
}

void            WallSceneNode::setTextureMatrix(const GLfloat* texmat)
{
    OpenGLGStateBuilder builder(gstate);
    builder.setTextureMatrix(texmat);
    gstate = builder.getState();
}

void            WallSceneNode::notifyStyleChange()
{
    float alpha;
    bool lighted = (BZDBCache::lighting && gstate.isLighted());
    OpenGLGStateBuilder builder(gstate);
    style = 0;
    if (lighted)
    {
        style += 1;
        builder.setShading();
    }
    else
        builder.setShading(GL_FLAT);
    if (BZDBCache::texture && gstate.isTextured())
    {
        style += 2;
        builder.enableTexture(true);
        builder.enableTextureMatrix(true);
        alpha = lighted ? lightedModulateColor[3] : modulateColor[3];
    }
    else
    {
        builder.enableTexture(false);
        builder.enableTextureMatrix(false);
        alpha = lighted ? lightedColor[3] : color[3];
    }
    builder.enableMaterial(lighted);
    if (wantBlending || (alpha != 1.0f))
    {
        builder.setBlending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        builder.setStipple(1.0f);
    }
    else
    {
        builder.resetBlending();
        builder.setStipple(alpha);
    }
    isBlended = wantBlending || (alpha != 1.0f);
    if (alphaThreshold != 0.0f)
        builder.setAlphaFunc(GL_GEQUAL, alphaThreshold);
    if (noCulling)
        builder.disableCulling();
    if (noSorting)
        builder.setNeedsSorting(false);
    if (wantSphereMap)
        builder.enableSphereMap(true);
    gstate = builder.getState();
}

void            WallSceneNode::copyStyle(WallSceneNode* node)
{
    gstate = node->gstate;
    useColorTexture = node->useColorTexture;
    dynamicColor = node->dynamicColor;
    setColor(node->color);
    setModulateColor(node->modulateColor);
    setLightedColor(node->lightedColor);
    setLightedModulateColor(node->lightedModulateColor);
    isBlended = node->isBlended;
    wantBlending = node->wantBlending;
    wantSphereMap = node->wantSphereMap;
}

void            WallSceneNode::setColor()
{
    if (colorOverride)
        return;

    if (BZDBCache::texture && useColorTexture)
        glColor(glm::vec4(1.0f));
    else if (dynamicColor != NULL)
        glColor(*dynamicColor);
    else
    {
        switch (style)
        {
        case 0:
            glColor(color);
            break;
        case 1:
            glColor(lightedColor);
            break;
        case 2:
            glColor(modulateColor);
            break;
        case 3:
            glColor(lightedModulateColor);
            break;
        }
    }
}

bool WallSceneNode::inAxisBox (const Extents& UNUSED(exts)) const
{
    // this should never happen, only the TriWallSceneNode
    // and QuadWallSceneNode version of this function will
    // be called
    printf ("WallSceneNode::inAxisBox() was called!\n");
    exit (1);
    return false;
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
