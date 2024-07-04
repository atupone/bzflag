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
#include "BackgroundRenderer.h"

// system headers
#include <string.h>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>

// common headers
#include "OpenGLMaterial.h"
#include "TextureManager.h"
#include "BZDBCache.h"
#include "BzMaterial.h"
#include "TextureMatrix.h"
#include "ParseColor.h"
#include "BZDBCache.h"
#include "OpenGLAPI.h"
#include "VBO_Drawing.h"

// local headers
#include "daylight.h"
#include "stars.h"
#include "MainWindow.h"
#include "SceneNode.h"
#include "effectsRenderer.h"
#include "World.h"

static const glm::vec2 squareShape[4] =
{
    {  1.0f,  1.0f }, { -1.0f,  1.0f },
    { -1.0f, -1.0f }, {  1.0f, -1.0f }
};


glm::vec3 BackgroundRenderer::skyPyramid[5];
const GLfloat       BackgroundRenderer::cloudRepeats = 3.0f;
static const int    NumMountainFaces = 16;

glm::vec4       BackgroundRenderer::groundColor[4];
glm::vec4       BackgroundRenderer::groundColorInv[4];

const glm::vec4 BackgroundRenderer::defaultGroundColor[4] =
{
    { 0.0f, 0.35f, 0.0f, 1.0f },
    { 0.0f, 0.20f, 0.0f, 1.0f },
    { 1.0f, 1.00f, 1.0f, 1.0f },
    { 1.0f, 1.00f, 1.0f, 1.0f }
};
const glm::vec4 BackgroundRenderer::defaultGroundColorInv[4] =
{
    { 0.35f, 0.00f, 0.35f, 1.0f },
    { 0.20f, 0.00f, 0.20f, 1.0f },
    { 1.00f, 1.00f, 1.00f, 1.0f },
    { 1.00f, 1.00f, 1.00f, 1.0f }
};

BackgroundRenderer::BackgroundRenderer() :
    blank(false),
    invert(false),
    bottomVBO(Vertex_Chunk::VTC, 4),
    topVBO(Vertex_Chunk::VTC, 4),
    leftVBO(Vertex_Chunk::VTC, 4),
    frontVBO(Vertex_Chunk::VTC, 4),
    rightVBO(Vertex_Chunk::VTC, 4),
    backVBO(Vertex_Chunk::VTC, 4),
    sunInSky(Vertex_Chunk::VC, 6),
    sunSet1(Vertex_Chunk::VC, 4),
    sunSet2(Vertex_Chunk::VC, 12),
    gridSpacing(60.0f), // meters
    gridCount(4.0f),
    mountainsAvailable(false),
    numMountainTextures(0),
    mountainsGState(NULL),
    mountainsList(NULL),
    cloudDriftU(0.0f),
    cloudDriftV(0.0f),
    opaqueCloud(Vertex_Chunk::VT, 4),
    cloudsList(Vertex_Chunk::VTC, 10),
    sunList(Vertex_Chunk::V, 21),
    starList(Vertex_Chunk::VC, NumStars)
{
    static bool init = false;
    OpenGLGStateBuilder gstate;
    static const auto black = glm::vec3(0.0f);
    static const auto white = glm::vec3(1.0f);
    OpenGLMaterial defaultMaterial(black, black, 0.0f);
    OpenGLMaterial rainMaterial(white, white, 0.0f);

    // initialize global to class stuff
    if (!init)
    {
        init = true;
        resizeSky();
    }

    // initialize the celestial vectors
    static const auto up = glm::vec3(0.0f, 0.0f, 1.0f);
    sunDirection  = up;
    moonDirection = up;

    // make ground materials
    setupSkybox();
    setupGroundMaterials();

    TextureManager &tm = TextureManager::instance();

    // make grid stuff
    gstate.reset();
    gstate.setBlending();
    gstate.setSmoothing();
    gridGState = gstate.getState();

    // make receiver stuff
    gstate.reset();
    gstate.setShading();
    gstate.setBlending((GLenum)GL_SRC_ALPHA, (GLenum)GL_ONE);
    receiverGState = gstate.getState();

    // sun shadow stuff
    gstate.reset();
    gstate.setStipple(0.5f);
    gstate.disableCulling();
    sunShadowsGState = gstate.getState();

    /* useMoonTexture = BZDBCache::texture && (BZDB.eval("useQuality")>2);
     int moonTexture = -1;
     if (useMoonTexture) {
       moonTexture = tm.getTextureID( "moon" );
       useMoonTexture = moonTexture>= 0;
     }*/
    // sky stuff
    gstate.reset();
    gstate.setShading();
    skyGState = gstate.getState();
    gstate.reset();
    sunGState = gstate.getState();
    gstate.reset();
    gstate.setBlending((GLenum)GL_ONE, (GLenum)GL_ONE);
// if (useMoonTexture)
//   gstate.setTexture(*moonTexture);
    moonGState[0] = gstate.getState();
    gstate.reset();
// if (useMoonTexture)
//   gstate.setTexture(*moonTexture);
    moonGState[1] = gstate.getState();
    gstate.reset();
    starGState[0] = gstate.getState();
    gstate.reset();
    gstate.setBlending();
    gstate.setSmoothing();
    starGState[1] = gstate.getState();

    // make cloud stuff
    cloudsAvailable = false;
    int cloudsTexture = tm.getTextureID( "clouds" );
    if (cloudsTexture >= 0)
    {
        cloudsAvailable = true;
        gstate.reset();
        gstate.setShading();
        gstate.setBlending((GLenum)GL_SRC_ALPHA, (GLenum)GL_ONE_MINUS_SRC_ALPHA);
        gstate.setMaterial(defaultMaterial);
        gstate.setTexture(cloudsTexture);
        gstate.setAlphaFunc();
        cloudsGState = gstate.getState();
    }

    // rain stuff
    weather.init();
    // effects
    EFFECTS.init();

    // make mountain stuff
    mountainsAvailable = false;
    {
        int mountainTexture;
        int height = 0;
        int i;

        numMountainTextures = 0;
        while (1)
        {
            char text[256];
            sprintf (text, "mountain%d", numMountainTextures + 1);
            mountainTexture = tm.getTextureID (text, false);
            if (mountainTexture < 0)
                break;
            const ImageInfo & info = tm.getInfo (mountainTexture);
            height = info.y;
            numMountainTextures++;
        }

        if (numMountainTextures > 0)
        {
            mountainsAvailable = true;

            // prepare common gstate
            gstate.reset ();
            gstate.setShading ();
            gstate.setBlending ();
            gstate.setMaterial (defaultMaterial);
            gstate.setAlphaFunc ();

            // find power of two at least as large as height
            int scaledHeight = 1;
            while (scaledHeight < height)
                scaledHeight <<= 1;

            // choose minimum width
            int minWidth = scaledHeight;
            if (minWidth > scaledHeight)
                minWidth = scaledHeight;
            mountainsMinWidth = minWidth;

            // prepare each texture
            mountainsGState = new OpenGLGState[numMountainTextures];
            mountainsList = new Vertex_Chunk[numMountainTextures];
            for (i = 0; i < numMountainTextures; i++)
            {
                char text[256];
                sprintf (text, "mountain%d", i + 1);
                gstate.setTexture (tm.getTextureID (text));
                mountainsGState[i] = gstate.getState ();
            }
        }
    }

    // create display lists
    doInitDisplayLists();

    // reset the sky color when it changes
    BZDB.addCallback("_skyColor", bzdbCallback, this);

    // recreate display lists when context is recreated
    OpenGLGState::registerContextInitializer(freeContext, initContext,
            (void*)this);

    notifyStyleChange();
}

