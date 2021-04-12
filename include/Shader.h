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

// Before everything
#include "common.h"

// Syste headers
#include <string>
#include <glm/fwd.hpp>
#include <deque>

// Common headers
#include "bzfgl.h"

class Shader
{
public:
    Shader(const char *shaderName);
    virtual ~Shader();

// Called to initialize/destroy the shader
    virtual void init();
    virtual void destroy();

    GLint getAttribLocation(const char *attribute_name);
    GLint getUniformLocation(const char *uniform_name);
    void push();
    void pop();
    void setUniform(GLint uniform, bool value);
    void setUniform(GLint uniform, int value);
    void setUniform(GLint uniform, float value);
    void setUniform(GLint uniform, const glm::vec2 &value);
    void setUniform(GLint uniform, const glm::vec3 &value);
    void setUniform(GLint uniform, const glm::vec4 &value);
    void setUniform(GLint uniform, int count, GLint *value);
    void setUniform(GLint uniform, int count, const glm::vec3 value[]);
    void setUniform(GLint uniform, int count, const glm::vec4 value[]);
protected:
    bool inited;
private:
// Those are callback for GL Context creation/free
// Called by OpenGLGState::
    static void initContext(void* data);
    static void freeContext(void* data);

    static std::deque<GLuint> programStack;

    GLuint program;

    GLuint create_shader(const char *filename, GLenum type);
    void print_log(GLuint object);
    char* file_read(const std::string &filename);
    std::string vertexShader;
    std::string fragmentShader;
};

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
