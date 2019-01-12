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

#define GLM_ENABLE_EXPERIMENTAL

// interface header
#include "MeshPolySceneNode.h"

// system headers
#include <glm/gtx/norm.hpp>

// common implementation headers
#include "Intersect.h"

// FIXME (SceneRenderer.cxx is in src/bzflag)
#include "SceneRenderer.h"

// FIXME - no tesselation is done on for shot lighting


//
// MeshPolySceneNode::Geometry
//

MeshPolySceneNode::Geometry::Geometry(
    MeshPolySceneNode* _node,
    const std::vector<glm::vec3> &_vertices,
    const std::vector<glm::vec3> &_normals,
    const std::vector<glm::vec2> &_texcoords,
    const glm::vec3 *_normal) :
    vertices(_vertices), normals(_normals), texcoords(_texcoords)
{
    if (normals.empty())
        vboIndex = Vertex_Chunk(Vertex_Chunk::VT, vertices.size());
    else
        vboIndex = Vertex_Chunk(Vertex_Chunk::VTN, vertices.size());
    sceneNode = _node;
    normal = _normal;
    style = 0;
    vboIndex.textureData(texcoords);
    vboIndex.normalData(normals);
    vboIndex.vertexData(vertices);
    return;
}



void MeshPolySceneNode::Geometry::render()
{
    sceneNode->setColor();

    if (normals.empty())
        glNormal3f(normal->x, normal->y, normal->z);

    vboIndex.enableArrays(style >= 2, true, false);
    vboIndex.glDrawArrays(GL_TRIANGLE_FAN);

    addTriangleCount(vertices.size() - 2);
    return;
}


void MeshPolySceneNode::Geometry::renderShadow()
{
    vboIndex.draw(GL_TRIANGLE_FAN, true);
    addTriangleCount(vertices.size() - 2);
    return;
}


const glm::vec3 MeshPolySceneNode::Geometry::getPosition() const
{
    return sceneNode->getCenter();
}


//
// MeshPolySceneNode
//

MeshPolySceneNode::MeshPolySceneNode(const glm::vec4 &_plane,
                                     bool _noRadar, bool _noShadow,
                                     const std::vector<glm::vec3> &vertices,
                                     const std::vector<glm::vec3> &normals,
                                     const std::vector<glm::vec2> &texcoords) :
    node(this, vertices, normals, texcoords, &normal)
{
    int i, j;
    const int count = vertices.size();

    setPlane(_plane);

    noRadar = _noRadar || (plane[2] <= 0.0f); // pre-cull if we can
    noShadow = _noShadow;

    // choose axis to ignore (the one with the largest normal component)
    int ignoreAxis;
    if (fabsf(normal[0]) > fabsf(normal[1]))
    {
        if (fabsf(normal[0]) > fabsf(normal[2]))
            ignoreAxis = 0;
        else
            ignoreAxis = 2;
    }
    else
    {
        if (fabsf(normal[1]) > fabsf(normal[2]))
            ignoreAxis = 1;
        else
            ignoreAxis = 2;
    }

    // project vertices onto plane
    std::vector<glm::vec2> flat(count);
    switch (ignoreAxis)
    {
    case 0:
        for (i = 0; i < count; i++)
        {
            flat[i][0] = vertices[i][1];
            flat[i][1] = vertices[i][2];
        }
        break;
    case 1:
        for (i = 0; i < count; i++)
        {
            flat[i][0] = vertices[i][2];
            flat[i][1] = vertices[i][0];
        }
        break;
    case 2:
        for (i = 0; i < count; i++)
        {
            flat[i][0] = vertices[i][0];
            flat[i][1] = vertices[i][1];
        }
        break;
    }

    // compute area of polygon
    float* area = new float[1];
    area[0] = 0.0f;
    for (j = count - 1, i = 0; i < count; j = i, i++)
        area[0] += flat[j][0] * flat[i][1] - flat[j][1] * flat[i][0];
    area[0] = 0.5f * fabsf(area[0]) / normal[ignoreAxis];

    // set lod info
    setNumLODs(1, area);

    // compute bounding sphere, put center at average of vertices
    auto myCenter = glm::vec3(0.0f);
    for (const auto &v : vertices)
        myCenter += v;
    myCenter /= (float)count;
    setCenter(myCenter);

    float myRadius2 = 0.0f;
    for (const auto &v : vertices)
    {
        const auto d = myCenter - v;
        auto r = glm::length2(d);
        if (r > myRadius2)
            myRadius2 = r;
    }
    setRadius(myRadius2);

    // record extents info
    for (const auto &v : vertices)
        extents.expandToPoint(v);

    return;
}


MeshPolySceneNode::~MeshPolySceneNode()
{
    return;
}


int MeshPolySceneNode::getVertexCount () const
{
    return node.getVertexCount();
}


bool MeshPolySceneNode::cull(const ViewFrustum& frustum) const
{
    // cull if eye is behind (or on) plane
    const auto eye = frustum.getEye();
    if ((glm::dot(eye, normal) + plane[3]) <= 0.0f)
        return true;

    // if the Visibility culler tells us that we're
    // fully visible, then skip the rest of these tests
    if (octreeState == OctreeVisible)
        return false;

    const Frustum* f = (const Frustum *) &frustum;
    if (testAxisBoxInFrustum(extents, f) == Outside)
        return true;

    // probably visible
    return false;
}


bool MeshPolySceneNode::inAxisBox (const Extents& exts) const
{
    if (!extents.touches(exts))
        return false;

    return testPolygonInAxisBox (node.vertices, plane, exts);
}


const glm::vec3 MeshPolySceneNode::getVertex(int i) const
{
    return node.getVertex(i);
}


void MeshPolySceneNode::addRenderNodes(SceneRenderer& renderer)
{
    node.setStyle(getStyle());
    const glm::vec4 *dyncol = getDynamicColor();
    if ((dyncol == NULL) || (dyncol->a != 0.0f))
        renderer.addRenderNode(&node, getWallGState());
    return;
}


void MeshPolySceneNode::addShadowNodes(SceneRenderer& renderer)
{
    if (!noShadow)
    {
        const glm::vec4 *dyncol = getDynamicColor();
        if ((dyncol == NULL) || (dyncol->a != 0.0f))
            renderer.addShadowNode(&node);
    }
    return;
}


void MeshPolySceneNode::renderRadar()
{
    if (!noRadar)
        node.renderRadar();
    return;
}


void MeshPolySceneNode::getRenderNodes(std::vector<RenderSet>& rnodes)
{
    RenderSet rs = { &node, getWallGState() };
    rnodes.push_back(rs);
    return;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
