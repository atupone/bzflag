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

#include "TankGeometryMgr.h"
using namespace TankGeometryUtils;

float sideVerts[][3] = {  {2.800000f, 0.878000f, 0.829000f},
    {2.820000f, 0.878000f, 0.716000f},
    {2.680000f, 0.877000f, 0.684000f},
    {-1.460000f, 0.877000f, 1.130000f},
    {-2.740000f, 0.877000f, 0.528000f},
    {-2.900000f, 0.877000f, 1.240000f},
    {2.570000f, 0.877000f, 0.990000f},
    {2.170000f, 0.877000f, 1.010000f},
    {2.610000f, 0.877000f, 0.408000f},
    {1.810000f, 0.877000f, 0.250000f},
    {-1.620000f, 0.877000f, 0.25000f},
    {1.760000f, 0.877000f, 1.130000f},
    {2.800000f, -0.876000f, 0.829000f},
    {2.680000f, -0.877000f, 0.684000f},
    {2.820000f, -0.877000f, 0.716000f},
    {-2.740000f, -0.877000f, 0.528000f},
    {-1.460000f, -0.877000f, 1.130000f},
    {-2.900000f, -0.877000f, 1.240000f},
    {2.570000f, -0.877000f, 0.990000f},
    {2.170000f, -0.877000f, 1.010000f},
    {2.610000f, -0.877000f, 0.408000f},
    {1.810000f, -0.877000f, 0.250000f},
    {-1.620000f, -0.877000f, 0.250000f},
    {1.760000f, -0.877000f, 1.130000f}
};

float sideUVs[][2] = {{0.996503f, 0.331452f},
    {1.000000f, 0.422581f},
    {0.975524f, 0.448387f},
    {0.251748f, 0.088710f},
    {0.027972f, 0.574194f},
    {0.000000f, 0.000000f},
    {0.956294f, 0.201613f},
    {0.886364f, 0.185484f},
    {0.963287f, 0.670968f},
    {0.823427f, 1.000000f},
    {0.223776f, 1.000000f},
    {0.814685f, 0.088710f}
};

float sideNormals[][3] = {{-0.006338f, 0.999979f, -0.001652f},
    {-0.003169f, 0.999995f, -0.000826f},
    {0.000000f, 1.000000f, 0.000000f},
    {0.001933f, -0.999984f, 0.005297f},
    {0.000966f, -0.999996f, 0.002648f},
    {-0.001944f, -0.999962f, 0.008505f},
    {0.000000f, -1.000000f, 0.000000f},
    {0.002905f, -0.999995f, 0.001044f}
};


static void DrawOBJIndexVertex (int v, int t, int n)
{
    doNormal3f(sideNormals[n-1][0], sideNormals[n-1][1],sideNormals[n-1][2]);
    doTexCoord2f(sideUVs[t-1][0],sideUVs[t-1][1]);
    doVertex3f(sideVerts[v-1][0], sideVerts[v-1][1], sideVerts[v-1][2]);
}


static int DrawTankSides (void)
{
    DrawOBJIndexVertex(1,   1, 1);
    DrawOBJIndexVertex(2,   2, 1);
    DrawOBJIndexVertex(7,   7, 2);
    DrawOBJIndexVertex(3,   3, 2);
    DrawOBJIndexVertex(8,   8, 3);
    DrawOBJIndexVertex(9,   9, 3);
    DrawOBJIndexVertex(12, 12, 3);
    DrawOBJIndexVertex(10, 10, 3);
    DrawOBJIndexVertex(4,   4, 3);
    DrawOBJIndexVertex(11, 11, 3);
    DrawOBJIndexVertex(6,   6, 3);
    DrawOBJIndexVertex(5,   5, 3);

    // Degenerate triangles
    DrawOBJIndexVertex(5,   5, 3);
    DrawOBJIndexVertex(15,  2, 6);

    DrawOBJIndexVertex(15,  2, 6);
    DrawOBJIndexVertex(13,  1, 4);
    DrawOBJIndexVertex(14,  3, 5);
    DrawOBJIndexVertex(19,  7, 8);
    DrawOBJIndexVertex(21,  9, 7);
    DrawOBJIndexVertex(20,  8, 7);
    DrawOBJIndexVertex(22, 10, 7);
    DrawOBJIndexVertex(24, 12, 7);
    DrawOBJIndexVertex(23, 11, 7);
    DrawOBJIndexVertex(17,  4, 7);
    DrawOBJIndexVertex(16,  5, 7);
    DrawOBJIndexVertex(18,  6, 7);

    // Degenerate triangle
    DrawOBJIndexVertex(18,  6, 7);
    return 20;
}

