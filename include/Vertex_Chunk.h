/* bzflag
 * Copyright (c) 2019-2026 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#pragma once

// 1st
#include "common.h"

// system interface headers
#include <glm/fwd.hpp>
#include <vector>

// Common headers
#include "bzfgl.h"

// forward declaration
class VBO_Vertex;

class Vertex_Chunk
{
public:
    enum Component {V=0, VC, VN, VT, VTN, VTC, VTNC, VLast};
    enum Usage { Static, Dynamic }; // Usage classification

    Vertex_Chunk() = default;
    Vertex_Chunk(Component comp, unsigned int size, Usage = Static);
    ~Vertex_Chunk();

    Vertex_Chunk(Vertex_Chunk&& other) noexcept;
    Vertex_Chunk& operator=(Vertex_Chunk&& data) noexcept;

    // This method will select which buffer should be enabled
    // Vertex is always enabled
    void enableArrays(bool texture = true, bool normal = true, bool color = true);

    // Syncs local chunk data to GPU VRAM for interlaced buffers
    void syncInterlaced();

    // Direct GPU streaming setup - static helper to orphan dynamic VBOs at frame start
    static void orphanDynamicBuffers();

    // These write the GL buffers from an std::vector
    void vertexData(const std::vector<glm::vec3>& vertices);
    void textureData(const std::vector<glm::vec2>& textures);
    void normalData(const std::vector<glm::vec3>& normals);
    void colorData(const std::vector<glm::vec4>& colors);

    // These are for the case the caller provides a bidimensional array
    void vertexData(const GLfloat vertices[][3], int size = -1);
    void textureData(const GLfloat textures[][2], int size = -1);
    void normalData(const GLfloat normals[][3], int size = -1);
    void colorData(const GLfloat colors[][4], int size = -1);

    // These are for the case the caller provides a pointer to the first element
    void vertexData(const GLfloat *vertices, int size = -1);
    void textureData(const GLfloat *textures, int size = -1);
    void normalData(const GLfloat *normals, int size = -1);
    void colorData(const GLfloat *colors, int size = -1);

    // These are for the case the caller provides an array of glm::vec* elements
    void vertexData(const glm::vec3 vertices[], int size = -1);
    void textureData(const glm::vec2 textures[], int size = -1);
    void normalData(const glm::vec3 normals[], int size = -1);
    void colorData(const glm::vec4 colors[], int size = -1);

    inline void glDrawArrays(GLenum mode)
    {
        ::glDrawArrays(mode, index, indexSize);
    }

    inline void glDrawArrays(GLenum mode, unsigned int size, unsigned int offset = 0)
    {
        ::glDrawArrays(mode, index + offset, size);
    }

    inline void draw(GLenum mode)
    {
        enableArrays();
        glDrawArrays(mode);
    }
    inline void draw(GLenum mode, bool onlyVertex)
    {
        if (onlyVertex)
            enableArrays(false, false, false);
        else
            enableArrays();
        glDrawArrays(mode);
    }
    void draw(GLenum mode, bool onlyVertex, int size)
    {
        if (onlyVertex)
            enableArrays(false, false, false);
        else
            enableArrays();
        glDrawArrays(mode, size);
    }

    inline unsigned int getIndex() const
    {
        return index;
    }

    unsigned int indexSize{0}; // size of the chunk

private:
    static VBO_Vertex*  staticVBO[VLast];
    static VBO_Vertex*  dynamicVBO[VLast];
    static unsigned int staticRefer[VLast];
    static unsigned int dynamicRefer[VLast];

    VBO_Vertex* createVBOForComponent(Component comp, bool interlaced);

    int         index{0}; // start of chunk in the buffer
    Component   myComp{V};
    Usage       myUsage{Static};

    VBO_Vertex**  vbo{nullptr};
    unsigned int* refer{nullptr};
    VBO_Vertex*   myVBO{nullptr};
};


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
