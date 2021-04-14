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
#include "BoltSceneNode.h"

// system headers
#include <stdlib.h>
#include <math.h>

// common implementation headers
#include "StateDatabase.h"
#include "BZDBCache.h"
#include "TextureManager.h"
#include "VBO_Drawing.h"
#include "Singleton.h"
#include "OpenGLCommon.h"
#include "PlayingShader.h"

// local implementation headers
#include "ViewFrustum.h"

// FIXME (SceneRenderer.cxx is in src/bzflag)
#include "SceneRenderer.h"

#include "TimeKeeper.h"

using namespace OpenGLCommon;

const float maxRad = 0.16f;
const float boosterLen = 0.2f;
const float finRadius = 0.16f;
const float finCapSize = 0.15f;
const float finForeDelta = 0.02f;
const float noseRad = 0.086f;
const float noseLen = 0.1f;
const float bevelLen = 0.02f;
const float waistRad = 0.125f;
const float engineLen = 0.08f;
const float FlareSpread = 0.08f;
const float CoreFraction = 0.4f;

#define BOLTDRAWER (BoltDrawer::instance())

class BoltDrawer: public Singleton<BoltDrawer>
{
public:
    void drawFin();
    void drawFlare();
    void nosecone();
    void body();
    void booster1();
    void booster2();
    void engine();
    void hemisphere1(int slices);
    void hemisphere2(int slices);
    void shaft(int slices);
protected:
    friend class Singleton<BoltDrawer>;
private:
    BoltDrawer();
    virtual ~BoltDrawer() = default;

    Vertex_Chunk buildFin();
    Vertex_Chunk buildFlare();
    Vertex_Chunk buildEmy1(int slices);
    Vertex_Chunk buildEmy2(int slices);

    int segment2Pos(int segment);
    Vertex_Chunk drawCylinder(
        float baseRadius, float topRadius, float height, int slices);
    void drawCylinder(
        float baseRadius, float topRadius, float height, int slices,
        std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals);
    const int segments[4] = {16, 25, 32, 48};

    Vertex_Chunk finIndex;
    Vertex_Chunk noseconeIndex;
    Vertex_Chunk bodyIndex;
    Vertex_Chunk booster1Index;
    Vertex_Chunk booster2Index;
    Vertex_Chunk engineIndex;
    Vertex_Chunk hemy1Index[4];
    Vertex_Chunk hemy2Index[4];
    Vertex_Chunk shaftIndex[4];
    Vertex_Chunk flareIndex;

    // parametrics
    const float engineRad = 0.1f;
};

BoltDrawer::BoltDrawer()
{
    finIndex    = buildFin();
    flareIndex  = buildFlare();
    int slices = 8;
    noseconeIndex = drawCylinder(maxRad,    noseRad,  noseLen,   slices);
    bodyIndex     = drawCylinder(waistRad,  maxRad,   bevelLen,  slices);
    booster1Index = drawCylinder(maxRad,    waistRad, bevelLen,  slices);
    booster2Index = drawCylinder(waistRad,  maxRad,   bevelLen,  slices);
    engineIndex   = drawCylinder(engineRad, waistRad, engineLen, slices);

    for (int i = 0; i < 4; i++)
    {
        slices = segments[i];
        hemy1Index[i] = buildEmy1(slices);
        hemy2Index[i] = buildEmy2(slices);
        shaftIndex[i] = drawCylinder(1.0f, 1.0f, 1.0f, slices);
    }
}