static int DrawCentralBody(void)
{
    // Degenerate triangle
    doVertex3f(2.820f, -0.877f, 0.716f);

    // draw the outer loop
    doNormal3f(0.984696f, 0.000000f, 0.174282f);
    doTexCoord2f(1.210f, 2.290f);
    doVertex3f(2.820f, -0.877f, 0.716f);
    doTexCoord2f(0.669f, 2.040f);
    doVertex3f(2.820f, 0.878f, 0.716f);
    doTexCoord2f(1.210f, 2.280f);
    doVertex3f(2.800f, -0.876f, 0.829f);
    doTexCoord2f(0.672f, 2.030f);
    doVertex3f(2.800f, 0.878f, 0.829f);
    doNormal3f(0.573462f, 0.000000f, 0.819232f);
    doTexCoord2f(1.240f, 2.210f);
    doVertex3f(2.570f, -0.877f, 0.990f);
    doTexCoord2f(0.705f, 1.960f);
    doVertex3f(2.570f, 0.877f, 0.990f);
    doNormal3f(0.049938f, 0.000000f, 0.998752f);
    doTexCoord2f(1.300f, 2.090f);
    doVertex3f(2.170f, -0.877f, 1.010f);
    doTexCoord2f(0.763f, 1.840f);
    doVertex3f(2.170f, 0.877f, 1.010f);
    doNormal3f(0.280899f, 0.000000f, 0.959737f);
    doTexCoord2f(1.360f, 1.970f);
    doVertex3f(1.760f, -0.877f, 1.130f);
    doTexCoord2f(0.822f, 1.710f);
    doVertex3f(1.760f, 0.877f, 1.130f);
    doNormal3f(0.000000f, 0.000000f, 1.000000f);
    doTexCoord2f(1.820f, 0.981f);
    doVertex3f(-1.460f, -0.877f, 1.130f);
    doTexCoord2f(1.280f, 0.729f);
    doVertex3f(-1.460f, 0.877f, 1.130f);
    doNormal3f(0.076167f, 0.000000f, 0.997095f);
    doTexCoord2f(2.030f, 0.541f);
    doVertex3f(-2.900f, -0.877f, 1.240f);
    doTexCoord2f(1.490f, 0.289f);
    doVertex3f(-2.900f, 0.877f, 1.240f);
    doNormal3f(-0.975668f, 0.000000f, -0.219251f);
    doTexCoord2f(2.000f, 0.590f);
    doVertex3f(-2.740f, -0.877f, 0.528f);
    doTexCoord2f(1.470f, 0.338f);
    doVertex3f(-2.740f, 0.877f, 0.528f);
    doNormal3f(-0.426419f, 0.000000f, -0.904526f);
    doTexCoord2f(1.840f, 0.932f);
    doVertex3f(-1.620f, -0.877f, 0.250f);
    doTexCoord2f(1.310f, 0.680f);
    doVertex3f(-1.620f, 0.877f, 0.250f);
    doNormal3f(0.000000f, 0.000000f, -1.000000f);
    doTexCoord2f(1.350f, 1.980f);
    doVertex3f(1.810f, -0.877f, 0.250f);
    doTexCoord2f(0.815f, 1.730f);
    doVertex3f(1.810f, 0.877f, 0.250f);
    doNormal3f(0.454326f, 0.000000f, -0.890835f);
    doTexCoord2f(1.240f, 2.230f);
    doVertex3f(2.610f, -0.877f, 0.408f);
    doTexCoord2f(0.700f, 1.970f);
    doVertex3f(2.610f, 0.877f, 0.408f);
    doNormal3f(0.969310f, 0.000000f, -0.245840f);
    doTexCoord2f(1.230f, 2.250f);
    doVertex3f(2.680f, -0.877f, 0.684f);
    doTexCoord2f(0.690f, 2.000f);
    doVertex3f(2.680f, 0.877f, 0.684f);
    doNormal3f(0.222825f, 0.000000f, -0.974858f);
    doTexCoord2f(1.210f, 2.290f);
    doVertex3f(2.820f, -0.877f, 0.716f);
    doTexCoord2f(0.669f, 2.040f);
    doVertex3f(2.820f, 0.878f, 0.716f);

    // Degenerate triangle
    doVertex3f(2.820f, 0.878f, 0.716f);
    return 24;
}