BackgroundRenderer::~BackgroundRenderer()
{
    BZDB.removeCallback("_skyColor", bzdbCallback, this);
    OpenGLGState::unregisterContextInitializer(freeContext, initContext,
            (void*)this);
    delete[] mountainsGState;
    delete[] mountainsList;
}


void BackgroundRenderer::bzdbCallback(const std::string& name, void* data)
{
    BackgroundRenderer* br = (BackgroundRenderer*) data;
    if (name == "_skyColor")
    {
        br->setSkyColors();
        br->prepareSky();
    }

    return;
}


void BackgroundRenderer::setupGroundMaterials()
{
    TextureManager &tm = TextureManager::instance();

    // see if we have a map specified material
    const BzMaterial* bzmat = MATERIALMGR.findMaterial("GroundMaterial");

    groundTextureID = -1;
    groundTextureMatrix = NULL;

    if (bzmat == NULL)
    {
        // default ground material
        for (int i = 0; i < 4; i++)
            groundColor[i] = defaultGroundColor[i];
        groundTextureID = tm.getTextureID(BZDB.get("stdGroundTexture").c_str(), true);
    }
    else
    {
        // map specified material
        bzmat->setReference();
        for (int i = 0; i < 4; i++)
            groundColor[i] = bzmat->getDiffuse();
        if (bzmat->getTextureCount() > 0)
        {
            groundTextureID = tm.getTextureID(bzmat->getTextureLocal(0).c_str(), false);
            if (groundTextureID < 0)
            {
                // use the default as a backup (default color too)
                for (int i = 0; i < 4; i++)
                    groundColor[i] = defaultGroundColor[i];
                groundTextureID = tm.getTextureID(BZDB.get("stdGroundTexture").c_str(), true);
            }
            else
            {
                // only apply the texture matrix if the texture is valid
                const int texMatId = bzmat->getTextureMatrix(0);
                const TextureMatrix* texmat = TEXMATRIXMGR.getMatrix(texMatId);
                if (texmat != NULL)
                    groundTextureMatrix = texmat->getMatrix();
            }
        }
    }

    static const auto black = glm::vec3(0.0f);
    OpenGLMaterial defaultMaterial(black, black, 0.0f);

    OpenGLGStateBuilder gb;

    // ground gstates
    gb.reset();
    groundGState[0] = gb.getState();
    gb.reset();
    gb.setMaterial(defaultMaterial);
    groundGState[1] = gb.getState();
    gb.reset();
    gb.setTexture(groundTextureID);
    gb.setTextureMatrix(groundTextureMatrix);
    groundGState[2] = gb.getState();
    gb.reset();
    gb.setMaterial(defaultMaterial);
    gb.setTexture(groundTextureID);
    gb.setTextureMatrix(groundTextureMatrix);
    groundGState[3] = gb.getState();


    // default inverted ground material
    int groundInvTextureID = -1;
    for (int i = 0; i < 4; i++)
        groundColorInv[i] = defaultGroundColorInv[i];
    if (groundInvTextureID < 0)
        groundInvTextureID = tm.getTextureID(BZDB.get("zoneGroundTexture").c_str(), false);

    // inverted ground gstates
    gb.reset();
    invGroundGState[0] = gb.getState();
    gb.reset();
    gb.setMaterial(defaultMaterial);
    invGroundGState[1] = gb.getState();
    gb.reset();
    gb.setTexture(groundInvTextureID);
    invGroundGState[2] = gb.getState();
    gb.reset();
    gb.setMaterial(defaultMaterial);
    gb.setTexture(groundInvTextureID);
    invGroundGState[3] = gb.getState();

    return;
}


void BackgroundRenderer::notifyStyleChange()
{
    if (BZDBCache::texture)
    {
        if (BZDBCache::lighting)
            styleIndex = 3;
        else
            styleIndex = 2;
    }
    else
    {
        if (BZDBCache::lighting)
            styleIndex = 1;
        else
            styleIndex = 0;
    }

    // some stuff is drawn only for certain states
    cloudsVisible = (styleIndex >= 2 && cloudsAvailable);
    mountainsVisible = (styleIndex >= 2 && mountainsAvailable);
    shadowsVisible = BZDB.isTrue("shadows");
    starGStateIndex = BZDB.isTrue("smooth");

    // fixup gstates
    OpenGLGStateBuilder gstate;
    gstate.reset();
    if (BZDB.isTrue("smooth"))
    {
        gstate.setBlending();
        gstate.setSmoothing();
    }
    gridGState = gstate.getState();
}


void BackgroundRenderer::resize()
{
    resizeSky();
    doFreeDisplayLists();
    doInitDisplayLists();
}


void BackgroundRenderer::setCelestial(const glm::vec3 &sunDir,
                                      const glm::vec3 &moonDir)
{
    // set sun and moon positions
    sunDirection  = sunDir;
    moonDirection = moonDir;

    makeCelestialLists();

    return;
}


void BackgroundRenderer::setSkyColors()
{
    // change sky colors according to the sun position
    glm::vec3 colors[4];
    getSkyColor(sunDirection, colors);

    skyZenithColor      = colors[0];
    skySunDirColor      = colors[1];
    skyAntiSunDirColor  = colors[2];
    skyCrossSunDirColor = colors[3];

    return;
}


void BackgroundRenderer::drawSunXForm()
{
    // draw pretransformed display list for sun
    glPushMatrix();
    glRotatef(sunAzimuth,   0.0f,  0.0f, 1.0f);
    glRotatef(sunElevation, 0.0f, -1.0f, 0.0f);
    sunList.draw(GL_TRIANGLE_FAN);
    glPopMatrix();
}


void BackgroundRenderer::makeCelestialLists()
{
    setSkyColors();

    // get a few other things concerning the sky
    doShadows = areShadowsCast(sunDirection);
    doStars = areStarsVisible(sunDirection);
    doSunset = getSunsetTop(sunDirection, sunsetTop);

    prepareSky();

    // make pretransformed display list for sun
    sunAzimuth   = atan2f(sunDirection[1], (sunDirection[0])) * 180.0 / M_PI;
    sunElevation = asinf(sunDirection[2]) * 180.0 / M_PI;

    // compute display list for moon
    float coverage = glm::dot(moonDirection, sunDirection);
    // hack coverage to lean towards full
    coverage = (coverage < 0.0f) ? -sqrtf(-coverage) : coverage * coverage;
    float worldSize = BZDBCache::worldSize;
    const float moonRadius = 2.0f * worldSize *
                             atanf((float)((60.0 * M_PI / 180.0) / 60.0));
    // limbAngle is dependent on moon position but sun is so much farther
    // away that the moon's position is negligible.  rotate sun and moon
    // so that moon is on the horizon in the +x direction, then compute
    // the angle to the sun position in the yz plane.
    float sun2[3];
    moonAzimuth = atan2f(moonDirection[1], moonDirection[0]);
    moonAltitude = asinf(moonDirection[2]);
    sun2[0] = sunDirection[0] * cosf(moonAzimuth) + sunDirection[1] * sinf(moonAzimuth);
    sun2[1] = sunDirection[1] * cosf(moonAzimuth) - sunDirection[0] * sinf(moonAzimuth);
    sun2[2] = sunDirection[2] * cosf(moonAltitude) - sun2[0] * sinf(moonAltitude);
    limbAngle = atan2f(sun2[2], sun2[1]);

    const int moonSegements = 64;
    moonList = Vertex_Chunk(Vertex_Chunk::V, moonSegements * 2);
    {
        std::vector<glm::vec3> moonVertex;

        auto vertex = glm::vec3(2.0f * worldSize, 0.0f, -moonRadius);
        // glTexCoord2f(0,-1);
        moonVertex.push_back(vertex);
        for (int i = 0; i < moonSegements-1; i++)
        {
            const float angle = (float)(0.5 * M_PI * double(i-(moonSegements/2)-1) / (moonSegements/2.0));
            float sinAngle = sinf(angle);
            float cosAngle = cosf(angle);
            // glTexCoord2f(coverage*cosAngle,sinAngle);
            vertex.y = coverage * moonRadius * cosAngle;
            vertex.z = moonRadius * sinAngle;
            moonVertex.push_back(vertex);

            // glTexCoord2f(cosAngle,sinAngle);
            vertex.y = moonRadius * cosAngle;
            moonVertex.push_back(vertex);
        }
        // glTexCoord2f(0,1);
        vertex.y = 0.0f;
        vertex.z = moonRadius;
        moonVertex.push_back(vertex);
        moonList.vertexData(moonVertex);
    }

    return;
}


