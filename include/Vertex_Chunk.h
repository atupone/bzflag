/* bzflag
 * Copyright (c) 2019-2019 Tim Riker
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

    Vertex_Chunk();
    Vertex_Chunk(Component comp, unsigned int size);
    ~Vertex_Chunk();

    Vertex_Chunk(Vertex_Chunk&& other);
    Vertex_Chunk& operator=(Vertex_Chunk&& data);

    // This method bind all the buffers, to be ready for drawing
    void enableArrays();
    // This method will select which buffer should be enabled
    // Vertex is always enabled
    void enableArrays(bool texture, bool normal, bool color);
    // This method will enable the vertex buffer only (used for shadows
    void enableVertexOnly();

    // These write the GL buffers at position index from an std::vector
    void vertexData(const std::vector<glm::vec3> vertices);
    void textureData(const std::vector<glm::vec2> textures);
    void normalData(const std::vector<glm::vec3> normals);
    void colorData(const std::vector<glm::vec4> colors);

    // These are for the case the caller provides a bidinmensional array
    void vertexData(const GLfloat vertices[][3]);
    void textureData(const GLfloat textures[][2]);
    void normalData(const GLfloat normals[][3]);
    void colorData(const GLfloat colors[][4]);

    // These are for the case the caller provides a bidinmensional array
    void vertexData(const GLfloat vertices[][3], unsigned int size);
    void textureData(const GLfloat textures[][2], unsigned int size);
    void normalData(const GLfloat normals[][3], unsigned int size);
    void colorData(const GLfloat colors[][4], unsigned int size);

    // These are for the case the caller provides a pointer to the first element
    void vertexData(const GLfloat *vertices, int size = -1);
    void textureData(const GLfloat *textures, int size = -1);
    void normalData(const GLfloat *normals, int size = -1);
    void colorData(const GLfloat *colors, int size = -1);

    // These are for the case the caller provides an array of glm::vec* elements
    void vertexData(const glm::vec3 vertices[]);
    void textureData(const glm::vec2 textures[]);
    void normalData(const glm::vec3 normals[]);
    void colorData(const glm::vec4 colors[]);

    // These are for the case the caller provides an array of glm::vec* elements
    void vertexData(const glm::vec3 vertices[], int size);
    void textureData(const glm::vec2 textures[], int size);
    void normalData(const glm::vec3 normals[], int size);
    void colorData(const glm::vec4 colors[], int size);

    void glDrawArrays(GLenum mode);
    void glDrawArrays(GLenum mode, unsigned int size, unsigned int offset = 0);

    void draw(GLenum mode);
    void draw(GLenum mode, bool onlyVertex);
    void draw(GLenum mode, bool onlyVertex, int size);

    unsigned int getIndex() const;
private:
    static VBO_Vertex  *vbo[VLast];
    static unsigned int refer[VLast];

    VBO_Vertex   *myVBO;
    int           index;
    unsigned int  indexSize;
    Component     myComp;
};

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
