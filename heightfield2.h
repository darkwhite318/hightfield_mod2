
/*
    pbrt source code Copyright(c) 1998-2012 Matt Pharr and Greg Humphreys.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PBRT_SHAPES_HEIGHTFIELD2_H
#define PBRT_SHAPES_HEIGHTFIELD2_H

// shapes/heightfield2.h*
#include "shape.h"

// Heightfield2 Declarations
class Heightfield2 : public Shape {
public:
    // Heightfield2 Public Methods
    Heightfield2(const Transform *o2, const Transform *w2o, bool ro, int nu, int nv, const float *zs);
    ~Heightfield2();
    bool CanIntersect() const;
    void Refine(vector<Reference<Shape> > &refined) const;
    bool Intersect(const Ray&,float*,float*,DifferentialGeometry*)const;
    bool IntersectP(const Ray&)const;
    bool IntersectCk(Point*,const Ray&,float*,float*,DifferentialGeometry*)const;
    bool IntersectCkP(Point*,const Ray&)const;
    BBox ObjectBound() const;
    Normal getNormal(int i, int j) const;
    Normal getNormalUnit(int nu, int x, int y)const;
    void initNormals();
    void Heightfield2::GetShadingGeometry(const Transform &obj2world,
        const DifferentialGeometry &dg,
        DifferentialGeometry *dgShading) const

private:
    // Heightfield2 Private Data
    float *z;
    int nx, ny;
    BBox bounds;
    Vector width;
    int nVoxels[2];
    Normal *all_normals;
};


Heightfield2 *CreateHeightfield2Shape(const Transform *o2w, const Transform *w2o,
        bool reverseOrientation, const ParamSet &params);

#endif // PBRT_SHAPES_HEIGHTFIELD2_H