void BackgroundRenderer::drawMoon()
{
    glPushMatrix();
    glRotatef(moonAzimuth * 180.0 / M_PI,  0.0f, 0.0f, 1.0f);
    glRotatef(moonAltitude * 180.0 / M_PI, 0.0f, -1.0f, 0.0f);
    glRotatef((float)(limbAngle * 180.0 / M_PI), 1.0f, 0.0f, 0.0f);
    moonList.draw(GL_TRIANGLE_STRIP);
    glPopMatrix();
}


void BackgroundRenderer::drawStarXForm(const SceneRenderer& renderer)
{
    float worldSize = BZDBCache::worldSize;
    // make pretransformed display list for stars
    {
        glPushMatrix();
        glMultMatrixf(renderer.getCelestialTransform());
        glScalef(worldSize, worldSize, worldSize);
        starList.draw(GL_POINTS);
        glPopMatrix();
    }
}


void BackgroundRenderer::addCloudDrift(GLfloat uDrift, GLfloat vDrift)
{
    cloudDriftU += 0.01f * uDrift / cloudRepeats;
    cloudDriftV += 0.01f * vDrift / cloudRepeats;
    if (cloudDriftU > 1.0f) cloudDriftU -= 1.0f;
    else if (cloudDriftU < 0.0f) cloudDriftU += 1.0f;
    if (cloudDriftV > 1.0f) cloudDriftV -= 1.0f;
    else if (cloudDriftV < 0.0f) cloudDriftV += 1.0f;
}


void BackgroundRenderer::renderSky(SceneRenderer& renderer,
                                   bool mirror)
{
    if (!BZDBCache::drawSky)
        return;
    {
        drawSky(renderer, mirror);
    }
}


void BackgroundRenderer::renderGround()
{
    {
        drawGround();
    }
}


void BackgroundRenderer::renderGroundEffects(SceneRenderer& renderer,
        bool drawingMirror)
{
    // zbuffer should be disabled.  either everything is coplanar with
    // the ground or is drawn back to front and is occluded by everything
    // drawn after it.  also use projection with very far clipping plane.

    // only draw the grid lines if texturing is disabled
    if (!BZDBCache::texture)
        drawGroundGrid(renderer);

    if (!blank)
    {
        const World *_world = World::getWorld();
        if (doShadows && shadowsVisible && !drawingMirror && _world)
            drawGroundShadows(renderer);

        // draw light receivers on ground (little meshes under light sources so
        // the ground gets illuminated).  this is necessary because lighting is
        // performed only at a vertex, and the ground's vertices are a few
        // kilometers away.
        if (BZDBCache::lighting &&
                !drawingMirror && BZDBCache::drawGroundLights)
        {
            if (BZDBCache::tessellation && (renderer.useQuality() >= 3))
            {
//    (BZDB.get(StateDatabase::BZDB_FOGMODE) == "none")) {
                // not really tessellation, but it is tied to the "Best" lighting,
                // avoid on foggy maps, because the blending function accumulates
                // too much brightness.
                drawAdvancedGroundReceivers(renderer);
            }
            else
                drawGroundReceivers(renderer);
        }

        {
            // light the mountains (so that they get dark when the sun goes down).
            // don't do zbuffer test since they occlude all drawn before them and
            // are occluded by all drawn after.
            if (mountainsVisible && BZDBCache::drawMountains)
                drawMountains();

            // draw clouds
            if (cloudsVisible && BZDBCache::drawClouds)
            {
                cloudsGState.setState();
                glMatrixMode(GL_TEXTURE);
                glPushMatrix();
                glTranslatef(cloudDriftU, cloudDriftV, 0.0f);
                glNormal3f(0.0f, 0.0f, 1.0f);
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                opaqueCloud.draw(GL_TRIANGLE_STRIP);
                cloudsList.draw(GL_TRIANGLE_STRIP);
                glLoadIdentity();   // maybe works around bug in some systems
                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
            }
        }
    }
}


void BackgroundRenderer::renderEnvironment(SceneRenderer& renderer, bool update)
{
    if (blank)
        return;

    if (update)
        weather.update();
    weather.draw(renderer);

    if (update)
        EFFECTS.update();
    EFFECTS.draw(renderer);
}


void BackgroundRenderer::resizeSky()
{
    // sky pyramid must fit inside far clipping plane
    // (adjusted for the deepProjection matrix)
    const GLfloat skySize = 3.0f * BZDBCache::worldSize;
    for (int i = 0; i < 4; i++)
        skyPyramid[i] = glm::vec3(skySize * squareShape[i], 0.0f);
    skyPyramid[4] = glm::vec3(0.0f, 0.0f, skySize);
    prepareSky();
}


void BackgroundRenderer::setupSkybox()
{
    haveSkybox = false;

    int i;
    const char *skyboxNames[6] =
    {
        "LeftSkyboxMaterial",
        "FrontSkyboxMaterial",
        "RightSkyboxMaterial",
        "BackSkyboxMaterial",
        "TopSkyboxMaterial",
        "BottomSkyboxMaterial"
    };
    TextureManager& tm = TextureManager::instance();
    const BzMaterial* bzmats[6] = {NULL, NULL, NULL, NULL, NULL, NULL};

    // try to load the textures
    for (i = 0; i < 6; i++)
    {
        bzmats[i] = MATERIALMGR.findMaterial(skyboxNames[i]);
        if ((bzmats[i] == NULL) || (bzmats[i]->getTextureCount() <= 0))
            break;
        skyboxTexID[i] = tm.getTextureID(bzmats[i]->getTextureLocal(0).c_str());
        if (skyboxTexID[i] < 0)
            break;
    }

    // unload textures if they were not all successful
    if (i != 6)
    {
        while (i >= 0)
        {
            if ((bzmats[i] != NULL) && (bzmats[i]->getTextureCount() > 0))
            {
                // NOTE: this could delete textures the might be used elsewhere
                tm.removeTexture(bzmats[i]->getTextureLocal(0).c_str());
            }
            i--;
        }
        return;
    }

    // reference map specified materials
    for (i = 0; i < 6; i++)
        bzmats[i]->setReference();

    // setup the corner colors
    const int cornerFaces[8][3] =
    {
        {5, 0, 1}, {5, 1, 2}, {5, 2, 3}, {5, 3, 0},
        {4, 0, 1}, {4, 1, 2}, {4, 2, 3}, {4, 3, 0}
    };
    for (i = 0; i < 8; i++)
    {
        skyboxColor[i] = glm::vec3(0.0f);
        for (int f = 0; f < 3; f++)
            skyboxColor[i] += glm::vec3(bzmats[cornerFaces[i][f]]->getDiffuse());
        skyboxColor[i] /= 3.0f;
    }

    haveSkybox = true;

    prepareSkybox();
    return;
}

