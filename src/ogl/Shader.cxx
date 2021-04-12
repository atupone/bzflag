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

// Interface
#include "Shader.h"

// System headers
#include <iostream>
#include <string>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Common header
#include "ErrorHandler.h"
#include "FileManager.h"
#include "OpenGLGState.h"

std::deque<GLuint> Shader::programStack;
Shader::Shader(const char *shaderName) : inited(false)
{
    vertexShader   = std::string(shaderName) + ".vert";
    fragmentShader = std::string(shaderName) + ".frag";
    OpenGLGState::registerContextInitializer(freeContext, initContext, this);
}

Shader::~Shader()
{
    OpenGLGState::unregisterContextInitializer(freeContext, initContext, this);
}

void Shader::initContext(void*)
{
}

void Shader::freeContext(void* data)
{
    static_cast<Shader*>(data)->destroy();
}

void Shader::init()
{
    GLuint vs, fs;
    GLint link_ok = GL_FALSE;
    vs = create_shader(vertexShader.c_str(), GL_VERTEX_SHADER);
    fs = create_shader(fragmentShader.c_str(), GL_FRAGMENT_SHADER);
    if (!vs && !fs)
    {
        printFatalError ("glsl file for shader %s and %s not found",
                         vertexShader.c_str(), fragmentShader.c_str());
        exit(0);
    }

    program = glCreateProgram();
    if (vs)
        glAttachShader(program, vs);
    if (fs)
        glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!link_ok)
    {
        std::cerr << "glLinkProgram:";
        print_log(program);
        exit(0);
    }
    inited = true;
}

void Shader::destroy()
{
    inited = false;
    glDeleteProgram(program);
}

GLuint Shader::create_shader(const char* filename, GLenum type)
{
    GLchar* source = file_read(filename);
    if (source == NULL)
        return 0;

    GLuint res = glCreateShader(type);

    const GLchar* sources[] = { source };
    glShaderSource(res, 1, sources, NULL);
    free(source);

    glCompileShader(res);
    GLint compile_ok = GL_FALSE;
    glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);
    if (compile_ok == GL_FALSE)
    {
        std::cerr << filename << ":";
        print_log(res);
        glDeleteShader(res);
        return 0;
    }

    return res;
}

char* Shader::file_read(const std::string &filename)
{
    std::istream *rw = FILEMGR.createDataInStream(filename, true);
    if (rw == NULL) return NULL;

    // get the shader size
    rw->seekg(0, std::ios::end);
    std::streampos size = rw->tellg();
    unsigned long res_size = (unsigned long)std::streamoff(size);

    // load the shader
    rw->seekg(0);

    char* res = (char*)malloc(res_size + 1);

    rw->read(res, res_size);
    delete rw;
    res[res_size] = '\0';
    return res;
}

/*
 * Display compilation errors from the OpenGL shader compiler
 */
void Shader::print_log(GLuint object)
{
    GLint log_length = 0;
    if (glIsShader(object))
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);

    else if (glIsProgram(object))
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);

    else
    {
        std::cerr << "printlog: Not a shader or a program" << std::endl;
        return;
    }

    char* log = (char*)malloc(log_length);

    if (glIsShader(object))
        glGetShaderInfoLog(object, log_length, NULL, log);
    else if (glIsProgram(object))
        glGetProgramInfoLog(object, log_length, NULL, log);

    std::cerr << log;
    free(log);
}

GLint Shader::getAttribLocation(const char *attribute_name)
{
    GLint attrib = glGetAttribLocation(program, attribute_name);
    if (attrib == -1)
        std::cerr << "Could not bind attribute " << attribute_name << std::endl;
    return attrib;
}

GLint Shader::getUniformLocation(const char *uniform_name)
{
    GLint uniform = glGetUniformLocation(program, uniform_name);
    if (uniform == -1)
        std::cerr << "Could not bind uniform " << uniform_name << std::endl;
    return uniform;
}

void Shader::setUniform(GLint uniform, bool value)
{
    if (!inited)
        return;
    if (programStack.empty())
        return;
    if (program != programStack.back())
        return;
    glUniform1i(uniform, value);
}

void Shader::setUniform(GLint uniform, float value)
{
    if (!inited)
        return;
    if (programStack.empty())
        return;
    if (program != programStack.back())
        return;
    glUniform1f(uniform, value);
}

void Shader::setUniform(GLint uniform, int value)
{
    if (!inited)
        return;
    if (programStack.empty())
        return;
    if (program != programStack.back())
        return;
    glUniform1i(uniform, value);
}

void Shader::setUniform(GLint uniform, const glm::vec2 &value)
{
    if (!inited)
        return;
    if (programStack.empty())
        return;
    if (program != programStack.back())
        return;
    glUniform2f(uniform, value[0], value[1]);
}

void Shader::setUniform(GLint uniform, const glm::vec3 &value)
{
    if (!inited)
        return;
    if (programStack.empty())
        return;
    if (program != programStack.back())
        return;
    glUniform3f(uniform, value[0], value[1], value[2]);
}

void Shader::setUniform(GLint uniform, const glm::vec4 &value)
{
    if (!inited)
        return;
    if (programStack.empty())
        return;
    if (program != programStack.back())
        return;
    glUniform4f(uniform, value[0], value[1], value[2], value[3]);
}

void Shader::setUniform(GLint uniform, int count, GLint *value)
{
    if (!inited)
        return;
    if (programStack.empty())
        return;
    if (program != programStack.back())
        return;
    glUniform1iv(uniform, count, value);
}

void Shader::setUniform(GLint uniform, int count, const glm::vec3 value[])
{
    if (!inited)
        return;
    if (programStack.empty())
        return;
    if (program != programStack.back())
        return;
    glUniform3fv(uniform, count, glm::value_ptr(value[0]));
}

void Shader::setUniform(GLint uniform, int count, const glm::vec4 value[])
{
    if (!inited)
        return;
    if (programStack.empty())
        return;
    if (program != programStack.back())
        return;
    glUniform4fv(uniform, count, glm::value_ptr(value[0]));
}

void Shader::push()
{
    if (!inited)
        init();
    programStack.push_back(program);
    glUseProgram(program);
}

void Shader::pop()
{
    programStack.pop_back();
    if (programStack.empty())
        glUseProgram(0);
    else
        glUseProgram(programStack.back());
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
