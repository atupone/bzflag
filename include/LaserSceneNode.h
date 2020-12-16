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

/* LaserSceneNode:
 *  Encapsulates information for rendering a laser beam.
 */

#ifndef BZF_LASER_SCENE_NODE_H
#define BZF_LASER_SCENE_NODE_H

// Inherits from
#include "SceneNode.h"

// System headers
#include <glm/vec4.hpp>

// Global headers
#include "Vertex_Chunk.h"


class LaserSceneNode : public SceneNode
{
public:
    LaserSceneNode(const GLfloat pos[3],
                   const GLfloat forward[3]);
    ~LaserSceneNode();

    void        setTexture(const int);

    bool        cull(const ViewFrustum&) const;

    void        notifyStyleChange();
    void        addRenderNodes(SceneRenderer&);

    void        setColor ( float r, float g, float b );
    void        setCenterColor ( float r, float g, float b );
    void        setFirst ( void )
    {
        first = true;
    }

protected:
    class LaserRenderNode : public RenderNode
    {
    public:
        LaserRenderNode(const LaserSceneNode*);
        ~LaserRenderNode();
        void        render() override;
        const glm::vec3 getPosition() const override;
    private:
        void renderFlatLaser();
        void renderGeoLaser();
        const LaserSceneNode* sceneNode;
        static Vertex_Chunk laserTexture1;
        static Vertex_Chunk laserTexture2;
        static Vertex_Chunk laserNoTexture1;
    };
    glm::vec4 color;
    glm::vec4 centerColor;
    bool first;
    friend class LaserRenderNode;

private:
    GLfloat     azimuth, elevation;
    GLfloat     length;
    bool        texturing;
    OpenGLGState    gstate;
    LaserRenderNode renderNode;
};

#endif // BZF_LASER_SCENE_NODE_H

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
