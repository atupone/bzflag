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

// interface header
#include "MeshFragSceneNode.h"

// system headers
#include <assert.h>
#include <cmath>
#include <cstring>

// common implementation headers
#include "Intersect.h"
#include "MeshFace.h"
#include "MeshSceneNodeGenerator.h"
#include "BzMaterial.h"
#include "StateDatabase.h"
#include "BZDBCache.h"
#include "SceneRenderer.h"


// FIXME - no tessellation is done on for shot lighting


//
// MeshFragSceneNode::Geometry
//

// NOTE: this should be based on visual pixel area
static int minLightDisabling = 100;

void MeshFragSceneNode::Geometry::render()
{
    const int triangles = sceneNode.arrayCount;
    const bool switchLights = (triangles >= minLightDisabling)
                              && BZDBCache::lighting;
    if (switchLights)
        RENDERER.disableLights(sceneNode.extents.mins, sceneNode.extents.maxs);

    // set the color
    sceneNode.setColor();

    sceneNode.chunk.enableArrays(BZDBCache::texture, BZDBCache::lighting, false);
    sceneNode.chunk.glDrawArrays(GL_TRIANGLES);

    if (switchLights)
        RENDERER.reenableLights();

    addTriangleCount(triangles);

    return;
}


void MeshFragSceneNode::Geometry::renderShadow()
{
    sceneNode.chunk.draw(GL_TRIANGLES, true);
    addTriangleCount(sceneNode.arrayCount);
}

void MeshFragSceneNode::Geometry::setStyle(int style_)
{
    style = style_;
}

const GLfloat* MeshFragSceneNode::Geometry::getPosition() const
{
    return sceneNode.getSphere();
}

//
// MeshFragSceneNode
//