Vertex_Chunk BoltDrawer::buildFin()
{
    const float finalRadius = maxRad     + finRadius;
    const float finFore     = boosterLen - finForeDelta;
    const float finCap      = finFore    - finCapSize;
    std::vector<glm::vec3> finNormal;
    std::vector<glm::vec3> finVertex;

    finNormal.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f, maxRad, 0.0f));
    finNormal.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f, maxRad, boosterLen));
    finNormal.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f, finalRadius, finCap));
    finNormal.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f, finalRadius, finFore));

    finNormal.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f, finalRadius, finCap));
    finNormal.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f, finalRadius, finFore));
    finNormal.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f, maxRad, 0.0f));
    finNormal.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f, maxRad, boosterLen));

    // Degraded triangles
    finNormal.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3( 0.0f,   maxRad, boosterLen));
    finNormal.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    finVertex.push_back(glm::vec3(-maxRad, 0.0f,   0.0f));

    finNormal.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    finVertex.push_back(glm::vec3(-maxRad,      0.0f, 0.0f));
    finNormal.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    finVertex.push_back(glm::vec3(-maxRad,      0.0f, boosterLen));
    finNormal.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    finVertex.push_back(glm::vec3(-finalRadius, 0.0f, finCap));
    finNormal.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    finVertex.push_back(glm::vec3(-finalRadius, 0.0f, finFore));

    finNormal.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    finVertex.push_back(glm::vec3(-finalRadius, 0.0f, finCap));
    finNormal.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    finVertex.push_back(glm::vec3(-finalRadius, 0.0f, finFore));
    finNormal.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    finVertex.push_back(glm::vec3(-maxRad,      0.0f, 0.0f));
    finNormal.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    finVertex.push_back(glm::vec3(-maxRad,      0.0f, boosterLen));

    // Degraded triangles
    finNormal.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    finVertex.push_back(glm::vec3(-maxRad, 0.0f, boosterLen));
    finNormal.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    finVertex.push_back(glm::vec3( maxRad, 0.0f, 0.0f));

    finNormal.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    finVertex.push_back(glm::vec3(maxRad,      0.0f, 0.0f));
    finNormal.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    finVertex.push_back(glm::vec3(maxRad,      0.0f, boosterLen));
    finNormal.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    finVertex.push_back(glm::vec3(finalRadius, 0.0f, finCap));
    finNormal.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
    finVertex.push_back(glm::vec3(finalRadius, 0.0f, finFore));

    finNormal.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    finVertex.push_back(glm::vec3(finalRadius, 0.0f, finCap));
    finNormal.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    finVertex.push_back(glm::vec3(finalRadius, 0.0f, finFore));
    finNormal.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    finVertex.push_back(glm::vec3(maxRad,      0.0f, 0.0f));
    finNormal.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    finVertex.push_back(glm::vec3(maxRad,      0.0f, boosterLen));

    // Degraded triangles
    finNormal.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    finVertex.push_back(glm::vec3(maxRad, 0.0f,    boosterLen));
    finNormal.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f,   -maxRad, 0.0f));

    finNormal.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f,  -maxRad,      0.0f));
    finNormal.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f,  -maxRad,      boosterLen));
    finNormal.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f,  -finalRadius, finCap));
    finNormal.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f,  -finalRadius, finFore));

    finNormal.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f,  -finalRadius, finCap));
    finNormal.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f,  -finalRadius, finFore));
    finNormal.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f,  -maxRad,     0.0f));
    finNormal.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    finVertex.push_back(glm::vec3(0.0f,  -maxRad,      boosterLen));

    Vertex_Chunk vboIndex = Vertex_Chunk(Vertex_Chunk::VN, finVertex.size());
    vboIndex.normalData(finNormal);
    vboIndex.vertexData(finVertex);

    return vboIndex;
}

Vertex_Chunk BoltDrawer::buildFlare()
{
    Vertex_Chunk vboIndex;
    glm::vec3    vertex[4];
    const float fs = FlareSpread;

    vertex[0] = glm::vec3(CoreFraction,  0.0f,        0.0f);
    vertex[1] = glm::vec3(cosf(fs),     -sin(fs),     0.0f);
    vertex[2] = glm::vec3(vertex[1].x,  -vertex[1].y, 0.0f);
    vertex[3] = glm::vec3(2.0f,          0.0f,        0.0f);
    vboIndex = Vertex_Chunk(Vertex_Chunk::V, 4);
    vboIndex.vertexData(vertex);
    return vboIndex;
}

Vertex_Chunk BoltDrawer::drawCylinder(
    float baseRadius, float topRadius, float height, int slices)
{
    Vertex_Chunk vboIndex;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec3> vertex;

    drawCylinder(baseRadius, topRadius, height, slices, vertex, normal);
    vboIndex = Vertex_Chunk(Vertex_Chunk::VN, vertex.size());
    vboIndex.normalData(normal);
    vboIndex.vertexData(vertex);
    return vboIndex;
}


