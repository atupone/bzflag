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
#include "QuadWallSceneNode.h"

// system headers
#include <math.h>
#include <glm/gtx/norm.hpp>
#include <algorithm>

// common implementation headers
#include "Intersect.h"
#include "StateDatabase.h"

// local implementation headers
#include "ViewFrustum.h"

// FIXME (SceneRenderer.cxx is in src/bzflag)
#include "SceneRenderer.h"

//
// QuadWallSceneNode::Geometry
//

QuadWallSceneNode::Geometry::Geometry(QuadWallSceneNode* _wall,
                                      int uCount, int vCount,
                                      const glm::vec3 &base,
                                      const glm::vec3 &uEdge,
                                      const glm::vec3 &vEdge,
                                      const glm::vec3 &_normal,
                                      float uOffset, float vOffset,
                                      float uRepeats, float vRepeats, bool fixedUVs) :
    wall(_wall),
    style(0),
    normal(_normal),
    vboIndexS(Vertex_Chunk::V, 4),
    vertex((uCount+1) * (vCount+1))
{
    const int ds = uCount;
    const int dt = vCount;
    std::vector<glm::vec2> uv((uCount+1) * (vCount+1));
    std::vector<glm::vec3> myVertex;
    std::vector<glm::vec2> myTextur;
    glm::vec3 myVertexS[4];

    for (int n = 0, j = 0; j <= vCount; j++)
    {
        const float t = (float)j / (float)vCount;
        for (int i = 0; i <= uCount; n++, i++)
        {
            const float s = (float)i / (float)uCount;
            vertex[n] = base + s * uEdge + t * vEdge;
            uv[n] = glm::vec2(uOffset + s * uRepeats, vOffset + t * vRepeats);
        }
    }

    if (!fixedUVs && BZDB.isTrue("remapTexCoords"))
    {
        const auto uvLen = glm::vec2(glm::length(uEdge) / uRepeats,
                                     glm::length(vEdge) / vRepeats);
        const auto scale = 10.0f / floor(10.0f * uvLen);
        if (fabsf(normal[2]) > 0.999f)
        {
            // horizontal surface
            for (unsigned int i = 0; i < vertex.size(); i++)
                uv[i] = scale * glm::vec2(vertex[i]);
        }
        else
        {
            // vertical surface
            const auto n = glm::normalize(glm::vec2(normal));
            const float vs = glm::inversesqrt(1.0f - normal[2] * normal[2]);
            for (unsigned int i = 0; i < vertex.size(); i++)
            {
                const auto v = vertex[i];
                const float uGeoScale = (n.x * v[1]) - (n.y * v[0]);
                const float vGeoScale = v[2] * vs;
                uv[i] = scale * glm::vec2(uGeoScale, vGeoScale);
            }

        }
    }

    int k = 0;
    int t = 0;
    while (1)
    {
        for (int s = 0; s <= ds; s++)
        {
            myTextur.push_back(uv[k+ds+1]);
            myTextur.push_back(uv[k]);
            myVertex.push_back(vertex[k+ds+1]);
            myVertex.push_back(vertex[k]);
            k++;
        }

        t++;
        if (t >= dt)
            break;
        // Degenerate triangles
        myTextur.push_back(uv[k - 1]);
        myVertex.push_back(vertex[k - 1]);
        myTextur.push_back(uv[k + ds + 1]);
        myVertex.push_back(vertex[k + ds + 1]);
    }
    vboIndex = Vertex_Chunk(Vertex_Chunk::VT, myVertex.size());
    vboIndex.textureData(myTextur);
    vboIndex.vertexData(myVertex);

    triangles = 2 * (uCount * vCount);

    int last = (ds + 1) * dt;
    myVertexS[0] = vertex[last];
    myVertexS[1] = vertex[0];
    myVertexS[2] = vertex[last + ds];
    myVertexS[3] = vertex[ds];
    vboIndexS.vertexData(myVertexS);
}

