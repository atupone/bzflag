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
 * RadarRenderer:
 *  Encapsulates drawing a radar
 */

#ifndef BZF_RADAR_RENDERER_H
#define BZF_RADAR_RENDERER_H

// Always first
#include "common.h"

// System headers
#include <vector>

// Common headers
#include "bzfgl.h"
#include "Obstacle.h"
#include "Vertex_Chunk.h"


class SceneRenderer;
class World;
class ShotPath;

class RadarRenderer
{
public:
    RadarRenderer(const SceneRenderer&, World* _world);
    void        setWorld(World* _world);

    void        setControlColor(const glm::vec3 &color = glm::vec3(0.0f));

    int         getX() const;
    int         getY() const;
    int         getWidth() const;
    int         getHeight() const;

    void        setShape(int x, int y, int w, int h);
    void        setJammed(bool = true);

    void        setDimming(float newDimming);

    void        render(SceneRenderer&, bool blank, bool observer);

    void        renderFrame(SceneRenderer&);

    void        renderObstacles(bool fastRadar, float range);
    void        renderWalls();
    void        renderBoxPyrMesh();
    void        renderBoxPyrMeshFast(float range);
    void        renderBasesAndTeles();

    int         getFrameTriangleCount() const;

private:
    // no copying
    RadarRenderer(const RadarRenderer&);
    RadarRenderer&  operator=(const RadarRenderer&);

    glm::vec3   getTankColor(const class Player *player);
    void        drawTank(const glm::vec3 &pos,
                         const class Player* player,
                         bool useSquares);
    void        drawFancyTank(const class Player* player);
    void        drawFlag(const glm::vec3 &pos);
    void        drawFlagOnTank();
    void        buildBoxPyrMeshVBO();
    void        buildWallsVBO();
    void        buildBasesAndTelesVBO();

    static float    colorScale(const float z, const float h);
    static float    transScale(const float z, const float h);

private:
    World*      world;
    int         x, y;
    int         w, h;
    float       dimming;
    float       ps;
    float       range;
    double      decay;
    glm::vec3   teamColor;
    bool        smooth;
    bool        jammed;
    bool        useTankModels;
    int         triangleCount;
    std::vector<std::vector<Vertex_Chunk>> meshVBO;
    std::vector<Vertex_Chunk>              boxVBO;
    std::vector<Vertex_Chunk>              pyrVBO;
    Vertex_Chunk                           wallsVBO;
    std::vector<std::vector<Vertex_Chunk>> baseVBO;
    std::vector<Vertex_Chunk>              telesVBO;
    std::vector<Vertex_Chunk>              boxOutlVBO;
    std::vector<Vertex_Chunk>              pyrOutlVBO;
    static const float  colorFactor;
};

//
// RadarRenderer
//

inline int      RadarRenderer::getX() const
{
    return x;
}

inline int      RadarRenderer::getY() const
{
    return y;
}

inline int      RadarRenderer::getWidth() const
{
    return w;
}

inline int      RadarRenderer::getHeight() const
{
    return h;
}

#endif // BZF_RADAR_RENDERER_H

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