void BoltDrawer::drawCylinder(
    float baseRadius, float topRadius, float height, int slices,
    std::vector<glm::vec3> &vertex, std::vector<glm::vec3> &normal)
{
    /* Compute length (needed for normal calculations) */
    float deltaRadius = baseRadius - topRadius;
    float length = sqrt(deltaRadius * deltaRadius + height * height);

    float zNormal  = deltaRadius / length;
    float xyNormalRatio = height / length;

    auto normalV = glm::vec3(0.0f, xyNormalRatio, zNormal);
    normal.push_back(normalV);
    normal.push_back(normalV);
    vertex.push_back(glm::vec3(0.0f, baseRadius,    0.0f));
    vertex.push_back(glm::vec3(0.0f, topRadius,     height));
    for (int i = 1; i < slices; i++)
    {
        float angle = (float)(2 * M_PI * i / slices);
        float sinCache = sin(angle);
        float cosCache = cos(angle);
        normalV = glm::vec3(xyNormalRatio * sinCache, xyNormalRatio * cosCache, zNormal);

        normal.push_back(normalV);
        normal.push_back(normalV);
        vertex.push_back(glm::vec3(baseRadius    * sinCache, baseRadius    * cosCache, 0.0f));
        vertex.push_back(glm::vec3(topRadius     * sinCache, topRadius     * cosCache, height));
    }
    normalV = glm::vec3(0.0f, xyNormalRatio, zNormal);
    normal.push_back(normalV);
    normal.push_back(normalV);
    vertex.push_back(glm::vec3(0.0f, baseRadius,    0.0f));
    vertex.push_back(glm::vec3(0.0f, topRadius,     height));
}

void BoltDrawer::nosecone()
{
    noseconeIndex.draw(GL_TRIANGLE_STRIP);
}

void BoltDrawer::body()
{
    bodyIndex.draw(GL_TRIANGLE_STRIP);
}

void BoltDrawer::booster1()
{
    booster1Index.draw(GL_TRIANGLE_STRIP);
}

void BoltDrawer::booster2()
{
    booster2Index.draw(GL_TRIANGLE_STRIP);
}

void BoltDrawer::engine()
{
    engineIndex.draw(GL_TRIANGLE_STRIP);
}

int BoltDrawer::segment2Pos(int slices)
{
    int i;
    if (slices == 16)
        i = 0;
    else if (slices == 25)
        i = 1;
    else if (slices == 32)
        i = 2;
    else if (slices == 48)
        i = 3;
    else
        abort();
    return i;
}

Vertex_Chunk BoltDrawer::buildEmy1(int slices)
{
    std::vector<glm::vec3> normal;
    std::vector<glm::vec3> vertex;
    Vertex_Chunk vboIndex;

    drawCylinder(0.0f, 0.43589f, 0.1f, slices, vertex, normal);
    // Add degenerate triangle
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    auto lastSize = vertex.size();

    drawCylinder(0.43589f, 0.66144f, 0.15f, slices, vertex, normal);
    // translate
    for (auto j = lastSize; j < vertex.size(); j++)
        vertex[j].z += 0.1f;

    vertex[lastSize - 1] = vertex[lastSize];
    normal[lastSize - 1] = normal[lastSize];

    // Add degenerate triangle
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    lastSize = vertex.size();
    drawCylinder(0.66144f, 0.86603f, 0.25f, slices, vertex, normal);
    // translate
    for (auto j = lastSize; j < vertex.size(); j++)
        vertex[j].z += 0.25f;

    vertex[lastSize - 1] = vertex[lastSize];
    normal[lastSize - 1] = normal[lastSize];

    // Add degenerate triangle
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    lastSize = vertex.size();
    drawCylinder(0.86603f, 1.0f, 0.5f, slices, vertex, normal);
    // translate
    for (auto j = lastSize; j < vertex.size(); j++)
        vertex[j].z += 0.5f;

    vertex[lastSize - 1] = vertex[lastSize];
    normal[lastSize - 1] = normal[lastSize];

    vboIndex = Vertex_Chunk(Vertex_Chunk::VN, vertex.size());
    vboIndex.normalData(normal);
    vboIndex.vertexData(vertex);

    return vboIndex;
}

void BoltDrawer::hemisphere1(int slices)
{
    int i = segment2Pos(slices);

    // 4 parts of the first hemisphere
    hemy1Index[i].draw(GL_TRIANGLE_STRIP);
}