QuadWallSceneNode::Geometry::~Geometry()
{
    // do nothing
}

void            QuadWallSceneNode::Geometry::render()
{
    wall->setColor();
    glNormal3f(normal.x, normal.y, normal.z);
    vboIndex.draw(GL_TRIANGLE_STRIP, style < 2);
    addTriangleCount(triangles);
    return;
}

void            QuadWallSceneNode::Geometry::renderShadow()
{
    vboIndex.draw(GL_TRIANGLE_STRIP, true);
    addTriangleCount(2);
}

const glm::vec3 &QuadWallSceneNode::Geometry::getVertex(int i) const
{
    return vertex[i];
}

const glm::vec3 QuadWallSceneNode::Geometry::getPosition() const
{
    return wall->getCenter();
}

//
// QuadWallSceneNode
//

QuadWallSceneNode::QuadWallSceneNode(const glm::vec3 &base,
                                     const glm::vec3 &uEdge,
                                     const glm::vec3 &vEdge,
                                     float uOffset,
                                     float vOffset,
                                     float uRepeats,
                                     float vRepeats,
                                     bool makeLODs)
{
    init(base, uEdge, vEdge, uOffset, vOffset, uRepeats, vRepeats, makeLODs, false);
}

QuadWallSceneNode::QuadWallSceneNode(const glm::vec3 &base,
                                     const glm::vec3 &uEdge,
                                     const glm::vec3 &vEdge,
                                     float uRepeats,
                                     float vRepeats,
                                     bool makeLODs, bool fixedUVs)
{
    init(base, uEdge, vEdge, 0.0f, 0.0f, uRepeats, vRepeats, makeLODs, fixedUVs);
}

