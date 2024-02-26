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

/* SceneNode:
 *  Encapsulates information for rendering an object in the scene.
 *
 * Probably shouldn't use names like this (GLfloat...).  Oh well.
 */

#ifndef BZF_SCENE_NODE_H
#define BZF_SCENE_NODE_H

// top include
#include "common.h"

// System headers
#include <vector>
#include <glm/vec3.hpp>

// Common headers
#include "bzfgl.h"
#include "bzfio.h"
#include "OpenGLGState.h"
#include "RenderNode.h"
#include "Extents.h"

#if !defined(_WIN32)
// bonehead win32 cruft.  just make it go away on other platforms.
#ifdef __stdcall
#undef __stdcall
#endif
#define __stdcall
#endif

#define myColor3f(r, g, b)  SceneNode::glColor3f(r, g, b)
#define myColor4f(r, g, b, a)   SceneNode::glColor4f(r, g, b, a)
#define myColor3fv(rgb)     SceneNode::glColor3fv(rgb)
#define myColor4fv(rgba)    SceneNode::glColor4fv(rgba)

class ViewFrustum;
class SceneRenderer;

class SceneNode
{
public:
    SceneNode();
    virtual     ~SceneNode();

    virtual void    notifyStyleChange();

    const glm::vec3 &getSphere() const;
    const float     &getRadius2() const;
    const Extents&  getExtents() const;
    virtual int     getVertexCount () const;
    virtual const glm::vec3 &getVertex (int vertex) const;
    virtual const glm::vec4 *getPlane() const;
    virtual GLfloat getDistance(const glm::vec3 &eye) const; // for BSP

    virtual bool    inAxisBox (const Extents& exts) const;

    virtual bool    cull(const ViewFrustum&) const;
    virtual bool    cullShadow(int pCount, const glm::vec4 planes[]) const;

    bool        isOccluder() const;
    void        setOccluder(bool value);

    virtual void    addLight(SceneRenderer&);
    virtual void    addShadowNodes(SceneRenderer&);
    virtual void    addRenderNodes(SceneRenderer&);
    virtual void    renderRadar();

    struct RenderSet
    {
        RenderNode* node;
        const OpenGLGState* gstate;
    };
    virtual void getRenderNodes(std::vector<RenderSet>& rnodes);


    static void     setColorOverride(bool = true);
    static void     glColor3f(GLfloat r, GLfloat g, GLfloat b)
    {
        if (!colorOverride) ::glColor3f(r, g, b);
    };

    static void     glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
    {
        if (!colorOverride) ::glColor4f(r, g, b, a);
    };

    static void glColor3fv(const glm::vec3 &rgb)
    {
        if (!colorOverride) ::glColor3f(rgb.r, rgb.g, rgb.b);
    };

    static void glColor4fv(const glm::vec4 &rgba)
    {
        if (!colorOverride)
            ::glColor4f(rgba.r, rgba.g, rgba.b, rgba.a);
    };

    static void glVertex2fv(const glm::vec2 &pos)
    {
        ::glVertex2f(pos.x, pos.y);
    }

    static void glVertex3fv(const glm::vec3 &pos)
    {
        ::glVertex3f(pos.x, pos.y, pos.z);
    }

    static void glTexCoord2fv(const glm::vec2 &coord)
    {
        ::glTexCoord2f(coord.s, coord.t);
    }

    static void     setStipple(GLfloat alpha)
    {
        (*stipple)(alpha);
    }

    static void glNormal3fv(const glm::vec3 &normal)
    {
        ::glNormal3f(normal.x, normal.y, normal.z);
    }

    enum CullState
    {
        OctreeCulled,
        OctreePartial,
        OctreeVisible
    };

    /** This boolean is used by the Octree code.
    Someone else can 'friend'ify it later.
    */
    CullState octreeState;

protected:
    void        setRadius(GLfloat radiusSquared);
    void        setCenter(const glm::vec3 &center);

private:
    SceneNode(const SceneNode&);
    SceneNode&      operator=(const SceneNode&);



    static void         noStipple(GLfloat);

protected:
    bool        occluder;
    Extents     extents;
private:
    glm::vec3   sphere;
    float       radius2;
    static bool  colorOverride;
    static void     (*stipple)(GLfloat);
};

inline const glm::vec3 &SceneNode::getSphere() const
{
    return sphere;
}

inline const float &SceneNode::getRadius2() const
{
    return radius2;
}

inline const Extents&   SceneNode::getExtents() const
{
    return extents;
}

inline bool     SceneNode::isOccluder() const
{
    return occluder;
}

inline void     SceneNode::setOccluder(bool value)
{
    occluder = value;
}
#endif // BZF_SCENE_NODE_H

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
