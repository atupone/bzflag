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

#define GLM_ENABLE_EXPERIMENTAL

// system headers
#include <string.h>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/transform.hpp>

// common headers
#include "OpenGLMaterial.h"
#include "TextureManager.h"
#include "BZDBCache.h"
#include "BzMaterial.h"
#include "TextureMatrix.h"
#include "ParseColor.h"
#include "BZDBCache.h"
#include "OpenGLCommon.h"
#include "VBO_Drawing.h"

// local headers
#include "daylight.h"
#include "stars.h"
#include "MainWindow.h"
#include "SceneNode.h"
#include "effectsRenderer.h"

static const glm::vec2 squareShape[4] =
{
    {  1.0f,  1.0f }, { -1.0f,  1.0f },
    { -1.0f, -1.0f }, {  1.0f, -1.0f }
};


glm::vec3       BackgroundRenderer::skyPyramid[5];
const GLfloat       BackgroundRenderer::cloudRepeats = 3.0f;
static const int    NumMountainFaces = 16;

glm::vec4       BackgroundRenderer::groundColor[4];
glm::vec4       BackgroundRenderer::groundColorInv[4];

const glm::vec4     BackgroundRenderer::defaultGroundColor[4] =
{
    { 0.0f, 0.35f, 0.0f, 1.0f },
    { 0.0f, 0.20f, 0.0f, 1.0f },
    { 1.0f, 1.00f, 1.0f, 1.0f },
    { 1.0f, 1.00f, 1.0f, 1.0f }
};
const glm::vec4     BackgroundRenderer::defaultGroundColorInv[4] =
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
    sunInSkyVBO(Vertex_Chunk::VC, 6),
    sunSet1VBO(Vertex_Chunk::VC, 4),
    sunSet2VBO(Vertex_Chunk::VC, 12),
    gridSpacing(60.0f), // meters
    gridCount(4.0f),
    mountainsAvailable(false),
    numMountainTextures(0),
    mountainsGState(NULL),
    cloudDriftU(0.0f),
    cloudDriftV(0.0f),
    cloudsList(Vertex_Chunk::VTC, 16),
    sunXFormList(Vertex_Chunk::V, 21),
    starXFormList(Vertex_Chunk::VC, NumStars)
{
    static bool init = false;
    OpenGLGStateBuilder gstate;
    static const auto black = glm::vec3(0.0f, 0.0f, 0.0f);
    static const auto white = glm::vec3(1.0f);
    OpenGLMaterial defaultMaterial(black, black, 0.0f);
    OpenGLMaterial rainMaterial(white, white, 0.0f);

    sun[0] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 20; i++)
    {
        const float angle = (float)(2.0 * M_PI * double(i) / 19.0);
        auto vertex = glm::vec4(1.0f, sinf(angle), cosf(angle), 1.0f);
        sun[i + 1] = vertex;
    }

    starsColor.clear();
    starsVertex.clear();
    for (int i = 0; i < (int)NumStars; i++)
    {
        starsColor.push_back(glm::vec4(stars[i][0], stars[i][1], stars[i][2], 1.0f));
        starsVertex.push_back(glm::vec4(stars[i][3], stars[i][4], stars[i][5], 1.0f));
    }

    // initialize global to class stuff
    if (!init)
    {
        init = true;
        resizeSky();
    }

    // initialize the celestial vectors
    static const auto up = glm::vec3(0.0f, 0.0f, 1.0f);
    sunDirection = up;
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
            mountainsList.resize(numMountainTextures);
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
}


