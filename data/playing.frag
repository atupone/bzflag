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

uniform bool lighting;
uniform int  fogMode;
uniform bool fogging;
uniform sampler2D ourTexture;
uniform bool      texturing;
uniform bool      replaceTexture;

void main(void)
{
    const int FogExp    = 0;
    const int FogExp2   = 1;
    const int FogLinear = 2;

    vec4 color;
    vec3 secondaryColor;

    color          = gl_Color;
    secondaryColor = gl_SecondaryColor.rgb;

    if (texturing)
    {
        vec4 texColor = texture2D(ourTexture, gl_TexCoord[0].st);
        if (replaceTexture)
            color  = texColor;
        else
            color *= texColor;
    }
    if (lighting)
        color.rgb = color.rgb + secondaryColor;
    color = clamp(color, 0.0, 1.0);

    if (fogging)
    {
        float fogFactor = 1.0;
        if (fogMode == FogExp)
            fogFactor = exp(-gl_Fog.density * gl_FogFragCoord);
        else if (fogMode == FogExp2)
            fogFactor = exp(-pow((gl_Fog.density * gl_FogFragCoord), 2.0));
        else if (fogMode == FogLinear)
            fogFactor = (gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale;
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        color = mix(gl_Fog.color, color, fogFactor);
    }
    gl_FragColor = color;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4 ft=cpp