void            QuadWallSceneNode::init(const glm::vec3 &base,
                                        const glm::vec3 &uEdge,
                                        const glm::vec3 &vEdge,
                                        float uOffset,
                                        float vOffset,
                                        float uRepeats,
                                        float vRepeats,
                                        bool makeLODs,
                                        bool fixedUVs)
{
    // record plane and bounding sphere info
    auto norm = glm::cross(uEdge, vEdge);
    auto myPlane = glm::vec4(norm, -glm::dot(norm, base));
    setPlane(myPlane);
    auto myCenter = (uEdge + vEdge) / 2.0f;
    const auto myRadius = glm::length2(myCenter);
    myCenter += base;
    setCenter(myCenter);
    setRadius(myRadius);

    // get length of sides
    const float uLength = glm::length(uEdge);
    const float vLength = glm::length(vEdge);
    float area = uLength * vLength;

    // If negative then these values aren't a number of times to repeat
    // the texture along the surface but the width, or a desired scaled
    // width, of the texture itself. Repeat the texture as many times
    // as necessary to fit the surface.
    if (uRepeats < 0.0f)
        uRepeats = - uLength / uRepeats;

    if (vRepeats < 0.0f)
        vRepeats = - vLength / vRepeats;

    // compute how many LODs required to get smaller edge down to
    // elements no bigger than 4 units on a side.
    int uElements = int(uLength) / 2;
    int vElements = int(vLength) / 2;
    int uLevels = 1, vLevels = 1;
    while (uElements >>= 1) uLevels++;
    while (vElements >>= 1) vLevels++;
    int numLevels = std::min(uLevels, vLevels);

    // if overly rectangular then add levels to square it up
    bool needsSquaring = false;
    if (makeLODs)
    {
        if (uLevels >= vLevels+2)
        {
            needsSquaring = true;
            numLevels += (uLevels - vLevels) / 2;
        }
        else if (vLevels >= uLevels+2)
        {
            needsSquaring = true;
            numLevels += (vLevels - uLevels) / 2;
        }
    }

    // if no lod's required then don't make any except most coarse
    if (!makeLODs)
        numLevels = 1;

    // make level of detail and element area arrays
    nodes = new Geometry*[numLevels];
    float* areas = new float[numLevels];

    // make top level (single polygon)
    int level = 0;
    uElements = 1;
    vElements = 1;
    areas[level] = area;
    nodes[level++] = new Geometry(this, uElements, vElements,
                                  base, uEdge, vEdge,
                                  plane, uOffset, vOffset,
                                  uRepeats, vRepeats, fixedUVs);
    shadowNode = new Geometry(this, uElements, vElements,
                              base, uEdge, vEdge,
                              plane, uOffset, vOffset,
                              uRepeats, vRepeats, fixedUVs);
    shadowNode->setStyle(0);

    // make squaring levels if necessary
    if (needsSquaring)
    {
        uElements = 1;
        vElements = 1;
        if (uLevels > vLevels)
        {
            int count = (uLevels - vLevels) / 2;
            while (count-- > 0)
            {
                uElements <<= 2;
                areas[level] = area / (float)uElements;
                nodes[level++] = new Geometry(this, uElements, vElements,
                                              base, uEdge, vEdge,
                                              plane, uOffset, vOffset,
                                              uRepeats, vRepeats, fixedUVs);

            }
            area /= (float)uElements;
        }
        else
        {
            int count = (vLevels - uLevels) / 2;
            while (count-- > 0)
            {
                vElements <<= 2;
                areas[level] = area / (float)vElements;
                nodes[level++] = new Geometry(this, uElements, vElements,
                                              base, uEdge, vEdge,
                                              plane, uOffset, vOffset,
                                              uRepeats, vRepeats, fixedUVs);

            }
            area /= (float)vElements;
        }
    }

    // make remaining levels by doubling elements in each dimension
    while (level < numLevels)
    {
        uElements <<= 1;
        vElements <<= 1;
        area *= 0.25f;
        areas[level] = area;
        nodes[level++] = new Geometry(this, uElements, vElements,
                                      base, uEdge, vEdge,
                                      plane, uOffset, vOffset,
                                      uRepeats, vRepeats, fixedUVs);
    }

    // record extents info
    for (int i = 0; i < 4; i++)
    {
        const auto point = getVertex(i);
        extents.expandToPoint(point);
    }

    // record LOD info
    setNumLODs(numLevels, areas);
}

QuadWallSceneNode::~QuadWallSceneNode()
{
    // free LODs
    const int numLevels = getNumLODs();
    for (int i = 0; i < numLevels; i++)
        delete nodes[i];
    delete[] nodes;
    delete shadowNode;
}

void            QuadWallSceneNode::addRenderNodes(
    SceneRenderer& renderer)
{
    const int lod = pickLevelOfDetail(renderer);
    nodes[lod]->setStyle(getStyle());
    renderer.addRenderNode(nodes[lod], getWallGState());
}

void            QuadWallSceneNode::addShadowNodes(
    SceneRenderer& renderer)
{
    renderer.addShadowNode(shadowNode);
}

bool            QuadWallSceneNode::inAxisBox(const Extents& exts) const
{
    if (!extents.touches(exts))
        return false;

    return testPolygonInAxisBox (nodes[0]->vertex, plane, exts);
}

int         QuadWallSceneNode::getVertexCount () const
{
    return 4;
}

const glm::vec3 QuadWallSceneNode::getVertex (int vertex) const
{
    // re-map these to a counter-clockwise order
    const int order[4] = {0, 1, 3, 2};
    return nodes[0]->getVertex(order[vertex]);
}


void            QuadWallSceneNode::getRenderNodes(std::vector<RenderSet>& rnodes)
{
    RenderSet rs = { nodes[0], getWallGState() };
    rnodes.push_back(rs);
    return;
}


void            QuadWallSceneNode::renderRadar()
{
    if (plane[2] > 0.0f)
        nodes[0]->renderRadar();
    return;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