Vertex_Chunk BoltDrawer::buildEmy2(int slices)
{
    std::vector<glm::vec3> normal;
    std::vector<glm::vec3> vertex;
    Vertex_Chunk vboIndex;

    drawCylinder(1.0f, 0.86603f, 0.5f, slices, vertex, normal);
    // Add degenerate triangle
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    auto lastSize = vertex.size();

    drawCylinder(0.86603f, 0.66144f, 0.25f, slices, vertex, normal);
    // translate
    for (auto j = lastSize; j < vertex.size(); j++)
        vertex[j].z += 0.5f;

    vertex[lastSize - 1] = vertex[lastSize];
    normal[lastSize - 1] = normal[lastSize];

    // Add degenerate triangle
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    lastSize = vertex.size();
    drawCylinder(0.66144f, 0.43589f, 0.15f, slices, vertex, normal);
    // translate
    for (auto j = lastSize; j < vertex.size(); j++)
        vertex[j].z += 0.75f;

    vertex[lastSize - 1] = vertex[lastSize];
    normal[lastSize - 1] = normal[lastSize];

    // Add degenerate triangle
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    vertex.push_back(vertex.back());
    normal.push_back(normal.back());
    lastSize = vertex.size();
    drawCylinder(0.43589f, 0.0f, 0.1f, slices, vertex, normal);
    // translate
    for (auto j = lastSize; j < vertex.size(); j++)
        vertex[j].z += 0.9f;

    vertex[lastSize - 1] = vertex[lastSize];
    normal[lastSize - 1] = normal[lastSize];

    vboIndex = Vertex_Chunk(Vertex_Chunk::VN, vertex.size());
    vboIndex.normalData(normal);
    vboIndex.vertexData(vertex);

    return vboIndex;
}

void BoltDrawer::hemisphere2(int slices)
{
    int i = segment2Pos(slices);

    // 4 parts of the last hemisphere
    hemy2Index[i].draw(GL_TRIANGLE_STRIP);
}

void BoltDrawer::shaft(int slices)
{
    int i = segment2Pos(slices);

    shaftIndex[i].draw(GL_TRIANGLE_STRIP);
}

BoltSceneNode::BoltSceneNode(const glm::vec3 &pos, const glm::vec3 &vel, bool super) :
    isSuper(super),
    invisible(false),
    drawFlares(false),
    texturing(false),
    colorblind(false),
    size(1.0f),
    renderNode(this),
    azimuth(0),
    elevation(0),
    length(1.0f)
{

    OpenGLGStateBuilder builder(gstate);
    builder.setBlending();
    builder.setAlphaFunc();
    //builder.setTextureEnvMode(GL_DECAL);
    gstate = builder.getState();

    // prepare light
    light.setAttenuation(0, 0.05f);
    light.setAttenuation(1, 0.0f);
    light.setAttenuation(2, 0.03f);

    // prepare geometry
    move(pos, vel);
    setSize(size);
    setColor(1.0f, 1.0f, 1.0f);
    teamColor = glm::vec4(1,1,1,1);
}

BoltSceneNode::~BoltSceneNode()
{
    // do nothing
}

void            BoltSceneNode::setFlares(bool on)
{
    drawFlares = on;
}

void            BoltSceneNode::setSize(float radius)
{
    size = radius;
    setRadius(size * size);
}
void            BoltSceneNode::setTextureColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    color = glm::vec4(r, g, b, a);
    light.setColor(1.5f * r, 1.5f * g, 1.5f * b);
    renderNode.setTextureColor(color);
}

void            BoltSceneNode::setColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    color = glm::vec4(r, g, b, a);
    light.setColor(1.5f * r, 1.5f * g, 1.5f * b);
    renderNode.setColor(color);
}

void            BoltSceneNode::setTeamColor(const glm::vec3 &c)
{
    teamColor = glm::vec4(c, 1.0f);
}

void            BoltSceneNode::setColor(const glm::vec3 &rgb)
{
    setColor(rgb[0], rgb[1], rgb[2]);
}

bool            BoltSceneNode::getColorblind() const
{
    return colorblind;
}

void            BoltSceneNode::setColorblind(bool _colorblind)
{
    colorblind = _colorblind;
}

void            BoltSceneNode::setTexture(const int texture)
{
    OpenGLGStateBuilder builder(gstate);
    builder.setTexture(texture);
    builder.enableTexture(texture>=0);
    gstate = builder.getState();
}

void            BoltSceneNode::setTextureAnimation(int cu, int cv)
{
    renderNode.setAnimation(cu, cv);
}

void            BoltSceneNode::move(const glm::vec3 &pos,
                                    const glm::vec3 &vel)
{
    setCenter(pos);
    light.setPosition(pos);
    velocity = vel;
    length = glm::length(vel);

    azimuth   = (float)(+RAD2DEG * atan2f(vel[1], vel[0]));
    elevation = (float)(-RAD2DEG * atan2f(vel[2], sqrtf(vel[0]* vel[0] + vel[1] *vel[1])));
}