void BackgroundRenderer::bzdbCallback(const std::string& name, void* data)
{
    BackgroundRenderer* br = (BackgroundRenderer*) data;
    if (name == "_skyColor")
        br->setSkyColors();
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

    static const auto black = glm::vec3(0.0f, 0.0f, 0.0f);
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


void BackgroundRenderer::setCelestial(const SceneRenderer& renderer,
                                      const glm::vec3 &sunDir,
                                      const glm::vec3 &moonDir)
{
    // set sun and moon positions
    sunDirection = sunDir;
    moonDirection = moonDir;

    makeCelestialLists(renderer);

    return;
}


void BackgroundRenderer::setSkyColors()
{
    // change sky colors according to the sun position
    glm::vec3 colors[4];
    getSkyColor(sunDirection, colors);

    skyZenithColor = colors[0];
    skySunDirColor = colors[1];
    skyAntiSunDirColor = colors[2];
    skyCrossSunDirColor = colors[3];

    prepareSkyVBO();

    return;
}


void BackgroundRenderer::makeCelestialLists(const SceneRenderer& renderer)
{
    setSkyColors();

    // get a few other things concerning the sky
    doShadows = areShadowsCast(sunDirection);
    doStars = areStarsVisible(sunDirection);
    doSunset = getSunsetTop(sunDirection, sunsetTop);

    // make pretransformed display list for sun
    glm::vec3 sunVertices[21];

    // sun first.  sun is a disk that should be about a half a degree wide
    // with a normal (60 degree) perspective.
    const float worldSize = BZDBCache::worldSize;
    const float sunRadius = (float)(2.0 * worldSize * atanf((float)(60.0*M_PI/180.0)) / 60.0);
    const auto up    = glm::vec3(0.0f,  0.0f, 1.0f);
    const auto south = glm::vec3(0.0f, -1.0f, 0.0f);
    const auto east  = glm::vec3(1.0f,  0.0f, 0.0f);

    glm::mat4 mat = glm::mat4(1.0f);
    mat = glm::rotate(
              mat,
              (GLfloat)atan2f(sunDirection[1], sunDirection[0]),
              up);
    mat = glm::rotate(mat, (GLfloat)asinf(sunDirection[2]), south);
    mat = glm::scale(mat, glm::vec3(2.0f * worldSize, sunRadius, sunRadius));
    for (int i = 0; i < 21; i++)
    {
        auto vertex = mat * sun[i];
        sunVertices[i] = glm::vec3(vertex);
    }
    sunXFormList.vertexData(sunVertices);

    // compute display list for moon
    float coverage = glm::dot(moonDirection, sunDirection);
    // hack coverage to lean towards full
    coverage = (coverage < 0.0f) ? -sqrtf(-coverage) : coverage * coverage;
    const float moonRadius = 2.0f * worldSize *
                             atanf((float)((60.0 * M_PI / 180.0) / 60.0));
    // limbAngle is dependent on moon position but sun is so much farther
    // away that the moon's position is negligible.  rotate sun and moon
    // so that moon is on the horizon in the +x direction, then compute
    // the angle to the sun position in the yz plane.
    float sun2[3];
    const float moonAzimuth = atan2f(moonDirection[1], moonDirection[0]);
    const float moonAltitude = asinf(moonDirection[2]);
    sun2[0] = sunDirection[0] * cosf(moonAzimuth) + sunDirection[1] * sinf(moonAzimuth);
    sun2[1] = sunDirection[1] * cosf(moonAzimuth) - sunDirection[0] * sinf(moonAzimuth);
    sun2[2] = sunDirection[2] * cosf(moonAltitude) - sun2[0] * sinf(moonAltitude);
    const float limbAngle = atan2f(sun2[2], sun2[1]);

    const int moonSegements = BZDB.evalInt("moonSegments");
    mat = glm::mat4(1.0f);
    mat = glm::rotate(
              mat,
              (GLfloat)atan2f(moonDirection[1], moonDirection[0]),
              up);
    mat = glm::rotate(mat, (GLfloat)asinf(moonDirection[2]), south);
    mat = glm::rotate(mat, (GLfloat)limbAngle, east);
    mat = glm::scale(mat, glm::vec3(2.0f * worldSize, moonRadius, moonRadius));

    moonVertex.clear();
    moonVertex.push_back(mat * glm::vec4(1.0f, 0.0f, -1.0f, 1.0f));
    for (int i = 0; i < moonSegements - 1; i++)
    {
        float angle = (i - (moonSegements / 2) - 1) / (moonSegements / 2.0)
                      * M_PI / 2.0f;
        float sinAngle = sinf(angle);
        float cosAngle = cosf(angle);
        auto vertex = glm::vec4(1.0f, coverage * cosAngle, sinAngle, 1.0f);
        moonVertex.push_back(mat * vertex);
        moonVertex.push_back(mat * glm::vec4(1.0f, cosAngle, sinAngle, 1.0f));
    }
    moonVertex.push_back(mat * glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
    moonList = Vertex_Chunk(Vertex_Chunk::V, moonVertex.size());
    moonList.vertexData(moonVertex);

    mat = glm::make_mat4(renderer.getCelestialTransform());
    mat = glm::scale(mat, glm::vec3(worldSize));

    // make pretransformed display list for stars
    // make stars list
    transformedStarsVertex.clear();
    for (int i = 0; i < (int)NumStars; i++)
    {
        auto vertex = glm::vec3(mat * starsVertex[i]);
        transformedStarsVertex.push_back(vertex);
    }
    starXFormList.vertexData(transformedStarsVertex);
    starXFormList.colorData(starsColor);

    prepareSkyVBO();

    return;
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


void BackgroundRenderer::renderSky(SceneRenderer& renderer, bool fullWindow,
                                   bool mirror)
{
    if (!BZDBCache::drawSky)
        return;
    if (renderer.useQuality() > 0)
        drawSky(renderer, mirror);
    else
    {
        // low detail -- draw as damn fast as ya can, ie cheat.  use glClear()
        // to draw solid color sky and ground.
        MainWindow& window = renderer.getWindow();
        const int x = window.getOriginX();
        const int y = window.getOriginY();
        const int width = window.getWidth();
        const int height = window.getHeight();
        const int viewHeight = window.getViewHeight();
        const SceneRenderer::ViewType viewType = renderer.getViewType();

        // draw sky
        glPushAttrib(GL_SCISSOR_BIT);
        glScissor(x, y + height - (viewHeight >> 1), width, (viewHeight >> 1));
        glClearColor(skyZenithColor[0], skyZenithColor[1], skyZenithColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw ground -- first get the color (assume it's all green)
        GLfloat _groundColor = 0.1f + 0.15f * renderer.getSunColor()[1];
        if (fullWindow && viewType == SceneRenderer::ThreeChannel)
            glScissor(x, y, width, height >> 1);
        else if (fullWindow && viewType == SceneRenderer::Stacked)
            glScissor(x, y, width, height >> 1);
#ifndef USE_GL_STEREO
        else if (fullWindow && viewType == SceneRenderer::Stereo)
            glScissor(x, y, width, height >> 1);
#endif
        else
            glScissor(x, y + height - viewHeight, width, (viewHeight + 1) >> 1);
        if (invert)
            glClearColor(_groundColor, 0.0f, _groundColor, 0.0f);
        else
            glClearColor(0.0f, _groundColor, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // back to normal
        glPopAttrib();
    }
}


void BackgroundRenderer::renderGround(SceneRenderer& renderer,
                                      bool fullWindow)
{
    if (renderer.useQuality() > 0)
        drawGround();
    else
    {
        // low detail -- draw as damn fast as ya can, ie cheat.  use glClear()
        // to draw solid color sky and ground.
        MainWindow& window = renderer.getWindow();
        const int x = window.getOriginX();
        const int y = window.getOriginY();
        const int width = window.getWidth();
        const int height = window.getHeight();
        const int viewHeight = window.getViewHeight();
        const SceneRenderer::ViewType viewType = renderer.getViewType();

        // draw sky
        glPushAttrib(GL_SCISSOR_BIT);
        glScissor(x, y + height - (viewHeight >> 1), width, (viewHeight >> 1));
        glClearColor(skyZenithColor[0], skyZenithColor[1], skyZenithColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw ground -- first get the color (assume it's all green)
        GLfloat _groundColor = 0.1f + 0.15f * renderer.getSunColor()[1];
        if (fullWindow && viewType == SceneRenderer::ThreeChannel)
            glScissor(x, y, width, height >> 1);
        else if (fullWindow && viewType == SceneRenderer::Stacked)
            glScissor(x, y, width, height >> 1);
#ifndef USE_GL_STEREO
        else if (fullWindow && viewType == SceneRenderer::Stereo)
            glScissor(x, y, width, height >> 1);
#endif
        else
            glScissor(x, y + height - viewHeight, width, (viewHeight + 1) >> 1);
        if (invert)
            glClearColor(_groundColor, 0.0f, _groundColor, 0.0f);
        else
            glClearColor(0.0f, _groundColor, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // back to normal
        glPopAttrib();
    }
}


void BackgroundRenderer::renderGroundEffects(SceneRenderer& renderer,
        bool drawingMirror)
{
    // zbuffer should be disabled.  either everything is coplanar with
    // the ground or is drawn back to front and is occluded by everything
    // drawn after it.  also use projection with very far clipping plane.

    // only draw the grid lines if texturing is disabled
    if (!BZDBCache::texture || (renderer.useQuality() <= 0))
        drawGroundGrid(renderer);

    if (!blank)
    {
        if (doShadows && shadowsVisible && !drawingMirror)
            drawGroundShadows(renderer);

        // draw light receivers on ground (little meshes under light sources so
        // the ground gets illuminated).  this is necessary because lighting is
        // performed only at a vertex, and the ground's vertices are a few
        // kilometers away.
        if (BZDBCache::lighting &&
                !drawingMirror && BZDBCache::drawGroundLights)
        {
            if (BZDBCache::tesselation && (renderer.useQuality() >= 3))
            {
//    (BZDB.get(StateDatabase::BZDB_FOGMODE) == "none")) {
                // not really tesselation, but it is tied to the "Best" lighting,
                // avoid on foggy maps, because the blending function accumulates
                // too much brightness.
                drawAdvancedGroundReceivers(renderer);
            }
            else
                drawGroundReceivers(renderer);
        }

        if (renderer.useQuality() > 1)
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
        skyPyramid[i] = skySize * glm::vec3(squareShape[i], 0.0f);
    skyPyramid[4] = glm::vec3(0.0f, 0.0f, skySize);
    prepareSkyVBO();
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
        for (int c = 0; c < 4; c++)
        {
            skyboxColor[i][c] = 0.0f;
            for (int f = 0; f < 3; f++)
                skyboxColor[i][c] += bzmats[cornerFaces[i][f]]->getDiffuse()[c];
            skyboxColor[i][c] /= 3.0f;
        }
    }

    haveSkybox = true;

    prepareSkyBoxVBO();
    return;
}

void BackgroundRenderer::prepareSkyBoxVBO()
{
    const glm::vec3 verts[8] =
    {
        {-1.0f, -1.0f, -1.0f}, {+1.0f, -1.0f, -1.0f},
        {+1.0f, +1.0f, -1.0f}, {-1.0f, +1.0f, -1.0f},
        {-1.0f, -1.0f, +1.0f}, {+1.0f, -1.0f, +1.0f},
        {+1.0f, +1.0f, +1.0f}, {-1.0f, +1.0f, +1.0f}
    };
    const glm::vec2 txcds[4] =
    {
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    const glm::vec4 *color = skyboxColor;

    glm::vec4 colors[4];
    glm::vec3 vertices[4];

    colors[0]   = glm::vec4(glm::vec3(color[2]), 1);
    colors[1]   = glm::vec4(glm::vec3(color[3]), 1);
    colors[2]   = glm::vec4(glm::vec3(color[1]), 1);
    colors[3]   = glm::vec4(glm::vec3(color[0]), 1);
    vertices[0] = verts[2];
    vertices[1] = verts[3];
    vertices[2] = verts[1];
    vertices[3] = verts[0];

    bottomVBO.colorData(colors);
    bottomVBO.vertexData(vertices);
    bottomVBO.textureData(txcds);

    colors[0]   = glm::vec4(glm::vec3(color[5]), 1);
    colors[1]   = glm::vec4(glm::vec3(color[4]), 1);
    colors[2]   = glm::vec4(glm::vec3(color[6]), 1);
    colors[3]   = glm::vec4(glm::vec3(color[7]), 1);
    vertices[0] = verts[5];
    vertices[1] = verts[4];
    vertices[2] = verts[6];
    vertices[3] = verts[7];

    topVBO.colorData(colors);
    topVBO.vertexData(vertices);
    topVBO.textureData(txcds);

    colors[0]   = glm::vec4(glm::vec3(color[0]), 1);
    colors[1]   = glm::vec4(glm::vec3(color[3]), 1);
    colors[2]   = glm::vec4(glm::vec3(color[4]), 1);
    colors[3]   = glm::vec4(glm::vec3(color[7]), 1);
    vertices[0] = verts[0];
    vertices[1] = verts[3];
    vertices[2] = verts[4];
    vertices[3] = verts[7];

    leftVBO.colorData(colors);
    leftVBO.vertexData(vertices);
    leftVBO.textureData(txcds);

    colors[0]   = glm::vec4(glm::vec3(color[1]), 1);
    colors[1]   = glm::vec4(glm::vec3(color[0]), 1);
    colors[2]   = glm::vec4(glm::vec3(color[5]), 1);
    colors[3]   = glm::vec4(glm::vec3(color[4]), 1);
    vertices[0] = verts[1];
    vertices[1] = verts[0];
    vertices[2] = verts[5];
    vertices[3] = verts[4];

    frontVBO.colorData(colors);
    frontVBO.vertexData(vertices);
    frontVBO.textureData(txcds);

    colors[0]   = glm::vec4(glm::vec3(color[2]), 1);
    colors[1]   = glm::vec4(glm::vec3(color[1]), 1);
    colors[2]   = glm::vec4(glm::vec3(color[6]), 1);
    colors[3]   = glm::vec4(glm::vec3(color[5]), 1);
    vertices[0] = verts[2];
    vertices[1] = verts[1];
    vertices[2] = verts[6];
    vertices[3] = verts[5];

    rightVBO.colorData(colors);
    rightVBO.vertexData(vertices);
    rightVBO.textureData(txcds);

    colors[0]   = glm::vec4(glm::vec3(color[3]), 1);
    colors[1]   = glm::vec4(glm::vec3(color[2]), 1);
    colors[2]   = glm::vec4(glm::vec3(color[7]), 1);
    colors[3]   = glm::vec4(glm::vec3(color[6]), 1);
    vertices[0] = verts[3];
    vertices[1] = verts[2];
    vertices[2] = verts[7];
    vertices[3] = verts[6];

    backVBO.colorData(colors);
    backVBO.vertexData(vertices);
    backVBO.textureData(txcds);
}


static void glVertex2fv(const glm::vec2 &p)
{
    ::glVertex2f(p.x, p.y);
}


static void glColor3fv(const glm::vec3 &c)
{
    ::glColor3f(c.r, c.g, c.b);
}


static void glColor4fv(const glm::vec4 &c)
{
    ::glColor4f(c.r, c.g, c.b, c.a);
}


static void glTexCoord2fv(const glm::vec2 &t)
{
    ::glTexCoord2f(t.s, t.t);
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


void BackgroundRenderer::prepareSkyVBO()
{
    glm::vec4 colors[12];
    glm::vec3 vertex[12];

    colors[0] = glm::make_vec4(skyZenithColor);
    vertex[0] = skyPyramid[4];
    colors[1] = glm::make_vec4(skyCrossSunDirColor);
    vertex[1] = skyPyramid[0];
    colors[2] = glm::make_vec4(skySunDirColor);
    vertex[2] = skyPyramid[3];
    colors[3] = glm::make_vec4(skyCrossSunDirColor);
    vertex[3] = skyPyramid[2];
    colors[4] = glm::make_vec4(skyAntiSunDirColor);
    vertex[4] = skyPyramid[1];
    colors[5] = glm::make_vec4(skyCrossSunDirColor);
    vertex[5] = skyPyramid[0];
    sunInSkyVBO.colorData(colors);
    sunInSkyVBO.vertexData(vertex);

    colors[0] = glm::make_vec4(skyZenithColor);
    vertex[0] = skyPyramid[4];
    colors[1] = glm::make_vec4(skyCrossSunDirColor);
    vertex[1] = skyPyramid[2];
    colors[2] = glm::make_vec4(skyAntiSunDirColor);
    vertex[2] = skyPyramid[1];
    colors[3] = glm::make_vec4(skyCrossSunDirColor);
    vertex[3] = skyPyramid[0];
    sunSet1VBO.colorData(colors);
    sunSet1VBO.vertexData(vertex);

    glm::vec3 sunsetTopPoint;
    sunsetTopPoint[0] = skyPyramid[3][0] * (1.0f - sunsetTop);
    sunsetTopPoint[1] = skyPyramid[3][1] * (1.0f - sunsetTop);
    sunsetTopPoint[2] = skyPyramid[4][2] * sunsetTop;

    colors[0] = glm::make_vec4(skyZenithColor);
    vertex[0] = skyPyramid[4];
    colors[1] = glm::make_vec4(skyCrossSunDirColor);
    vertex[1] = skyPyramid[0];
    colors[2] = glm::make_vec4(skyZenithColor);
    vertex[2] = sunsetTopPoint;
    colors[3] = colors[2];
    vertex[3] = skyPyramid[4];
    colors[4] = colors[3];
    vertex[4] = sunsetTopPoint;
    colors[5] = glm::make_vec4(skyCrossSunDirColor);
    vertex[5] = skyPyramid[2];
    colors[6] = glm::make_vec4(skyZenithColor);
    vertex[6] = sunsetTopPoint;
    colors[7] = glm::make_vec4(skyCrossSunDirColor);
    vertex[7] = skyPyramid[0];
    colors[8] = glm::make_vec4(skySunDirColor);
    vertex[8] = skyPyramid[3];
    colors[9] = glm::make_vec4(skyCrossSunDirColor);
    vertex[9] = skyPyramid[2];
    colors[10] = glm::make_vec4(skyZenithColor);
    vertex[10] = sunsetTopPoint;
    colors[11] = glm::make_vec4(skySunDirColor);
    vertex[11] = skyPyramid[3];
    sunSet2VBO.colorData(colors);
    sunSet2VBO.vertexData(vertex);
}


void BackgroundRenderer::drawSky(SceneRenderer& renderer, bool mirror)
{
    glPushMatrix();

    const bool doSkybox = haveSkybox && (renderer.useQuality() >= 2);

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
            sunInSkyVBO.draw(GL_TRIANGLE_FAN);
        }
        else
        {
            // overall shape is a pyramid, but the solar sides are two
            // triangles each.  the top triangle is all zenith color,
            // the bottom goes from zenith to sun-dir color.
            sunSet1VBO.draw(GL_TRIANGLE_FAN);
            sunSet2VBO.draw(GL_TRIANGLES);
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
            const GLfloat *sunColor = renderer.getSunScaledColor();
            glColor4f(sunColor[0], sunColor[1], sunColor[2], 1.0f);
            sunXFormList.draw(GL_TRIANGLE_FAN);
        }

        if (doStars)
        {
            starGState[starGStateIndex].setState();
            starXFormList.draw(GL_POINTS);
        }

        if (moonDirection[2] > -0.009f)
        {
            moonGState[doStars ? 1 : 0].setState();
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            //   if (useMoonTexture)
            //     glEnable(GL_TEXTURE_2D);
            moonList.draw(GL_TRIANGLE_STRIP);
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
        glm::vec4 c;
        if (invert)
            c = groundColorInv[styleIndex];
        else
        {
            float color[4];
            if (BZDB.isSet("GroundOverideColor") &&
                    parseColorString(BZDB.get("GroundOverideColor"), color))
                c = glm::make_vec4(color);
            else
                c = groundColor[styleIndex];
        }
        glColor4f(c.r, c.g, c.b, c.a);
        if (invert)
            invGroundGState[styleIndex].setState();
        else
            groundGState[styleIndex].setState();

        if (RENDERER.useQuality() >= 2)
            drawGroundCentered();
        else if (BZDBCache::texture)
            simpleGroundList[1].draw(GL_TRIANGLE_STRIP);
        else
            simpleGroundList[0].draw(GL_TRIANGLE_STRIP);
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
        for (int q = 0; q < 5; q++)
        {
            glBegin(GL_TRIANGLE_STRIP);
            for (int c = 0; c < 4; c++)
            {
                const int index = indices[q][c];
                glTexCoord2fv(vertices[index] * repeat);
                glVertex2fv(vertices[index]);
            }
            glEnd();
        }
    }

    return;
}


void BackgroundRenderer::drawGroundGrid(
    SceneRenderer& renderer)
{
    const auto pos = renderer.getViewFrustum().getEye();
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
    static const auto black = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    OpenGLCommon::getFogColor(fogColor);
    OpenGLCommon::setFogColor(black);
}


void BackgroundRenderer::drawGroundReceivers(SceneRenderer& renderer)
{
    static const int receiverRings = 4;
    static const int receiverSlices = 8;
    static const float receiverRingSize = 1.2f;   // meters
    static glm::vec2 angle[receiverSlices + 1];

    static bool init = false;
    if (!init)
    {
        init = true;
        const float receiverSliceAngle = (float)(2.0 * M_PI / double(receiverSlices));
        for (int i = 0; i <= receiverSlices; i++)
        {
            angle[i][0] = cosf((float)i * receiverSliceAngle);
            angle[i][1] = sinf((float)i * receiverSliceAngle);
        }
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

        const auto pos = light.getPosition();
        const auto lightColor = light.getColor();
        const GLfloat* atten = light.getAttenuation();

        // point under light
        float d = pos[2];
        float I = B / (atten[0] + d * (atten[1] + d * atten[2]));

        // maximum value
        const float maxVal = glm::compMax(lightColor);

        // if I is too attenuated, don't bother drawing anything
        if ((I * maxVal) < 0.02f)
            continue;

        // move to the light's position
        glTranslatef(pos[0], pos[1], 0.0f);

        // set the main lighting color
        glm::vec4 color;
        color[0] = lightColor[0];
        color[1] = lightColor[1];
        color[2] = lightColor[2];

        GLfloat outerSize = 0.0f;
        float outerAlpha  = I;

        glm::vec2 vertexList[receiverSlices + 1];

        for (j = 0; j <= receiverSlices; j++)
            vertexList[j] = glm::vec2(0.0f);

        glBegin(GL_TRIANGLE_STRIP);
        for (i = 1; i <= receiverRings; i++)
        {
            outerSize = receiverRingSize * GLfloat(i * i);

            // compute inner and outer lit colors
            float innerAlpha = outerAlpha;

            if (i == receiverRings)
                I = 0.0f;
            else
            {
                d = hypotf(outerSize, pos[2]);
                I = B / (atten[0] + d * (atten[1] + d * atten[2]));
                I *= pos[2] / d;
            }
            outerAlpha = I;

            for (j = 0; j < receiverSlices; j++)
            {
                color[3] = innerAlpha;
                glColor4fv(color);
                glVertex2fv(vertexList[j]);
                vertexList[j] = angle[j] * outerSize;
                color[3] = outerAlpha;
                glColor4fv(color);
                glVertex2fv(vertexList[j]);
            }
            color[3] = innerAlpha;
            glColor4fv(color);
            glVertex2fv(vertexList[receiverSlices]);
            vertexList[receiverSlices] = vertexList[0];
            color[3] = outerAlpha;
            glColor4fv(color);
            glVertex2fv(vertexList[0]);
        }
        glEnd();
        triangleCount += (receiverSlices + 1) * receiverRings * 2 - 2;

        glTranslatef(-pos[0], -pos[1], 0.0f);
    }
    glPopMatrix();

    OpenGLCommon::setFogColor(fogColor);
}


void BackgroundRenderer::drawAdvancedGroundReceivers(SceneRenderer& renderer)
{
    const float minLuminance = 0.02f;
    static const int receiverSlices = 32;
    static const float receiverRingSize = 0.5f;   // meters
    static glm::vec2 angle[receiverSlices + 1];

    static bool init = false;
    if (!init)
    {
        init = true;
        const float receiverSliceAngle = (float)(2.0 * M_PI / double(receiverSlices));
        for (int i = 0; i <= receiverSlices; i++)
        {
            angle[i][0] = cosf((float)i * receiverSliceAngle);
            angle[i][1] = sinf((float)i * receiverSliceAngle);
        }
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
        const auto sPlane = glm::vec4(repeat, 0.0f, 0.0f, 0.0f);
        const auto tPlane = glm::vec4(0.0f, repeat, 0.0f, 0.0f);
        OpenGLCommon::setEyePlanes(sPlane, tPlane);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
    }

    const auto black = glm::vec3(0.0f);
    glPushMatrix();
    int i, j;
    for (int k = 0; k < count; k++)
    {
        const OpenGLLight& light = renderer.getLight(k);
        if (light.getOnlyReal())
            continue;

        // get the light parameters
        const auto pos = light.getPosition();
        const auto lightColor = light.getColor();
        const GLfloat* atten = light.getAttenuation();

        // point under light
        float d = pos[2];
        float I = 1.0f / (atten[0] + d * (atten[1] + d * atten[2]));

        // set the main lighting color
        auto baseColor = glm::vec3(gndColor) * glm::vec3(lightColor);
        if (invert)   // beats me, should just color logic op the static nodes
            baseColor = 1.0f - baseColor;

        // maximum value
        const float maxVal = glm::compMax(baseColor);

        // if I is too attenuated, don't bother drawing anything
        if ((I * maxVal) < minLuminance)
            continue;

        // move to the light's position
        glTranslatef(pos[0], pos[1], 0.0f);

        float outerSize = 0.0f;
        auto outerColor = I * baseColor;

        glm::vec2 vertexList[receiverSlices + 1];

        for (j = 0; j <= receiverSlices; j++)
            vertexList[j] = glm::vec2(0.0f);

        glBegin(GL_TRIANGLE_STRIP);

        bool moreRings = true;
        for (i = 1; moreRings; i++)
        {
            // inner ring
            const auto innerColor = outerColor;

            // outer ring
            outerSize = receiverRingSize * GLfloat(i * i);
            d = hypotf(outerSize, pos[2]);
            I = 1.0f / (atten[0] + d * (atten[1] + d * atten[2]));
            I *= pos[2] / d; // diffuse angle factor
            moreRings = (I * maxVal) >= minLuminance;
            outerColor = moreRings ? I * baseColor : black;

            for (j = 0; j < receiverSlices; j++)
            {
                glColor3fv(innerColor);
                glVertex2fv(vertexList[j]);
                vertexList[j] = angle[j] * outerSize;
                glColor3fv(outerColor);
                glVertex2fv(vertexList[j]);
            }
            glColor3fv(innerColor);
            glVertex2fv(vertexList[receiverSlices]);
            vertexList[receiverSlices] = vertexList[0];
            glColor3fv(outerColor);
            glVertex2fv(vertexList[0]);
        }

        glEnd();
        triangleCount += (receiverSlices + 1) * i * 2 - 2;

        glTranslatef(-pos[0], -pos[1], 0.0f);
    }
    glPopMatrix();

    if (useTexture)
    {
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }

    OpenGLCommon::setFogColor(fogColor);
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
    SceneRenderer& renderer = RENDERER;

    // don't forget the tag-along
    EFFECTS.rebuildContext();

    //
    // sky stuff
    //

    const float worldSize = BZDBCache::worldSize;

    //
    // ground
    //

    const GLfloat groundSize = 10.0f * worldSize;
    glm::vec3 groundPlane[4];
    for (i = 0; i < 4; i++)
        groundPlane[i] = groundSize * glm::vec3(squareShape[i], 0.0f);

    {
        GLfloat xmin, xmax;
        GLfloat ymin, ymax;
        GLfloat xdist, ydist;
        GLfloat xtexmin, xtexmax;
        GLfloat ytexmin, ytexmax;
        GLfloat xtexdist, ytexdist;
        glm::vec2 vec;

#define GROUND_DIVS (4) //FIXME -- seems to be enough

        xmax = groundPlane[0][0];
        ymax = groundPlane[0][1];
        xmin = groundPlane[2][0];
        ymin = groundPlane[2][1];
        xdist = (xmax - xmin) / (float)GROUND_DIVS;
        ydist = (ymax - ymin) / (float)GROUND_DIVS;

        renderer.getGroundUV (groundPlane[0], vec);
        xtexmax = vec[0];
        ytexmax = vec[1];
        renderer.getGroundUV (groundPlane[2], vec);
        xtexmin = vec[0];
        ytexmin = vec[1];
        xtexdist = (xtexmax - xtexmin) / (float)GROUND_DIVS;
        ytexdist = (ytexmax - ytexmin) / (float)GROUND_DIVS;

        std::vector<glm::vec2> groundListTexture;
        std::vector<glm::vec3> groundListVertex;

        glm::vec2 texture;
        glm::vec3 vertex;

        for (i = 0; i < GROUND_DIVS; i++)
        {
            GLfloat yoff, ytexoff;

            yoff = ymin + ydist * (GLfloat)i;
            ytexoff = ytexmin + ytexdist * (GLfloat)i;

            texture = glm::vec2(xtexmin, ytexoff + ytexdist);
            vertex  = glm::vec3(xmin, yoff + ydist, 0.0f);

            if (i)
            {
                // Add degenerate triangle
                groundListTexture.push_back(texture);
                groundListVertex.push_back(vertex);
            }

            groundListTexture.push_back(texture);
            groundListVertex.push_back(vertex);

            texture = glm::vec2(xtexmin, ytexoff);
            vertex  = glm::vec3(xmin, yoff, 0.0f);

            groundListTexture.push_back(texture);
            groundListVertex.push_back(vertex);

            for (j = 0; j < GROUND_DIVS; j++)
            {
                GLfloat xoff, xtexoff;

                xoff = xmin + xdist * (GLfloat)(j + 1);
                xtexoff = xtexmin + xtexdist * (GLfloat)(j + 1);

                texture = glm::vec2(xtexoff, ytexoff + ytexdist);
                vertex  = glm::vec3(xoff, yoff + ydist, 0.0f);

                groundListTexture.push_back(texture);
                groundListVertex.push_back(vertex);

                texture = glm::vec2(xtexoff, ytexoff);
                vertex  = glm::vec3(xoff, yoff, 0.0f);

                groundListTexture.push_back(texture);
                groundListVertex.push_back(vertex);
            }
            if (i + 1 < GROUND_DIVS)
            {
                // Add degenerate triangle
                groundListTexture.push_back(texture);
                groundListVertex.push_back(vertex);
            }
        }
        simpleGroundList[1] = Vertex_Chunk(Vertex_Chunk::VT, groundListVertex.size());
        simpleGroundList[1].textureData(groundListTexture);
        simpleGroundList[1].vertexData(groundListVertex);
    }

    glm::vec3 groundVertex[4];
    groundVertex[0] = groundPlane[0];
    groundVertex[1] = groundPlane[1];
    groundVertex[2] = groundPlane[3];
    groundVertex[3] = groundPlane[2];
    simpleGroundList[0] = Vertex_Chunk(Vertex_Chunk::V, 4);
    simpleGroundList[0].vertexData(groundVertex);

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
            cloudsOuter[i][0] = groundPlane[i][0];
            cloudsOuter[i][1] = groundPlane[i][1];
            cloudsOuter[i][2] = groundPlane[i][2] + 120.0f * BZDBCache::tankHeight;
            cloudsInner[i][0] = uvScale * cloudsOuter[i][0];
            cloudsInner[i][1] = uvScale * cloudsOuter[i][1];
            cloudsInner[i][2] = cloudsOuter[i][2];
        }

        cloudColor.clear();
        cloudTexture.clear();
        cloudVertex.clear();

        const auto white = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        const auto tWhit = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);

        // inner clouds -- full opacity
        cloudColor.push_back(white);
        cloudTexture.push_back(uvScale * cloudRepeats * squareShape[3]);
        cloudVertex.push_back(cloudsInner[3]);
        cloudColor.push_back(white);
        cloudTexture.push_back(uvScale * cloudRepeats * squareShape[2]);
        cloudVertex.push_back(cloudsInner[2]);
        cloudColor.push_back(white);
        cloudTexture.push_back(uvScale * cloudRepeats * squareShape[0]);
        cloudVertex.push_back(cloudsInner[0]);
        cloudColor.push_back(white);
        cloudTexture.push_back(uvScale * cloudRepeats * squareShape[1]);
        cloudVertex.push_back(cloudsInner[1]);

        // Insert degenerated triangles
        cloudColor.push_back(white);
        cloudTexture.push_back(uvScale * cloudRepeats * squareShape[1]);
        cloudVertex.push_back(cloudsInner[1]);
        cloudColor.push_back(tWhit);
        cloudTexture.push_back(cloudRepeats * squareShape[1]);
        cloudVertex.push_back(cloudsOuter[1]);

        // outer clouds -- fade to zero opacity at outer edge
        cloudColor.push_back(tWhit);
        cloudTexture.push_back(cloudRepeats * squareShape[1]);
        cloudVertex.push_back(cloudsOuter[1]);
        cloudColor.push_back(white);
        cloudTexture.push_back(uvScale * cloudRepeats * squareShape[1]);
        cloudVertex.push_back(cloudsInner[1]);
        cloudColor.push_back(tWhit);
        cloudTexture.push_back(cloudRepeats * squareShape[2]);
        cloudVertex.push_back(cloudsOuter[2]);
        cloudColor.push_back(white);
        cloudTexture.push_back(uvScale * cloudRepeats * squareShape[2]);
        cloudVertex.push_back(cloudsInner[2]);
        cloudColor.push_back(tWhit);
        cloudTexture.push_back(cloudRepeats * squareShape[3]);
        cloudVertex.push_back(cloudsOuter[3]);
        cloudColor.push_back(white);
        cloudTexture.push_back(uvScale * cloudRepeats * squareShape[3]);
        cloudVertex.push_back(cloudsInner[3]);
        cloudColor.push_back(tWhit);
        cloudTexture.push_back(cloudRepeats * squareShape[0]);
        cloudVertex.push_back(cloudsOuter[0]);
        cloudColor.push_back(white);
        cloudTexture.push_back(uvScale * cloudRepeats * squareShape[0]);
        cloudVertex.push_back(cloudsInner[0]);
        cloudColor.push_back(tWhit);
        cloudTexture.push_back(cloudRepeats * squareShape[1]);
        cloudVertex.push_back(cloudsOuter[1]);
        cloudColor.push_back(white);
        cloudTexture.push_back(uvScale * cloudRepeats * squareShape[1]);
        cloudVertex.push_back(cloudsInner[1]);

        cloudsList.vertexData(cloudVertex);
        cloudsList.textureData(cloudTexture);
        cloudsList.colorData(cloudColor);
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

        glm::vec3 normal;
        glm::vec2 texture;
        glm::vec3 vertex;
        float     angle;

        for (j = 0; j < numMountainTextures; n += numFacesPerTexture, j++)
        {
            mountainsVertex.clear();
            mountainsNormal.clear();
            mountainsTexture.clear();

            for (i = 0; i <= numFacesPerTexture; i++)
            {
                angle = angleScale * (float)(i + n);
                float frac = (float)i / (float)numFacesPerTexture;
                if (numMountainTextures != 1)
                    frac = (frac * (float)(mountainsMinWidth - 2) + 1.0f) /
                           (float)mountainsMinWidth;
                normal = glm::vec3(
                             (float)(-M_SQRT1_2 * cosf(angle)),
                             (float)(-M_SQRT1_2 * sinf(angle)),
                             (float)M_SQRT1_2);
                texture = glm::vec2(frac, 0.02f);
                vertex = glm::vec3(
                             2.25f * worldSize * cosf(angle),
                             2.25f * worldSize * sinf(angle),
                             0.0f);

                mountainsNormal.push_back(normal);
                mountainsTexture.push_back(texture);
                mountainsVertex.push_back(vertex);

                texture.t = 0.99f;
                vertex.z  = 0.45f * worldSize * hightScale;
                mountainsNormal.push_back(normal);
                mountainsTexture.push_back(texture);
                mountainsVertex.push_back(vertex);
            }

            // degenerated triangles
            mountainsNormal.push_back(normal);
            mountainsTexture.push_back(texture);
            mountainsVertex.push_back(vertex);

            angle = (float)(M_PI + angleScale * (double)n);
            vertex = glm::vec3(
                         2.25f * worldSize * cosf(angle),
                         2.25f * worldSize * sinf(angle),
                         0.0f);

            mountainsNormal.push_back(normal);
            mountainsTexture.push_back(texture);
            mountainsVertex.push_back(vertex);

            for (i = 0; i <= numFacesPerTexture; i++)
            {
                angle = (float)(M_PI + angleScale * (double)(i + n));
                float frac = (float)i / (float)numFacesPerTexture;
                if (numMountainTextures != 1)
                    frac = (frac * (float)(mountainsMinWidth - 2) + 1.0f) /
                           (float)mountainsMinWidth;
                normal = glm::vec3(
                             (float)(-M_SQRT1_2 * cosf(angle)),
                             (float)(-M_SQRT1_2 * sinf(angle)),
                             (float)M_SQRT1_2);
                texture = glm::vec2(frac, 0.02f);
                vertex = glm::vec3(
                             2.25f * worldSize * cosf(angle),
                             2.25f * worldSize * sinf(angle),
                             0.0f);

                mountainsNormal.push_back(normal);
                mountainsTexture.push_back(texture);
                mountainsVertex.push_back(vertex);

                texture.t = 0.99f;
                vertex.z  = 0.45f * worldSize * hightScale;
                mountainsNormal.push_back(normal);
                mountainsTexture.push_back(texture);
                mountainsVertex.push_back(vertex);
            }

            mountainsList[j] = Vertex_Chunk(Vertex_Chunk::VTN, mountainsVertex.size());
            mountainsList[j].normalData(mountainsNormal);
            mountainsList[j].textureData(mountainsTexture);
            mountainsList[j].vertexData(mountainsVertex);
        }
    }

    //
    // update objects in sky.  the appearance of these objects will
    // be wrong until setCelestial is called with the appropriate
    // arguments.
    //
    makeCelestialLists(renderer);
}


void BackgroundRenderer::freeContext(void* self)
{
    ((BackgroundRenderer*)self)->doFreeDisplayLists();
}


void BackgroundRenderer::initContext(void* self)
{
    ((BackgroundRenderer*)self)->doInitDisplayLists();
}


const glm::vec3 &BackgroundRenderer::getSunDirection() const
{
    return sunDirection;
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