static int DrawRightRearExaust (void)
{
    // Degenerate triangle
    doVertex3f(-2.820f, 0.686f, 1.070f);

    doNormal3f(0.000000f, 1.000000f, 0.000000f);
    doTexCoord2f(1.540f, 0.341f);
    doVertex3f(-2.820f, 0.686f, 1.070f);
    doTexCoord2f(1.580f, 0.261f);
    doVertex3f(-3.080f, 0.686f, 1.070f);
    doTexCoord2f(1.540f, 0.341f);
    doVertex3f(-2.820f, 0.686f, 1.170f);
    doTexCoord2f(1.580f, 0.261f);
    doVertex3f(-3.080f, 0.686f, 1.170f);
    doNormal3f(0.000000f, 0.000000f, 1.000000f);
    doTexCoord2f(1.640f, 0.387f);
    doVertex3f(-2.820f, 0.367f, 1.170f);
    doTexCoord2f(1.670f, 0.307f);
    doVertex3f(-3.080f, 0.367f, 1.170f);
    doNormal3f(0.000000f, -1.000000f, 0.000000f);
    doTexCoord2f(1.640f, 0.387f);
    doVertex3f(-2.820f, 0.367f, 1.070f);
    doTexCoord2f(1.670f, 0.307f);
    doVertex3f(-3.080f, 0.367f, 1.070f);
    doNormal3f(0.000000f, 0.000000f, -1.000000f);
    doTexCoord2f(1.540f, 0.341f);
    doVertex3f(-2.820f, 0.686f, 1.070f);
    doTexCoord2f(1.580f, 0.261f);
    doVertex3f(-3.080f, 0.686f, 1.070f);

    // Degenerate triangles
    doVertex3f(-3.080f, 0.686f, 1.070f);
    doVertex3f(-3.080f, 0.686f, 1.170f);

    doNormal3f(-1.000000f, 0.000000f, 0.000000f);
    doTexCoord2f(1.580f, 0.261f);
    doVertex3f(-3.080f, 0.686f, 1.170f);
    doTexCoord2f(1.580f, 0.261f);
    doVertex3f(-3.080f, 0.686f, 1.070f);
    doTexCoord2f(1.670f, 0.307f);
    doVertex3f(-3.080f, 0.367f, 1.170f);
    doTexCoord2f(1.670f, 0.307f);
    doVertex3f(-3.080f, 0.367f, 1.070f);

    // Degenerate triangle
    doVertex3f(-3.080f, 0.367f, 1.070f);

    return 10;
}


static int DrawLeftRearExaust ( void )
{
    // Degenerate triangle
    doVertex3f(-2.840f, -0.084f, 1.070f);

    doNormal3f(0.000000f, 1.000000f, 0.000000f);
    doTexCoord2f(1.780f, 0.445f);
    doVertex3f(-2.840f, -0.084f, 1.070f);
    doTexCoord2f(1.810f, 0.366f);
    doVertex3f(-3.100f, -0.084f, 1.070f);
    doTexCoord2f(1.780f, 0.445f);
    doVertex3f(-2.840f, -0.084f, 1.170f);
    doTexCoord2f(1.810f, 0.366f);
    doVertex3f(-3.100f, -0.084f, 1.170f);
    doNormal3f(0.000000f, 0.000000f, 1.000000f);
    doTexCoord2f(2.020f, 0.559f);
    doVertex3f(-2.840f, -0.877f, 1.170f);
    doTexCoord2f(2.060f, 0.480f);
    doVertex3f(-3.100f, -0.877f, 1.170f);
    doNormal3f(0.000000f, -1.000000f, 0.000000f);
    doTexCoord2f(2.020f, 0.559f);
    doVertex3f(-2.840f, -0.877f, 1.070f);
    doTexCoord2f(2.060f, 0.480f);
    doVertex3f(-3.100f, -0.877f, 1.070f);
    doNormal3f(0.000000f, 0.000000f, -1.000000f);
    doTexCoord2f(1.780f, 0.445f);
    doVertex3f(-2.840f, -0.084f, 1.070f);
    doTexCoord2f(1.810f, 0.366f);
    doVertex3f(-3.100f, -0.084f, 1.070f);

    // Degenerate triangles
    doVertex3f(-3.100f, -0.084f, 1.070f);
    doVertex3f(-3.100f, -0.084f, 1.170f);

    doNormal3f(-1.000000f, 0.000000f, 0.000000f);
    doTexCoord2f(1.810f, 0.366f);
    doVertex3f(-3.100f, -0.084f, 1.170f);
    doTexCoord2f(1.810f, 0.366f);
    doVertex3f(-3.100f, -0.084f, 1.070f);
    doTexCoord2f(2.060f, 0.480f);
    doVertex3f(-3.100f, -0.877f, 1.170f);
    doTexCoord2f(2.060f, 0.480f);
    doVertex3f(-3.100f, -0.877f, 1.070f);

    return 10;
}


int TankGeometryUtils::buildHighBody (void)
{
    int tris = 0;
    tris += DrawTankSides();
    tris += DrawCentralBody();
    tris += DrawRightRearExaust();
    tris += DrawLeftRearExaust();
    return tris;
}

/*
 * Local Variables: ***
 * mode: C ***
 * tab-width: 8 ***
 * c-basic-offset: 2 ***
 * indent-tabs-mode: t ***
 * End: ***
 * ex: shiftwidth=4 tabstop=4
 */