void BackgroundRenderer::prepareSkybox()
{
    const glm::vec3 verts[8] =
    {
        {-1.0f, -1.0f, -1.0f},
        {+1.0f, -1.0f, -1.0f},
        {+1.0f, +1.0f, -1.0f},
        {-1.0f, +1.0f, -1.0f},
        {-1.0f, -1.0f, +1.0f},
        {+1.0f, -1.0f, +1.0f},
        {+1.0f, +1.0f, +1.0f},
        {-1.0f, +1.0f, +1.0f}
    };

    const glm::vec2 txcds[4] =
    {
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 1.0f}
    };

    const auto &color = skyboxColor;

    glm::vec2 t[4];
    glm::vec4 c[4];
    glm::vec3 v[4];

    t[0] = txcds[0];
    c[0] = glm::vec4(color[2], 1.0f);
    v[0] = verts[2];

    t[1] = txcds[1];
    c[1] = glm::vec4(color[3], 1.0f);
    v[1] = verts[3];

    t[2] = txcds[3];
    c[2] = glm::vec4(color[1], 1.0f);
    v[2] = verts[1];

    t[3] = txcds[2];
    c[3] = glm::vec4(color[0], 1.0f);
    v[3] = verts[0];

    bottomVBO.textureData(t);
    bottomVBO.colorData(c);
    bottomVBO.vertexData(v);

    t[0] = txcds[0];
    c[0] = glm::vec4(color[5], 1.0f);
    v[0] = verts[5];

    t[1] = txcds[1];
    c[1] = glm::vec4(color[4], 1.0f);
    v[1] = verts[4];

    t[2] = txcds[3];
    c[2] = glm::vec4(color[6], 1.0f);
    v[2] = verts[6];

    t[3] = txcds[2];
    c[3] = glm::vec4(color[7], 1.0f);
    v[3] = verts[7];

    topVBO.textureData(t);
    topVBO.colorData(c);
    topVBO.vertexData(v);

    t[0] = txcds[0];
    c[0] = glm::vec4(color[0], 1.0f);
    v[0] = verts[0];

    t[1] = txcds[1];
    c[1] = glm::vec4(color[3], 1.0f);
    v[1] = verts[3];

    t[2] = txcds[3];
    c[2] = glm::vec4(color[4], 1.0f);
    v[2] = verts[4];

    t[3] = txcds[2];
    c[3] = glm::vec4(color[7], 1.0f);
    v[3] = verts[7];

    leftVBO.textureData(t);
    leftVBO.colorData(c);
    leftVBO.vertexData(v);

    t[0] = txcds[0];
    c[0] = glm::vec4(color[1], 1.0f);
    v[0] = verts[1];

    t[1] = txcds[1];
    c[1] = glm::vec4(color[0], 1.0f);
    v[1] = verts[0];

    t[2] = txcds[3];
    c[2] = glm::vec4(color[5], 1.0f);
    v[2] = verts[5];

    t[3] = txcds[2];
    c[3] = glm::vec4(color[4], 1.0f);
    v[3] = verts[4];

    frontVBO.textureData(t);
    frontVBO.colorData(c);
    frontVBO.vertexData(v);

    t[0] = txcds[0];
    c[0] = glm::vec4(color[2], 1.0f);
    v[0] = verts[2];

    t[1] = txcds[1];
    c[1] = glm::vec4(color[1], 1.0f);
    v[1] = verts[1];

    t[2] = txcds[3];
    c[2] = glm::vec4(color[6], 1.0f);
    v[2] = verts[6];

    t[3] = txcds[2];
    c[3] = glm::vec4(color[5], 1.0f);
    v[3] = verts[5];

    rightVBO.textureData(t);
    rightVBO.colorData(c);
    rightVBO.vertexData(v);

    t[0] = txcds[0];
    c[0] = glm::vec4(color[3], 1.0f);
    v[0] = verts[3];

    t[1] = txcds[1];
    c[1] = glm::vec4(color[2], 1.0f);
    v[1] = verts[2];

    t[2] = txcds[3];
    c[2] = glm::vec4(color[7], 1.0f);
    v[2] = verts[7];

    t[3] = txcds[2];
    c[3] = glm::vec4(color[6], 1.0f);
    v[3] = verts[6];

    backVBO.textureData(t);
    backVBO.colorData(c);
    backVBO.vertexData(v);
}

void BackgroundRenderer::drawSkybox()
{
    // sky box must fit inside far clipping plane
    // (adjusted for the deepProjection matrix)
    const float d = 3.0f * BZDBCache::worldSize;

    TextureManager& tm = TextureManager::instance();

    OpenGLGState::resetState();

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);

    glPushMatrix();
    glScalef(d, d, d);
    if (!BZDBCache::drawGround)
    {
        tm.bind(skyboxTexID[5]); // bottom
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        bottomVBO.draw(GL_TRIANGLE_STRIP);
    }

    tm.bind(skyboxTexID[4]); // top
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    topVBO.draw(GL_TRIANGLE_STRIP);

    tm.bind(skyboxTexID[0]); // left
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    leftVBO.draw(GL_TRIANGLE_STRIP);

    tm.bind(skyboxTexID[1]); // front
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    frontVBO.draw(GL_TRIANGLE_STRIP);

    tm.bind(skyboxTexID[2]); // right
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    rightVBO.draw(GL_TRIANGLE_STRIP);

    tm.bind(skyboxTexID[3]); // back
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    backVBO.draw(GL_TRIANGLE_STRIP);
    glPopMatrix();

    glShadeModel(GL_FLAT);
    glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
}