void            BoltSceneNode::addLight(
    SceneRenderer& renderer)
{
    renderer.addLight(light);
}

void            BoltSceneNode::notifyStyleChange()
{
    texturing = BZDBCache::texture;
    OpenGLGStateBuilder builder(gstate);
    builder.enableTexture(texturing);
    const int shotLength = int(BZDBCache::shotLength * 3);
    if (shotLength > 0 && !drawFlares)
        builder.setBlending(GL_SRC_ALPHA, GL_ONE);
    else
        builder.setBlending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    builder.setStipple(1.0f);
    builder.setAlphaFunc();
    if ((RENDERER.useQuality() >= 3) && drawFlares)
    {
        builder.setShading(GL_SMOOTH);
        builder.enableMaterial(false);
    }
    else
        builder.setShading(texturing ? GL_FLAT : GL_SMOOTH);
    gstate = builder.getState();
}

void            BoltSceneNode::addRenderNodes(
    SceneRenderer& renderer)
{
    renderer.addRenderNode(&renderNode, &gstate);
}

//
// BoltSceneNode::BoltRenderNode
//

const GLfloat   FlareSize = 1.0f;
glm::vec3       core[8];
glm::vec3       corona[8];
const glm::vec3 ring[8] =
{
    { 1.0f, 0.0f, 0.0f },
    { (float)M_SQRT1_2, (float)M_SQRT1_2, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { (float)-M_SQRT1_2, (float)M_SQRT1_2, 0.0f },
    { -1.0f, 0.0f, 0.0f },
    { (float)-M_SQRT1_2, (float)-M_SQRT1_2, 0.0f },
    { 0.0f, -1.0f, 0.0f },
    { (float)M_SQRT1_2, (float)-M_SQRT1_2, 0.0f }
};

BoltSceneNode::BoltRenderNode::BoltRenderNode(
    const BoltSceneNode* _sceneNode) :
    sceneNode(_sceneNode),
    numFlares(0)
{
    // initialize core and corona if not already done
    static bool init = false;
    if (!init)
    {
        init = true;
        core[0] = glm::vec3(0.0f);
        for (int i = 0; i < 8; i++)
        {
            core[i] = CoreFraction * ring[i];
            corona[i] = ring[i];
        }
    }

    textureColor = glm::vec4(1.0f);

    setAnimation(1, 1);
}

BoltSceneNode::BoltRenderNode::~BoltRenderNode()
{
    // do nothing
}

void            BoltSceneNode::BoltRenderNode::setAnimation(
    int _cu, int _cv)
{
    cu = _cu;
    cv = _cv;
    du = 1.0f / (float)cu;
    dv = 1.0f / (float)cv;

    // pick a random start frame
    const int index = (int)((float)cu * (float)cv * bzfrand());
    u = index % cu;
    v = index / cu;
    if (v >= cv) v = 0;
}
void            BoltSceneNode::BoltRenderNode::setTextureColor(const glm::vec4 &rgba)
{
    textureColor = rgba;
}


void            BoltSceneNode::BoltRenderNode::setColor(
    const glm::vec4 &rgba)
{
    mainColor   = rgba;
    innerColor  = 0.5f * (1.0f + mainColor);
    outerColor  = mainColor;
    coronaColor = mainColor;
    flareColor  = mainColor;

    innerColor.a = rgba.a;
    if (rgba.a == 1.0f)
    {
        outerColor.a  = 0.1f;
        coronaColor.a = 0.5f;
        flareColor.a  = 0.667f;
    }

    // compute corona & core vbo
    glm::vec3 vertex[18];
    glm::vec4 colors[18];
    int j = 0;

    for (auto i = 0; i < 8; i++)
    {
        colors[j] = mainColor;
        vertex[j++] = core[i];
        colors[j] = outerColor;
        vertex[j++] = corona[i];
    }
    colors[j] = mainColor;
    vertex[j++] = core[0];
    colors[j] = outerColor;
    vertex[j++] = corona[0];
    coronaIndex = Vertex_Chunk(Vertex_Chunk::VC, 18);
    coronaIndex.colorData(colors);
    coronaIndex.vertexData(vertex);

    colors[0] = innerColor;
    vertex[0] = glm::vec3(0.0f);
    for (auto i = 0; i < 8; i++)
    {
        colors[i + 1] = mainColor;
        vertex[i + 1] = core[i];
    }
    colors[9] = mainColor;
    vertex[9] = core[0];
    coreIndex = Vertex_Chunk(Vertex_Chunk::VC, 10);
    coreIndex.colorData(colors);
    coreIndex.vertexData(vertex);
}

void BoltDrawer::drawFin()
{
    finIndex.draw(GL_TRIANGLE_STRIP);
}

void BoltDrawer::drawFlare()
{
    flareIndex.draw(GL_TRIANGLE_STRIP);
}

void BoltSceneNode::BoltRenderNode::renderGeoGMBolt()
{
    // bzdb these 2? they control the shot size
    float gmMissleSize = BZDBCache::gmSize;

    // parametrics
    float bodyLen = 0.44f;
    float waistLen = 0.16f;

#ifdef DEBUG_RENDERING
    int slices = 8;
#endif

    float rotSpeed = 90.0f;

    glDepthMask(GL_TRUE);
    glRotatef(sceneNode->azimuth, 0.0f, 0.0f, 1.0f);
    glRotatef(sceneNode->elevation, 0.0f, 1.0f, 0.0f);
    glRotatef(90, 0.0f, 1.0f, 0.0f);

    SHADER.setTexturing(false);

    glm::vec4 noseColor = sceneNode->teamColor;
    glm::vec4 finColor(noseColor.r*0.5f,noseColor.g*0.5f,noseColor.b*0.5f,1);
    glm::vec4 coneColor(0.125f,0.125f,0.125f,1);
    glm::vec4 bodyColor(1,1,1,1);

    glPushMatrix();
    glScalef(gmMissleSize, gmMissleSize, gmMissleSize);

    glColor4f(noseColor.r,noseColor.g,noseColor.b,1.0f);
    glTranslatef(0, 0, 1.0f);
    glRotatef((float)TimeKeeper::getCurrent().getSeconds() * rotSpeed,0,0,1);

    // nosecone
    glPushMatrix();
    glScalef(noseRad, noseRad, 0.0f);
    DRAWER.disk8();
    glPopMatrix();
    glTranslatef(0, 0, -noseLen);
    BOLTDRAWER.nosecone();
    addTriangleCount(slices * 2);

    // body
    if (!colorOverride)
        glColor4f(bodyColor.r, bodyColor.g, bodyColor.b, bodyColor.a);
    glTranslatef(0, 0, -bodyLen);
    glPushMatrix();
    glScalef(maxRad, maxRad, bodyLen);
    DRAWER.cylinder8();
    glPopMatrix();
    addTriangleCount(slices);

    glTranslatef(0, 0, -bevelLen);
    BOLTDRAWER.body();
    addTriangleCount(slices);

    // waist
    if (!colorOverride)
        glColor4f(coneColor.r, coneColor.g, coneColor.b, coneColor.a);
    glTranslatef(0, 0, -waistLen);
    glPushMatrix();
    glScalef(waistRad, waistRad, waistLen);
    DRAWER.cylinder8();
    glPopMatrix();
    addTriangleCount(slices);

    // booster
    if (!colorOverride)
        glColor4f(bodyColor.r, bodyColor.g, bodyColor.b, 1.0f);
    glTranslatef(0, 0, -bevelLen);
    BOLTDRAWER.booster1();
    addTriangleCount(slices);

    glTranslatef(0, 0, -boosterLen);
    glPushMatrix();
    glScalef(maxRad, maxRad, boosterLen);
    DRAWER.cylinder8();
    glPopMatrix();
    addTriangleCount(slices);

    glTranslatef(0, 0, -bevelLen);
    BOLTDRAWER.booster2();
    addTriangleCount(slices);

    // engine
    if (!colorOverride)
        glColor4f(coneColor.r, coneColor.g, coneColor.b, 1.0f);
    glTranslatef(0, 0, -engineLen);
    BOLTDRAWER.engine();
    addTriangleCount(slices);

    // fins
    if (!colorOverride)
        glColor4f(finColor.r, finColor.g, finColor.b, 1.0f);
    glTranslatef(0, 0, engineLen + bevelLen);

    BOLTDRAWER.drawFin();

    glPopMatrix();

    SHADER.setTexturing(true);

    glDepthMask(GL_FALSE);
}


void BoltSceneNode::BoltRenderNode::renderGeoBolt()
{
    // bzdb these 2? they control the shot size
    float lenMod = 0.0675f + (BZDBCache::shotLength * 0.0125f);
    float baseRadius = 0.225f;

    float len = sceneNode->length * lenMod;
    glRotatef(sceneNode->azimuth, 0.0f, 0.0f, 1.0f);
    glRotatef(sceneNode->elevation, 0.0f, 1.0f, 0.0f);
    glRotatef(90, 0.0f, 1.0f, 0.0f);

    float alphaMod = 1.0f;
// if (sceneNode->phasingShot)
//   alphaMod = 0.85f;

    SHADER.setTexturing(false);

    float coreBleed = 4.5f;
    float minimumChannelVal = 0.45f;

    const auto c = sceneNode->color;
    auto coreColor = glm::max(c * coreBleed, minimumChannelVal);

    if (!colorOverride)
        glColor4f(coreColor.r, coreColor.g, coreColor.b, 0.85f * alphaMod);
    glPushMatrix();
    renderGeoPill(baseRadius,len,16);
    glPopMatrix();

    float radInc = 1.5f * baseRadius - baseRadius;
    glPushMatrix();
    glTranslatef(0, 0, -radInc * 0.5f);
    if (!colorOverride)
        glColor4f(c.r, c.g, c.b, 0.5f);
    renderGeoPill(1.5f * baseRadius, len + radInc, 25);
    glPopMatrix();

    radInc = 2.7f * baseRadius - baseRadius;
    glPushMatrix();
    glTranslatef(0, 0, -radInc*0.5f);
    if (!colorOverride)
        glColor4f(c.r, c.g, c.b, 0.25f);
    renderGeoPill(2.7f * baseRadius, len + radInc, 32);
    glPopMatrix();

    radInc = 3.8f * baseRadius - baseRadius;
    glPushMatrix();
    glTranslatef(0, 0,-radInc*0.5f);
    if (!colorOverride)
        glColor4f(c.r, c.g, c.b, 0.125f);
    renderGeoPill(3.8f * baseRadius, len + radInc, 48);
    glPopMatrix();

    SHADER.setTexturing(true);
}


void BoltSceneNode::BoltRenderNode::renderGeoPill(float radius, float len,
        int segments)
{
    glScalef(radius, radius, radius);

    // 4 parts of the first hemisphere
    BOLTDRAWER.hemisphere1(segments);
    addTriangleCount(4 * segments);

    glTranslatef(0.0f, 0.0f, 1.0f);

    // the "shaft"
    if (len > 2.0f * radius)
    {
        float lenMinusRads = len / radius - 2.0f;

        glPushMatrix();
        glScalef(1.0f, 1.0f, lenMinusRads);
        BOLTDRAWER.shaft(segments);
        glPopMatrix();
        addTriangleCount(segments);
        glTranslatef(0.0f, 0.0f, lenMinusRads);
    }

    // 4 parts of the last hemisphere
    BOLTDRAWER.hemisphere2(segments);
    addTriangleCount(4 * segments);
}

void            BoltSceneNode::BoltRenderNode::render()
{
    if (sceneNode->invisible)
        return;
    const float radius = sceneNode->size;
    const int   shotLength = (int)(BZDBCache::shotLength * 3.0f);
    const bool  experimental = (RENDERER.useQuality() >= 3);

    const bool blackFog = RENDERER.isFogActive() &&
                          ((shotLength > 0) || experimental);
    if (blackFog)
        glFogfv(GL_FOG_COLOR, glm::value_ptr(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)));

    auto pos = sceneNode->getCenter();
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);

    if (experimental && sceneNode->isSuper)
        renderGeoBolt();
    else
    {
        if (sceneNode->drawFlares)
        {
            if (experimental && (BZDBCache::shotLength > 0))
            {
                glPushMatrix();
                renderGeoGMBolt();
                glPopMatrix();
            }
            RENDERER.getViewFrustum().executeBillboard();
            glScalef(radius, radius, radius);
            // draw some flares
            if (!RENDERER.isSameFrame())
            {
                numFlares = 3 + int(3.0f * (float)bzfrand());
                for (int i = 0; i < numFlares; i++)
                {
                    theta[i] = (float)(2.0 * M_PI * bzfrand());
                    phi[i] = (float)bzfrand() - 0.5f;
                    phi[i] *= (float)(2.0 * M_PI * fabsf(phi[i]));
                }
            }

            bool oldTexturing = SHADER.setTexturing(false);
            if (!colorOverride)
                glColor4f(flareColor.r, flareColor.g, flareColor.b, flareColor.a);
            for (int i = 0; i < numFlares; i++)
            {
                // pick random direction in 3-space.  picking a random theta with
                // a uniform distribution is fine, but doing so with phi biases
                // the directions toward the poles.  my correction doesn't remove
                // the bias completely, but moves it towards the equator, which is
                // really where i want it anyway cos the flares are more noticeable
                // there.
                const float ti = theta[i];
                glPushMatrix();
                glRotatef(ti * 180.0 / M_PI,     0.0f, 0.0f, 1.0f);
                glRotatef(phi[i] * 180.0 / M_PI, 0.0f, 1.0f, 1.0f);
                BOLTDRAWER.drawFlare();
                glPopMatrix();
            }
            SHADER.setTexturing(oldTexturing);

            addTriangleCount(numFlares * 2);
        }
        else
        {
            RENDERER.getViewFrustum().executeBillboard();
            glScalef(radius, radius, radius);
        }

        if (sceneNode->texturing)
        {
            // draw billboard square
            if (!colorOverride)
                glColor4f(textureColor.r, textureColor.g, textureColor.b, textureColor.a);
            glMatrixMode(GL_TEXTURE);
            glPushMatrix();
            glLoadIdentity();
            glScalef(du, dv, 0.0f);
            glTranslatef(u, v, 0.0f);
            DRAWER.simmetricTexturedRect();
            addTriangleCount(2);

            // draw shot trail  (more billboarded quads)
            if ((shotLength > 0) && (sceneNode->length > 1.0e-6f))
            {
                const float startSize  = 0.6f;
                const float startAlpha = 0.8f;

                glPushAttrib(GL_TEXTURE_BIT);
                TextureManager &tm = TextureManager::instance();
                const int texID = tm.getTextureID("shot_tail");
                const ImageInfo& texInfo = tm.getInfo(texID);
                if (texInfo.id >= 0)
                    texInfo.texture->execute();

                auto vel = sceneNode->velocity;
                const auto dir = vel * (-1.0f / sceneNode->length);

                const float invLenPlusOne = 1.0f / (float)(shotLength + 1);
                const float shiftScale = 90.0f / (150.0f + (float)shotLength);
                float Size = sceneNode->size * startSize;
                float alpha = startAlpha;
                const float sizeStep  = Size  * invLenPlusOne;
                const float alphaStep = alpha * invLenPlusOne;

                int uvCell = rand() % 16;

                glLoadIdentity();
                glScalef(0.25f, 0.25f, 0);
                glMatrixMode(GL_MODELVIEW);

                for (int i = 0; i < shotLength; i++)
                {
                    Size  -= sizeStep;
                    const float s = Size * (0.65f + (1.0f * (float)bzfrand()));
                    const float shift = s * shiftScale;

                    pos += (shift * dir);
                    if (pos.z < 0.0f)
                        continue;

                    uvCell = (uvCell + 1) % 16;
                    const float U0 = uvCell % 4;
                    const float V0 = uvCell / 4;

                    alpha -= alphaStep;
                    glColor4f(mainColor[0],mainColor[1],mainColor[2], alpha);
                    glPopMatrix();
                    glPushMatrix();

                    glTranslatef(pos.x, pos.y, pos.z);
                    RENDERER.getViewFrustum().executeBillboard();
                    glScalef(s, s, s);

                    glMatrixMode(GL_TEXTURE);
                    glPushMatrix();
                    glTranslatef(U0, V0, 0.0f);
                    DRAWER.simmetricTexturedRect();
                    glPopMatrix();
                    glMatrixMode(GL_MODELVIEW);
                }

                addTriangleCount(shotLength * 2);
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                glPopAttrib(); // revert the texture
            }
            glMatrixMode(GL_TEXTURE);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
        }
        else
        {
            // draw corona
            coronaIndex.draw(GL_TRIANGLE_STRIP, colorOverride);
            // 18 verts -> 16 tris

            // draw core
            coreIndex.draw(GL_TRIANGLE_FAN, colorOverride);
            // 10 verts -> 8 tris

            addTriangleCount(24);
        }
    }

    glPopMatrix();

    if (blackFog)
        glFogfv(GL_FOG_COLOR, glm::value_ptr(RENDERER.getFogColor()));

    if (RENDERER.isLastFrame())
    {
        if (++u == cu)
        {
            u = 0;
            if (++v == cv)
                v = 0;
        }
    }
}


const glm::vec3 BoltSceneNode::BoltRenderNode::getPosition() const
{
    return sceneNode->getCenter();
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