MeshFragSceneNode::MeshFragSceneNode(int faceCount_, const MeshFace** faces_)
    : renderNode(*this)
    , faceCount(faceCount_)
    , faces(faces_)
    , arrayCount(0)
    , chunk()
{
    int i, j, k;

    // This is semi-pointless, as assert is typically compiled out in production code.
    // If protection is needed, a more/ robust solution should be provided
    assert ((faceCount > 0) && (faces != NULL));

    // disable the plane
    static const float fakePlane[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    setPlane(fakePlane);

    const BzMaterial* bzmat = faces[0]->getMaterial();

    // disable radar and shadows if required
    noRadar = bzmat->getNoRadar();
    noShadow = bzmat->getNoShadow();

    // set lod info
    setNumLODs(0, NULL /* unused because LOD = 0 */);

    // record extents info
    for (i = 0; i < faceCount; i++)
    {
        const Extents& fExts = faces[i]->getExtents();
        extents.expandToBox(fExts);
    }

    // setup sphere
    float diffs[3];
    diffs[0] = extents.maxs[0] - extents.mins[0];
    diffs[1] = extents.maxs[1] - extents.mins[1];
    diffs[2] = extents.maxs[2] - extents.mins[2];
    float mySphere[4];
    mySphere[0] = 0.5f * (extents.maxs[0] + extents.mins[0]);
    mySphere[1] = 0.5f * (extents.maxs[1] + extents.mins[1]);
    mySphere[2] = 0.5f * (extents.maxs[2] + extents.mins[2]);
    mySphere[3] = 0.25f *
                  ((diffs[0] * diffs[0]) + (diffs[1] * diffs[1]) + (diffs[2] * diffs[2]));
    setSphere(mySphere);

    // count the number of actual vertices
    for (i = 0; i < faceCount; i++)
    {
        const MeshFace* face = faces[i];
        arrayCount = arrayCount + (face->getVertexCount() - 2);
    }

    // make the lists
    const int vertexCount = (arrayCount * 3);
    std::vector<glm::vec3> normals(vertexCount);
    std::vector<glm::vec3> vertices(vertexCount);
    std::vector<glm::vec2> texcoords(vertexCount);

    // fill in the lists
    int arrayIndex = 0;
    for (i = 0; i < faceCount; i++)
    {
        const MeshFace* face = faces[i];
        const int vCount = face->getVertexCount();
        // number of triangles
        const int tcount = (vCount - 2);

        // pre-generate the texcoords if required
        std::vector<glm::vec2> t(vCount);
        if (!face->useTexcoords())
        {
            std::vector<glm::vec3> v(vCount);
            for (j = 0; j < vCount; j++)
                v[j] = face->getVertex(j);
            MeshSceneNodeGenerator::makeTexcoords(face->getPlane(), v, t);
        }

        const glm::vec3 facePlaneNormal = glm::make_vec3(face->getPlane());
        const bool useNorms = face->useNormals();
        const bool useUVs = face->useTexcoords();

        const int numVertsInFace = tcount * 3;
        for (int n = 0; n < numVertsInFace; n++)
        {
            j = n / 3;
            k = n % 3;
            // basically GL_TRIANGLE_FAN done the hard way
            const int vIndex = (k == 0) ? 0 : (j + k);

            // get the vertices
            vertices[arrayIndex] = face->getVertex(vIndex);

            // get the normals
            normals[arrayIndex] = useNorms ? face->getNormal(vIndex) : facePlaneNormal;

            // get the texcoords
            texcoords[arrayIndex] = useUVs ? face->getTexcoord(vIndex) : t[vIndex];

            ++arrayIndex;
        }
    }

    assert(arrayIndex == (arrayCount * 3));

    // Upload geometry data into Vertex_Chunk
    const int totalVerts = arrayCount * 3;

    // Allocate the actual chunk memory via move-assignment operator
    chunk = Vertex_Chunk(Vertex_Chunk::VTN, totalVerts);

    // Fill and upload data safely
    chunk.vertexData(vertices);
    chunk.normalData(normals);
    chunk.textureData(texcoords);
    chunk.syncInterlaced();
}


MeshFragSceneNode::~MeshFragSceneNode()
{
    delete[] faces;
}


const GLfloat* MeshFragSceneNode::getPlane() const
{
    return NULL;
}

bool MeshFragSceneNode::cull(const ViewFrustum& frustum) const
{
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


bool MeshFragSceneNode::inAxisBox (const Extents& exts) const
{
    // NOTE: it should be OK to use the faces while building

    float pos[3];
    pos[0] = 0.5f * (exts.maxs[0] + exts.mins[0]);
    pos[1] = 0.5f * (exts.maxs[1] + exts.mins[1]);
    pos[2] = exts.mins[2];
    float size[3];
    size[0] = 0.5f * (exts.maxs[0] - exts.mins[0]);
    size[1] = 0.5f * (exts.maxs[1] - exts.mins[1]);
    size[2] = (exts.maxs[2] - exts.mins[2]);

    for (int i = 0; i < faceCount; i++)
    {
        if (faces[i]->inBox(pos, 0.0f, size[0], size[1], size[2]))
            return true;
    }

    return false;
}


void MeshFragSceneNode::addRenderNodes(SceneRenderer& renderer)
{
    renderNode.setStyle(getStyle());
    const GLfloat* dyncol = getDynamicColor();
    if ((dyncol == NULL) || (dyncol[3] != 0.0f))
        renderer.addRenderNode(&renderNode, getWallGState());
}


void MeshFragSceneNode::addShadowNodes(SceneRenderer& renderer)
{
    if (!noShadow)
    {
        const GLfloat* dyncol = getDynamicColor();
        if ((dyncol == NULL) || (dyncol[3] != 0.0f))
            renderer.addShadowNode(&renderNode);
    }
}


void MeshFragSceneNode::renderRadar()
{
    if (!noRadar)
        renderNode.renderRadar();
    return;
}


void MeshFragSceneNode::getRenderNodes(std::vector<RenderSet>& rnodes)
{
    RenderSet rs = { &renderNode, getWallGState() };
    rnodes.push_back(rs);
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