void BackgroundRenderer::prepareSky()
{
    glm::vec4 c[12];
    glm::vec3 v[12];

    c[0] = glm::vec4(skyZenithColor, 1.0f);
    v[0] = skyPyramid[4];
    c[1] = glm::vec4(skyCrossSunDirColor, 1.0f);
    v[1] = skyPyramid[0];
    c[2] = glm::vec4(skySunDirColor, 1.0f);
    v[2] = skyPyramid[3];
    c[3] = glm::vec4(skyCrossSunDirColor, 1.0f);
    v[3] = skyPyramid[2];
    c[4] = glm::vec4(skyAntiSunDirColor, 1.0f);
    v[4] = skyPyramid[1];
    c[5] = glm::vec4(skyCrossSunDirColor, 1.0f);
    v[5] = skyPyramid[0];
    sunInSky.colorData(c);
    sunInSky.vertexData(v);

    c[0] = glm::vec4(skyZenithColor, 1.0f);
    v[0] = skyPyramid[4];
    c[1] = glm::vec4(skyCrossSunDirColor, 1.0f);
    v[1] = skyPyramid[2];
    c[2] = glm::vec4(skyAntiSunDirColor, 1.0f);
    v[2] = skyPyramid[1];
    c[3] = glm::vec4(skyCrossSunDirColor, 1.0f);
    v[3] = skyPyramid[0];
    sunSet1.colorData(c);
    sunSet1.vertexData(v);

    glm::vec3 sunsetTopPoint;
    sunsetTopPoint[0] = skyPyramid[3][0] * (1.0f - sunsetTop);
    sunsetTopPoint[1] = skyPyramid[3][1] * (1.0f - sunsetTop);
    sunsetTopPoint[2] = skyPyramid[4][2] * sunsetTop;

    c[0] = glm::vec4(skyZenithColor, 1.0f);
    v[0] = skyPyramid[4];
    c[1] = glm::vec4(skyCrossSunDirColor, 1.0f);
    v[1] = skyPyramid[0];
    c[2] = glm::vec4(skyZenithColor, 1.0f);
    v[2] = sunsetTopPoint;
    c[3] = glm::vec4(skyZenithColor, 1.0f);
    v[3] = skyPyramid[4];
    c[4] = glm::vec4(skyZenithColor, 1.0f);
    v[4] = sunsetTopPoint;
    c[5] = glm::vec4(skyCrossSunDirColor, 1.0f);
    v[5] = skyPyramid[2];
    c[6] = glm::vec4(skyZenithColor, 1.0f);
    v[6] = sunsetTopPoint;
    c[7] = glm::vec4(skyCrossSunDirColor, 1.0f);
    v[7] = skyPyramid[0];
    c[8] = glm::vec4(skySunDirColor, 1.0f);
    v[8] = skyPyramid[3];
    c[9] = glm::vec4(skyCrossSunDirColor, 1.0f);
    v[9] = skyPyramid[2];
    c[10] = glm::vec4(skyZenithColor, 1.0f);
    v[10] = sunsetTopPoint;
    c[11] = glm::vec4(skySunDirColor, 1.0f);
    v[11] = skyPyramid[3];
    sunSet2.colorData(c);
    sunSet2.vertexData(v);
}


void BackgroundRenderer::drawSky(SceneRenderer& renderer, bool mirror)
{
    glPushMatrix();

    const bool doSkybox = haveSkybox;

    if (!doSkybox)
    {
        // rotate sky so that horizon-point-toward-sun-color is actually
        // toward the sun
        glRotatef((GLfloat)((atan2f(sunDirection[1], sunDirection[0]) * 180.0 + 135.0) / M_PI),
                  0.0f, 0.0f, 1.0f);

        // draw sky
        skyGState.setState();
        if (!doSunset)
        {
            // just a pyramid
            sunInSky.draw(GL_TRIANGLE_FAN);
        }
        else
        {
            // overall shape is a pyramid, but the solar sides are two
            // triangles each.  the top triangle is all zenith color,
            // the bottom goes from zenith to sun-dir color.
            sunSet1.draw(GL_TRIANGLE_FAN);

            sunSet2.draw(GL_TRIANGLES);
        }
    }

    glLoadIdentity();
    renderer.getViewFrustum().executeOrientation();

    const bool useClipPlane = (mirror && (doSkybox || BZDBCache::drawCelestial));

    if (useClipPlane)
        glEnable(GL_CLIP_PLANE0);

    if (doSkybox)
        drawSkybox();

    if (BZDBCache::drawCelestial)
    {
        if (sunDirection[2] > -0.009f)
        {
            sunGState.setState();
            glColor(renderer.getSunScaledColor());
            drawSunXForm();
        }

        if (doStars)
        {
            starGState[starGStateIndex].setState();
            drawStarXForm(renderer);
        }

        if (moonDirection[2] > -0.009f)
        {
            moonGState[doStars ? 1 : 0].setState();
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            //   if (useMoonTexture)
            //     glEnable(GL_TEXTURE_2D);
            drawMoon();
        }

    }

    if (useClipPlane)
        glDisable(GL_CLIP_PLANE0);

    glPopMatrix();
}


void BackgroundRenderer::drawGround()
{
    if (!BZDBCache::drawGround)
        return;

    {
        // draw ground
        glNormal3f(0.0f, 0.0f, 1.0f);
        if (invert)
        {
            glColor(groundColorInv[styleIndex]);
            invGroundGState[styleIndex].setState();
        }
        else
        {
            glm::vec4 color;
            if (BZDB.isSet("GroundOverideColor") &&
                    parseColorString(BZDB.get("GroundOverideColor"), color))
                glColor(color);
            else
                glColor(groundColor[styleIndex]);
            groundGState[styleIndex].setState();
        }

        {
            drawGroundCentered();
        }
    }
}

void BackgroundRenderer::drawGroundCentered()
{
    const float groundSize = 10.0f * BZDBCache::worldSize;
    const float centerSize = 128.0f;

    const ViewFrustum& frustum = RENDERER.getViewFrustum();
    auto center = glm::vec2(frustum.getEye());
    const float minDist = -groundSize + centerSize;
    const float maxDist = +groundSize - centerSize;
    center = glm::clamp(center, minDist, maxDist);

    const glm::vec2 vertices[8] =
    {
        { -groundSize, -groundSize },
        { +groundSize, -groundSize },
        { +groundSize, +groundSize },
        { -groundSize, +groundSize },
        { center[0] - centerSize, center[1] - centerSize },
        { center[0] + centerSize, center[1] - centerSize },
        { center[0] + centerSize, center[1] + centerSize },
        { center[0] - centerSize, center[1] + centerSize }
    };

    const float repeat = BZDB.eval("groundHighResTexRepeat");
    const int indices[5][4] =
    {
        { 4, 5, 7, 6 },
        { 0, 1, 4, 5 },
        { 1, 2, 5, 6 },
        { 2, 3, 6, 7 },
        { 3, 0, 7, 4 },
    };

    glNormal3f(0.0f, 0.0f, 1.0f);
    {
        static Vertex_Chunk ground(Vertex_Chunk::VT, 4);
        for (int q = 0; q < 5; q++)
        {
            glm::vec2 t[4];
            glm::vec3 v[4];
            for (int c = 0; c < 4; c++)
            {
                const int index = indices[q][c];
                t[c] = vertices[index] * repeat;
                v[c] = glm::vec3(vertices[index], 0.0f);
            }
            ground.textureData(t);
            ground.vertexData(v);
            ground.draw(GL_TRIANGLE_STRIP);
        }
    }

    return;
}


void BackgroundRenderer::drawGroundGrid(
    SceneRenderer& renderer)
{
    const auto &pos = renderer.getViewFrustum().getEye();
    const GLfloat xhalf = gridSpacing * (gridCount + floorf(pos[2] / 4.0f));
    const GLfloat yhalf = gridSpacing * (gridCount + floorf(pos[2] / 4.0f));
    const GLfloat x0 = floorf(pos[0] / gridSpacing) * gridSpacing;
    const GLfloat y0 = floorf(pos[1] / gridSpacing) * gridSpacing;
    GLfloat i;

    gridGState.setState();

    // x lines
    if (doShadows)
        glColor4f(0.0f, 0.75f, 0.5f, 1.0f);
    else
        glColor4f(0.0f, 0.4f, 0.3f, 1.0f);
    glPushMatrix();
    glTranslatef(x0 - xhalf, y0, 0.0f);
    glScalef(1.0f, yhalf, 0.0f);
    for (i = -xhalf; i <= xhalf; i += gridSpacing)
    {
        DRAWER.simmetricLineY();
        glTranslatef(gridSpacing, 0.0f, 0.0f);
    }
    glPopMatrix();

    /* z lines */
    if (doShadows)
        glColor4f(0.5f, 0.75f, 0.0f, 1.0f);
    else
        glColor4f(0.3f, 0.4f, 0.0f, 1.0f);
    glPushMatrix();
    glTranslatef(x0, y0 - yhalf, 0.0f);
    glScalef(xhalf, 1.0f, 0.0f);
    for (i = -yhalf; i <= yhalf; i += gridSpacing)
    {
        DRAWER.simmetricLineX();
        glTranslatef(0.0f, gridSpacing, 0.0f);
    }
    glPopMatrix();
}

