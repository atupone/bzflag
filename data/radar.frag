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

#version 110

uniform int       radarKind;
uniform bool      texturing;
uniform sampler2D ourTexture;
uniform vec4      baseData[2];
uniform vec3      objColor;
uniform float     alphaDim;

const int enhancedRadar = 0;
const int fastRadar     = 1;
const int emulFixed     = 2;
const int jammed        = 3;
const int viewLine      = 4;
const int flag          = 5;
const int dimming       = 6;
const int tele          = 7;
const int shot          = 8;
const int solidColor    = 10;

void main(void)
{
    vec4 fragColor;
    if (radarKind == jammed)
        fragColor = texture2D(ourTexture, gl_TexCoord[0].st);
    else if (radarKind == viewLine)
        fragColor = vec4(1.0, 0.625, 0.125, 1.0);
    else if (radarKind == flag)
        fragColor = gl_Color;
    else if (radarKind == dimming)
        fragColor = vec4(0.0, 0.0, 0.0, alphaDim);
    else if (radarKind == enhancedRadar)
        fragColor = gl_Color;
    else if (radarKind == fastRadar)
        fragColor = texture2D(ourTexture, gl_TexCoord[0].st);
    else if (radarKind == tele)
        fragColor = gl_Color;
    else if (radarKind == shot)
        fragColor = gl_Color;
    else if (radarKind == solidColor)
        fragColor = vec4(objColor, 1.0);
    else
    {
        fragColor = gl_Color;
        if (texturing)
            fragColor *= texture2D(ourTexture, gl_TexCoord[0].st);
    }
    gl_FragColor = fragColor;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4 ft=cpp
