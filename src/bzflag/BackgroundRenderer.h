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

/*
 * BackgroundRenderer:
 *  Encapsulates rendering background stuff
 *
 * FIXME -- should be abstract base for background rendering
 */

#ifndef BZF_BACKGROUND_RENDERER_H
#define BZF_BACKGROUND_RENDERER_H

#include "common.h"

/* system headers */
#include <string>

/* common interface headers */
#include "bzfgl.h"
#include "OpenGLGState.h"
#include "WeatherRenderer.h"
#include "Vertex_Chunk.h"

class SceneRenderer;
class BackgroundRenderer
{
public:
    BackgroundRenderer();
    ~BackgroundRenderer();

    BackgroundRenderer(const BackgroundRenderer&) = delete;
    BackgroundRenderer& operator=(const BackgroundRenderer&) = delete;

    void        setupGroundMaterials();
    void        setupSkybox();

    void        renderSky(SceneRenderer&, bool fullWindow, bool mirror);
    void        renderGround(SceneRenderer&, bool fullWindow);
    void        renderGroundEffects(SceneRenderer&, bool drawingMirror);
    void        renderEnvironment(SceneRenderer&, bool update);

    void        resize();

    const glm::vec3 &getSunDirection() const;
    void        setBlank(bool blank = true);
    void        setInvert(bool invert = true);
    void        setCelestial(const SceneRenderer&,
                             const glm::vec3 &sunDirection,
                             const glm::vec3 &moonDirection);
    void        addCloudDrift(GLfloat uDrift, GLfloat vDrift);
    void        notifyStyleChange();

    int         getTriangleCount() const;
    void        resetTriangleCount();

private:
    void        drawSky(SceneRenderer&, bool mirror);
    void        drawSkybox();
    void        drawGround(void);
    void        drawGroundCentered(void);
    void        drawGroundGrid(SceneRenderer&);
    void        drawGroundShadows(SceneRenderer&);
    void        drawGroundReceivers(SceneRenderer&);
    void        drawAdvancedGroundReceivers(SceneRenderer&);
    void        drawMountains(void);

    void        resizeSky();

    void        doFreeDisplayLists();
    void        doInitDisplayLists();
    void        setSkyColors();
    void        makeCelestialLists(const SceneRenderer&);
    static void     freeContext(void*);
    static void     initContext(void*);
    static void     bzdbCallback(const std::string&, void*);

    // rendering state
    bool        blank;
    bool        invert;
    int         styleIndex;

    // stuff for ground
    OpenGLGState    groundGState[4];
    OpenGLGState    invGroundGState[4];
    Vertex_Chunk    simpleGroundList[2];
    int         groundTextureID;
    const GLfloat*  groundTextureMatrix;

    // stuff for grid
    GLfloat     gridSpacing;
    GLfloat     gridCount;
    OpenGLGState    gridGState;

    // stuff for ground receivers
    OpenGLGState    receiverGState;

    // stuff for mountains
    bool        mountainsAvailable;
    bool        mountainsVisible;
    int         numMountainTextures;
    int         mountainsMinWidth;
    OpenGLGState*   mountainsGState;
    std::vector<Vertex_Chunk> mountainsList;
    std::vector<glm::vec3>    mountainsVertex;
    std::vector<glm::vec3>    mountainsNormal;
    std::vector<glm::vec2>    mountainsTexture;

    // stuff for clouds
    GLfloat     cloudDriftU, cloudDriftV;
    bool        cloudsAvailable;
    bool        cloudsVisible;
    OpenGLGState    cloudsGState;
    Vertex_Chunk    cloudsList;
    std::vector<glm::vec4> cloudColor;
    std::vector<glm::vec3> cloudVertex;
    std::vector<glm::vec2> cloudTexture;

    // weather
    WeatherRenderer weather;

    // stuff for sun shadows
    bool        doShadows;
    bool        shadowsVisible;
    OpenGLGState    sunShadowsGState;

    // celestial stuff
    bool        haveSkybox;
    int         skyboxTexID[6];
    glm::vec4   skyboxColor[8];
    bool        doStars;
    bool        doSunset;
    glm::vec3   skyZenithColor;
    glm::vec3   skySunDirColor;
    glm::vec3   skyAntiSunDirColor;
    glm::vec3   skyCrossSunDirColor;
    glm::vec3   sunDirection;
    glm::vec3   moonDirection;
    float       sunsetTop;
    int         starGStateIndex;
    OpenGLGState    skyGState;
    OpenGLGState    sunGState;
    OpenGLGState    moonGState[2];
    OpenGLGState    starGState[2];
    glm::vec4              sun[21];
    Vertex_Chunk           sunXFormList;
    std::vector<glm::vec3> moonVertex;
    Vertex_Chunk           moonList;
    Vertex_Chunk           starXFormList;
    std::vector<glm::vec4> starsColor;
    std::vector<glm::vec4> starsVertex;
    std::vector<glm::vec3> transformedStarsVertex;

    static glm::vec3    skyPyramid[5];
    static const GLfloat    cloudRepeats;

    static glm::vec4    groundColor[4];
    static glm::vec4    groundColorInv[4];
    static const glm::vec4  defaultGroundColor[4];
    static const glm::vec4  defaultGroundColorInv[4];

    int         triangleCount;
};

//
// BackgroundRenderer
//

inline void     BackgroundRenderer::setBlank(bool _blank)
{
    blank = _blank;
}

inline void     BackgroundRenderer::setInvert(bool _invert)
{
    invert = _invert;
}

inline int      BackgroundRenderer::getTriangleCount() const
{
    return triangleCount;
}

inline void     BackgroundRenderer::resetTriangleCount()
{
    triangleCount = 0;
}


#endif // BZF_BACKGROUND_RENDERER_H

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