void BackgroundRenderer::drawGroundShadows(
    SceneRenderer& renderer)
{
    // draw sun shadows -- always stippled so overlapping shadows don't
    // accumulate darkness.  make and multiply by shadow projection matrix.
    GLfloat shadowProjection[16];
    shadowProjection[0] = shadowProjection[5] = shadowProjection[15] = 1.0f;
    shadowProjection[8] = -sunDirection[0] / sunDirection[2];
    shadowProjection[9] = -sunDirection[1] / sunDirection[2];
    shadowProjection[1] = shadowProjection[2] =
                              shadowProjection[3] = shadowProjection[4] =
                                          shadowProjection[6] = shadowProjection[7] =
                                                  shadowProjection[10] = shadowProjection[11] =
                                                          shadowProjection[12] = shadowProjection[13] =
                                                                  shadowProjection[14] = 0.0f;
    glPushMatrix();
    glMultMatrixf(shadowProjection);

    // disable color updates
    SceneNode::setColorOverride(true);

    if (BZDBCache::stencilShadows)
    {
        OpenGLGState::resetState();
        const float shadowAlpha = BZDB.eval("shadowAlpha");
        glColor4f(0.0f, 0.0f, 0.0f, shadowAlpha);
        if (shadowAlpha < 1.0f)
        {
            // use the stencil to avoid overlapping shadows
            glClearStencil(0);
            glClear(GL_STENCIL_BUFFER_BIT);
            glStencilFunc(GL_NOTEQUAL, 1, 1);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glEnable(GL_STENCIL_TEST);

            // turn on blending, and kill culling
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            glDisable(GL_CULL_FACE);
        }
    }
    else
    {
        // use stippling to avoid overlapping shadows
        sunShadowsGState.setState();
        glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // render those nodes
    renderer.getShadowList().render();

    // revert to OpenGLGState defaults
    if (BZDBCache::stencilShadows)
    {
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        glBlendFunc(GL_ONE, GL_ZERO);
    }

    // enable color updates
    SceneNode::setColorOverride(false);

    OpenGLGState::resetState();

    glPopMatrix();
}


static void setupBlackFog(glm::vec4 &fogColor)
{
    static const auto black = glm::vec4(0.0f);
    fogColor = glGetFogColor();
    glSetFogColor(black);
}


void BackgroundRenderer::drawGroundReceivers(SceneRenderer& renderer)
{
    static const int receiverRings = 4;
    static const int receiverSlices = 8;
    static const float receiverRingSize = 1.2f;   // meters
    static glm::vec2 angle[receiverSlices + 1];
    static Vertex_Chunk circle(Vertex_Chunk::VC, receiverSlices + 2);

    static bool init = false;
    if (!init)
    {
        init = true;
        const float receiverSliceAngle = (float)(2.0 * M_PI / double(receiverSlices));
        glm::vec3 v[receiverSlices + 2];
        v[0] = glm::vec3(0.0f);
        for (int i = 0; i <= receiverSlices; i++)
        {
            angle[i][0] = cosf((float)i * receiverSliceAngle);
            angle[i][1] = sinf((float)i * receiverSliceAngle);
            v[i + 1] = glm::vec3(angle[i] * receiverRingSize, 0.0f);
        }
        circle.vertexData(v);
    }

    const int count = renderer.getNumAllLights();
    if (count == 0)
        return;

    // bright sun dims intensity of ground receivers
    const float B = 1.0f - (0.6f * renderer.getSunBrightness());

    receiverGState.setState();

    // setup black fog
    glm::vec4 fogColor;
    setupBlackFog(fogColor);

    glPushMatrix();
    int i, j;
    for (int k = 0; k < count; k++)
    {
        const OpenGLLight& light = renderer.getLight(k);
        if (light.getOnlyReal())
            continue;

        const auto &pos = light.getPosition();
        const auto &lightColor = light.getColor();
        const GLfloat* atten = light.getAttenuation();

        // point under light
        float d = pos[2];
        float I = B / (atten[0] + d * (atten[1] + d * atten[2]));

        // maximum value
        const float maxVal = glm::compMax(glm::vec3(lightColor));

        // if I is too attenuated, don't bother drawing anything
        if ((I * maxVal) < 0.02f)
            continue;

        // move to the light's position
        glTranslatef(pos[0], pos[1], 0.0f);

        // set the main lighting color
        auto color = lightColor;
        color[3] = I;

        // draw ground receiver, computing lighting at each vertex ourselves
        {
            glm::vec4 c[receiverSlices + 2];
            c[0] = color;

            // inner ring
            d = hypotf(receiverRingSize, pos[2]);
            I = B / (atten[0] + d * (atten[1] + d * atten[2]));
            I *= pos[2] / d;
            color[3] = I;
            for (j = 0; j <= receiverSlices; j++)
                c[j+1] = color;
            circle.colorData(c);
        }
        circle.draw(GL_TRIANGLE_FAN);
        triangleCount += receiverSlices;

        GLfloat outerSize = receiverRingSize;
        d = hypotf(outerSize, pos[2]);
        I = B / (atten[0] + d * (atten[1] + d * atten[2]));
        I *= pos[2] / d;

        std::vector<glm::vec4> c;
        std::vector<glm::vec3> v;
        for (i = 1; i < receiverRings; i++)
        {
            const GLfloat innerSize = outerSize;
            outerSize = receiverRingSize * GLfloat((i + 1) * (i + 1));

            float innerAlpha = I;

            if (i + 1 == receiverRings)
                I = 0.0f;
            else
            {
                d = hypotf(outerSize, pos[2]);
                I = B / (atten[0] + d * (atten[1] + d * atten[2]));
                I *= pos[2] / d;
            }
            float outerAlpha = I;

            {
                for (j = 0; j <= receiverSlices; j++)
                {
                    color[3] = innerAlpha;
                    c.push_back(color);
                    v.push_back(glm::vec3(angle[j] * innerSize, 0.0f));
                    color[3] = outerAlpha;
                    c.push_back(color);
                    v.push_back(glm::vec3(angle[j] * outerSize, 0.0f));
                }
            }
        }
        Vertex_Chunk ring(Vertex_Chunk::VC, v.size());
        ring.colorData(c);
        ring.vertexData(v);
        ring.draw(GL_TRIANGLE_STRIP);
        triangleCount += (receiverSlices * receiverRings * 2);

        glTranslatef(-pos[0], -pos[1], 0.0f);
    }
    glPopMatrix();

    glSetFogColor(fogColor);
}


void BackgroundRenderer::drawAdvancedGroundReceivers(SceneRenderer& renderer)
{
    const float minLuminance = 0.02f;
    static const int receiverSlices = 32;
    static const float receiverRingSize = 0.5f;   // meters
    static glm::vec2 angle[receiverSlices + 1];
    static Vertex_Chunk circle(Vertex_Chunk::VC, receiverSlices + 2);

    static bool init = false;
    if (!init)
    {
        init = true;
        const float receiverSliceAngle = (float)(2.0 * M_PI / double(receiverSlices));
        glm::vec3 v[receiverSlices + 2];
        v[0] = glm::vec3(0.0f);
        for (int i = 0; i <= receiverSlices; i++)
        {
            angle[i][0] = cosf((float)i * receiverSliceAngle);
            angle[i][1] = sinf((float)i * receiverSliceAngle);
            v[i + 1] = glm::vec3(angle[i] * receiverRingSize, 0.0f);
        }
        circle.vertexData(v);
    }

    const int count = renderer.getNumAllLights();
    if (count == 0)
        return;

    // setup the ground tint
    auto gndColor = groundColor[styleIndex];
    glm::vec4 overrideColor;
    if (BZDB.isSet("GroundOverideColor") &&
            parseColorString(BZDB.get("GroundOverideColor"), overrideColor))
        gndColor = overrideColor;

    const bool useTexture = BZDBCache::texture && (groundTextureID >= 0);
    OpenGLGState advGState;
    OpenGLGStateBuilder builder;
    builder.setShading(GL_SMOOTH);
    builder.setBlending((GLenum)GL_ONE, (GLenum)GL_ONE);
    if (useTexture)
    {
        builder.setTexture(groundTextureID);
        builder.setTextureMatrix(groundTextureMatrix);
    }
    advGState = builder.getState();
    advGState.setState();

    // setup black fog
    glm::vec4 fogColor;
    setupBlackFog(fogColor);

    // lazy way to get texcoords
    if (useTexture)
    {
        const float repeat = BZDB.eval("groundHighResTexRepeat");
        const float sPlane[4] = { repeat, 0.0f, 0.0f, 0.0f };
        const float tPlane[4] = { 0.0f, repeat, 0.0f, 0.0f };
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
        glTexGenfv(GL_S, GL_EYE_PLANE, sPlane);
        glTexGenfv(GL_T, GL_EYE_PLANE, tPlane);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
    }

    glPushMatrix();
    int i, j;
    for (int k = 0; k < count; k++)
    {
        const OpenGLLight& light = renderer.getLight(k);
        if (light.getOnlyReal())
            continue;

        // get the light parameters
        const auto &pos = light.getPosition();
        const auto &lightColor = light.getColor();
        const GLfloat* atten = light.getAttenuation();

        // point under light
        float d = pos[2];
        float I = 1.0f / (atten[0] + d * (atten[1] + d * atten[2]));

        // set the main lighting color
        auto baseColor = glm::vec3(gndColor * lightColor);
        if (invert)   // beats me, should just color logic op the static nodes
            baseColor = 1.0f - baseColor;

        // maximum value
        const float maxVal = glm::compMax(baseColor);

        // if I is too attenuated, don't bother drawing anything
        if ((I * maxVal) < minLuminance)
            continue;

        // move to the light's position
        glTranslatef(pos[0], pos[1], 0.0f);

        float innerSize;
        glm::vec3 innerColor;
        float outerSize;
        glm::vec3 outerColor;

        // draw ground receiver, computing lighting at each vertex ourselves
        {
            glm::vec4 c[receiverSlices + 2];
            // center point
            innerColor = I * baseColor;
            c[0] = glm::vec4(innerColor, 1.0f);

            // inner ring
            d = hypotf(receiverRingSize, pos[2]);
            I = 1.0f / (atten[0] + d * (atten[1] + d * atten[2]));
            I *= pos[2] / d; // diffuse angle factor
            outerColor = I * baseColor;
            outerSize = receiverRingSize;
            for (j = 0; j <= receiverSlices; j++)
                c[j + 1] = glm::vec4(outerColor, 1.0f);
            circle.colorData(c);
        }
        circle.draw(GL_TRIANGLE_FAN);
        triangleCount += receiverSlices;

        bool moreRings = true;
        std::vector<glm::vec4> c;
        std::vector<glm::vec3> v;
        for (i = 2; moreRings; i++)
        {
            // inner ring
            innerSize = outerSize;
            innerColor = outerColor;

            // outer ring
            outerSize = receiverRingSize * GLfloat(i * i);
            d = hypotf(outerSize, pos[2]);
            I = 1.0f / (atten[0] + d * (atten[1] + d * atten[2]));
            I *= pos[2] / d; // diffuse angle factor
            if ((I * maxVal) < minLuminance)
            {
                I = 0.0f;
                moreRings = false; // bail after this ring
            }
            outerColor = I * baseColor;

            {
                for (j = 0; j <= receiverSlices; j++)
                {
                    c.push_back(glm::vec4(innerColor, 1.0f));
                    v.push_back(glm::vec3(angle[j] * innerSize, 0.0f));
                    c.push_back(glm::vec4(outerColor, 1.0f));
                    v.push_back(glm::vec3(angle[j] * outerSize, 0.0f));
                }
            }
        }
        Vertex_Chunk ring(Vertex_Chunk::VC, v.size());
        ring.colorData(c);
        ring.vertexData(v);
        ring.draw(GL_TRIANGLE_STRIP);
        triangleCount += (receiverSlices * 2 * (i - 2));

        glTranslatef(-pos[0], -pos[1], 0.0f);
    }
    glPopMatrix();

    if (useTexture)
    {
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }

    glSetFogColor(fogColor);
}


void BackgroundRenderer::drawMountains(void)
{
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    for (int i = 0; i < numMountainTextures; i++)
    {
        mountainsGState[i].setState();
        mountainsList[i].draw(GL_TRIANGLE_STRIP);
    }
}


void BackgroundRenderer::doFreeDisplayLists()
{
    // don't forget the tag-along
    EFFECTS.freeContext();

    return;
}


void BackgroundRenderer::doInitDisplayLists()
{
    int i, j;

    // don't forget the tag-along
    EFFECTS.rebuildContext();

    //
    // sky stuff
    //

    // sun first.  sun is a disk that should be about a half a degree wide
    // with a normal (60 degree) perspective.
    const float worldSize = BZDBCache::worldSize;
    const float sunRadius = (float)(2.0 * worldSize * atanf((float)(60.0*M_PI/180.0)) / 60.0);
    {
        glm::vec3 sunVertex[21];
        {
            sunVertex[0] = glm::vec3(2.0f * worldSize, 0.0f, 0.0f);
            for (i = 0; i < 20; i++)
            {
                const float angle = (float)(2.0 * M_PI * double(i) / 19.0);
                sunVertex[i + 1] = glm::vec3(2.0f * worldSize,
                                             sunRadius * sinf(angle),
                                             sunRadius * cosf(angle));
            }
        }
        sunList.vertexData(sunVertex);
    }

    // make stars list
    {
        std::vector<glm::vec3> starsVertex;
        std::vector<glm::vec4> starsColors;
        for (i = 0; i < (int)NumStars; i++)
        {
            starsColors.push_back(glm::vec4(glm::make_vec3(stars[i]), 1.0f));
            starsVertex.push_back(glm::make_vec3(stars[i] + 3));
        }
        starList.vertexData(starsVertex);
        starList.colorData(starsColors);
    }

    //
    // ground
    //

    const GLfloat groundSize = 10.0f * worldSize;
    glm::vec3 groundPlane[4];
    for (i = 0; i < 4; i++)
        groundPlane[i] = glm::vec3(groundSize * squareShape[i], 0.0f);

    //
    // clouds
    //

    if (cloudsAvailable)
    {
        // make vertices for cloud polygons
        glm::vec3 cloudsOuter[4], cloudsInner[4];
        const GLfloat uvScale = 0.25f;
        for (i = 0; i < 4; i++)
        {
            cloudsOuter[i]   = groundPlane[i];
            cloudsOuter[i].z = 120.0f * BZDBCache::tankHeight;
            cloudsInner[i][0] = uvScale * cloudsOuter[i][0];
            cloudsInner[i][1] = uvScale * cloudsOuter[i][1];
            cloudsInner[i][2] = cloudsOuter[i][2];
        }

        {
            // inner clouds -- full opacity
            glm::vec2 opaqueTextur[10];
            glm::vec3 opaqueVertex[10];
            glm::vec4 opaqueColors[10];
            opaqueTextur[0] = uvScale * cloudRepeats * squareShape[3];
            opaqueVertex[0] = cloudsInner[3];
            opaqueTextur[1] = uvScale * cloudRepeats * squareShape[2];
            opaqueVertex[1] = cloudsInner[2];
            opaqueTextur[2] = uvScale * cloudRepeats * squareShape[0];
            opaqueVertex[2] = cloudsInner[0];
            opaqueTextur[3] = uvScale * cloudRepeats * squareShape[1];
            opaqueVertex[3] = cloudsInner[1];
            opaqueCloud.textureData(opaqueTextur);
            opaqueCloud.vertexData(opaqueVertex);

            const auto white = glm::vec4(1.0f);
            const auto tWhit = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);

            // outer clouds -- fade to zero opacity at outer edge
            opaqueColors[0] = tWhit;
            opaqueTextur[0] = cloudRepeats * squareShape[1];
            opaqueVertex[0] = cloudsOuter[1];
            opaqueColors[1] = white;
            opaqueTextur[1] = uvScale * cloudRepeats * squareShape[1];
            opaqueVertex[1] = cloudsInner[1];

            opaqueColors[2] = tWhit;
            opaqueTextur[2] = cloudRepeats * squareShape[2];
            opaqueVertex[2] = cloudsOuter[2];
            opaqueColors[3] = white;
            opaqueTextur[3] = uvScale * cloudRepeats * squareShape[2];
            opaqueVertex[3] = cloudsInner[2];

            opaqueColors[4] = tWhit;
            opaqueTextur[4] = cloudRepeats * squareShape[3];
            opaqueVertex[4] = cloudsOuter[3];
            opaqueColors[5] = white;
            opaqueTextur[5] = uvScale * cloudRepeats * squareShape[3];
            opaqueVertex[5] = cloudsInner[3];

            opaqueColors[6] = tWhit;
            opaqueTextur[6] = cloudRepeats * squareShape[0];
            opaqueVertex[6] = cloudsOuter[0];
            opaqueColors[7] = white;
            opaqueTextur[7] = uvScale * cloudRepeats * squareShape[0];
            opaqueVertex[7] = cloudsInner[0];

            opaqueColors[8] = tWhit;
            opaqueTextur[8] = cloudRepeats * squareShape[1];
            opaqueVertex[8] = cloudsOuter[1];
            opaqueColors[9] = white;
            opaqueTextur[9] = uvScale * cloudRepeats * squareShape[1];
            opaqueVertex[9] = cloudsInner[1];

            cloudsList.colorData(opaqueColors);
            cloudsList.textureData(opaqueTextur);
            cloudsList.vertexData(opaqueVertex);
        }
    }

    //
    // mountains
    //

    if (numMountainTextures > 0)
    {
        // prepare display lists.  need at least NumMountainFaces, but
        // we also need a multiple of the number of subtextures.  put
        // all the faces using a given texture into the same list.
        const int numFacesPerTexture = (NumMountainFaces +
                                        numMountainTextures - 1) / numMountainTextures;
        const float angleScale = (float)(M_PI / (numMountainTextures * numFacesPerTexture));
        int n = numFacesPerTexture / 2;
        float hightScale = mountainsMinWidth / 256.0f;

        for (j = 0; j < numMountainTextures; n += numFacesPerTexture, j++)
        {
            const int vectorSize = 4 * numFacesPerTexture + 6;
            mountainsList[j] = Vertex_Chunk(Vertex_Chunk::VTN, vectorSize);
            auto mountainsNormal  = new glm::vec3[vectorSize];
            auto mountainsTexture = new glm::vec2[vectorSize];
            auto mountainsVertex  = new glm::vec3[vectorSize];
            {
                auto currNormal = mountainsNormal;
                auto currTextur = mountainsTexture;
                auto currVertex = mountainsVertex;

                glm::vec3 normal;
                glm::vec2 textur;
                glm::vec3 vertex;
                for (i = 0; i <= numFacesPerTexture; i++)
                {
                    const float angle = angleScale * (float)(i + n);
                    float frac = (float)i / (float)numFacesPerTexture;
                    if (numMountainTextures != 1)
                        frac = (frac * (float)(mountainsMinWidth - 2) + 1.0f) /
                               (float)mountainsMinWidth;
                    const float cosa = cosf(angle);
                    const float sina = sinf(angle);
                    normal = -(float)M_SQRT1_2 * glm::vec3(cosa, sina, -1.0f);
                    textur = glm::vec2(frac, 0.02f);
                    vertex = 2.25f * worldSize * glm::vec3(cosa, sina, 0.0f);
                    *currNormal++ = normal;
                    *currTextur++ = textur;
                    *currVertex++ = vertex;
                    textur.t = 0.99f;
                    vertex.z = 0.45f * worldSize * hightScale;
                    *currNormal++ = normal;
                    *currTextur++ = textur;
                    *currVertex++ = vertex;
                }
                // degenerated triangles
                {
                    *currNormal++ = normal;
                    *currTextur++ = textur;
                    *currVertex++ = vertex;

                    const float angle = (float)(M_PI + angleScale * (float)n);
                    const float cosa = cosf(angle);
                    const float sina = sinf(angle);
                    vertex = 2.25f * worldSize * glm::vec3(cosa, sina, 0.0f);
                    *currNormal++ = normal;
                    *currTextur++ = textur;
                    *currVertex++ = vertex;
                }
                for (i = 0; i <= numFacesPerTexture; i++)
                {
                    const float angle = (float)(M_PI + angleScale * (double)(i + n));
                    float frac = (float)i / (float)numFacesPerTexture;
                    if (numMountainTextures != 1)
                        frac = (frac * (float)(mountainsMinWidth - 2) + 1.0f) /
                               (float)mountainsMinWidth;
                    const float cosa = cosf(angle);
                    const float sina = sinf(angle);
                    normal = -(float)M_SQRT1_2 * glm::vec3(cosa, sina, -1.0f);
                    textur = glm::vec2(frac, 0.02f);
                    vertex = 2.25f * worldSize * glm::vec3(cosa, sina, 0.0f);
                    *currNormal++ = normal;
                    *currTextur++ = textur;
                    *currVertex++ = vertex;
                    textur.t = 0.99f;
                    vertex.z = 0.45f * worldSize * hightScale;
                    *currNormal++ = normal;
                    *currTextur++ = textur;
                    *currVertex++ = vertex;
                }
            }
            mountainsList[j].normalData(mountainsNormal);
            mountainsList[j].textureData(mountainsTexture);
            mountainsList[j].vertexData(mountainsVertex);

            delete [] mountainsNormal;
            delete [] mountainsTexture;
            delete [] mountainsVertex;
        }
    }

    //
    // update objects in sky.  the appearance of these objects will
    // be wrong until setCelestial is called with the appropriate
    // arguments.
    //
    makeCelestialLists();
}


void BackgroundRenderer::freeContext(void* self)
{
    ((BackgroundRenderer*)self)->doFreeDisplayLists();
}


void BackgroundRenderer::initContext(void* self)
{
    ((BackgroundRenderer*)self)->doInitDisplayLists();
}


const glm::vec3 *BackgroundRenderer::getSunDirection() const
{
    if (areShadowsCast(sunDirection))
        return &sunDirection;
    else
        return NULL;
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
