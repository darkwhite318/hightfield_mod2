
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


// shapes/heightfield2.cpp*
#include "stdafx.h"
#include "shapes/heightfield2.h"
#include "shapes/trianglemesh.h"
#include "paramset.h"
//#include <iostream>

// Heightfield2 Method Definitions
Heightfield2::Heightfield2(const Transform *o2w, const Transform *w2o,
        bool ro, int x, int y, const float *zs)
    : Shape(o2w, w2o, ro) {
    nx = x;
    ny = y;
    z = new float[nx*ny];
    memcpy(z, zs, nx*ny*sizeof(float));
    
     bounds = ObjectBound();
     width = Vector(1.f/(nx-1),1.f/(ny-1),0);
     nVoxels[0] = (nx-1);
     nVoxels[1] = (ny-1);
     all_normals = new Normal[nx * ny];
     initNormals();
}


Heightfield2::~Heightfield2() {
    delete[] z;
}


BBox Heightfield2::ObjectBound() const {
    float minz = z[0], maxz = z[0];
    for (int i = 1; i < nx*ny; ++i) {
        if (z[i] < minz) minz = z[i];
        if (z[i] > maxz) maxz = z[i];
    }
    return BBox(Point(0,0,minz), Point(1,1,maxz));
}


bool Heightfield2::CanIntersect() const {
    return true;
}

bool Heightfield2::Intersect(const Ray &r, float *tHit, float *rayEpsilon,DifferentialGeometry *dg) const{
 //copy form grid.cpp
    // Check ray against overall grid bounds
    float rayT;
    
    Ray ray_o;
    (*WorldToObject)(r,&ray_o);//convert ray to object coordinate 
    
     //std::cout<<"ray_o.mint:"<<ray_o.mint<<std::endl;
    if (bounds.Inside(ray_o(ray_o.mint)))//
        rayT = ray_o.mint;
    else if (!bounds.IntersectP(ray_o, &rayT))
    {
        return false;
    }
    Point gridIntersect = ray_o(rayT);//position of 1st intersect of ray_o and bounds 

    // Set up 3D DDA for ray
    float NextCrossingT[2], DeltaT[2];
    int Step[2], Out[2], Pos[2];    
    float invWidth[2];
    float voxelToPos[2];

    for (int axis = 0; axis < 2; ++axis) {
        // Compute current voxel for axis //posToVoxel
        
        invWidth[axis] = (width[axis]==0.f) ? 0.f:1.f / width[axis];
        int v = Float2Int((gridIntersect[axis]-bounds.pMin[axis])*invWidth[axis]);
        Pos[axis] = Clamp(v,0,nVoxels[axis]-1);
        
        
        
        if (ray_o.d[axis] >= 0) {
            // Handle ray with positive direction for voxel stepping
            voxelToPos[axis] = bounds.pMin[axis] + (Pos[axis]+1)*width[axis];
            NextCrossingT[axis] = rayT +
                (voxelToPos[axis] - gridIntersect[axis]) / ray_o.d[axis];
            DeltaT[axis] = width[axis] / ray_o.d[axis];
            Step[axis] = 1;
            Out[axis] = nVoxels[axis];
 
        }
        else {
            // Handle ray with negative direction for voxel stepping
            voxelToPos[axis] = bounds.pMin[axis] + (Pos[axis])*width[axis];
            NextCrossingT[axis] = rayT +
                (voxelToPos[axis] - gridIntersect[axis]) / ray_o.d[axis];
            DeltaT[axis] = -width[axis] / ray_o.d[axis];
            Step[axis] = -1;
            Out[axis] = -1;

        }
    }

    Point* triangleUnit = new Point[3];
    // Walk ray through voxel grid
    for (;;) {
        // Check for intersection in current voxel and advance to next
        triangleUnit[0] = (*ObjectToWorld)(Point(Pos[0]*width[0],Pos[1]*width[1],z[Pos[0]+Pos[1]*nx]));
        triangleUnit[1] = (*ObjectToWorld)(Point((Pos[0]+1)*width[0],Pos[1]*width[1],z[Pos[0]+1+Pos[1]*nx]));
        triangleUnit[2] = (*ObjectToWorld)(Point(Pos[0]*width[0],(Pos[1]+1)*width[1],z[Pos[0]+(Pos[1]+1)*nx]));
        
        if (IntersectCk(triangleUnit,r,tHit,rayEpsilon,dg))
        	return true;

        triangleUnit[0] = (*ObjectToWorld)(Point((Pos[0]+1)*width[0],(Pos[1]+1)*width[1],z[Pos[0]+1+(Pos[1]+1)*nx]));

        if (IntersectCk(triangleUnit,r,tHit,rayEpsilon,dg))
        	return true;

        // Find _stepAxis_ for stepping to next voxel        
        int stepAxis = (NextCrossingT[1]<NextCrossingT[0])? 1:0;
        	
        if (ray_o.maxt < NextCrossingT[stepAxis])
            break;
        Pos[stepAxis] += Step[stepAxis];
        if (Pos[stepAxis] == Out[stepAxis])
            break;
        NextCrossingT[stepAxis] += DeltaT[stepAxis];
    }
   return false;
}

bool Heightfield2::IntersectCk(Point* triangleUnit, const Ray &ray, float *tHit, float *rayEpsilon,
                         DifferentialGeometry *dg) const {
   
    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point p1 = triangleUnit[0];
    const Point p2 = triangleUnit[1];
    const Point p3 = triangleUnit[2];

    Vector e1 = p2 - p1;
    Vector e2 = p3 - p1;
    Vector s1 = Cross(ray.d, e2);
    float divisor = Dot(s1, e1);
    
    if (divisor == 0.)
        return false;
    float invDivisor = 1.f / divisor;

    // Compute first barycentric coordinate
    Vector s = ray.o - p1;
    float b1 = Dot(s, s1) * invDivisor;
    if (b1 < 0. || b1 > 1.)
        return false;

    // Compute second barycentric coordinate
    Vector s2 = Cross(s, e1);
    float b2 = Dot(ray.d, s2) * invDivisor;
    if (b2 < 0. || b1 + b2 > 1.)
        return false;

    // Compute _t_ to intersection point
    float t = Dot(e2, s2) * invDivisor;
    if (t < ray.mint || t > ray.maxt)
        return false;

    // Compute triangle partial derivatives //not yet modified form here!!!!!!!!!!!!!!!!!!!!!!!
    Vector dpdu, dpdv;


    // Compute deltas for triangle partial derivatives
    float du1 = triangleUnit[0].x - triangleUnit[2].x;
    float du2 = triangleUnit[1].x - triangleUnit[2].x;
    float dv1 = triangleUnit[0].y - triangleUnit[2].y;
    float dv2 = triangleUnit[1].y - triangleUnit[2].y;
    Vector dp1 = p1 - p3, dp2 = p2 - p3;
    float determinant = du1 * dv2 - dv1 * du2;
    if (determinant == 0.f) {
        // Handle zero determinant for triangle partial derivative matrix
        CoordinateSystem(Normalize(Cross(e2, e1)), &dpdu, &dpdv);
    }
    else {
        float invdet = 1.f / determinant;
        dpdu = ( dv2 * dp1 - dv1 * dp2) * invdet;
        dpdv = (-du2 * dp1 + du1 * dp2) * invdet;
    }

    // Interpolate $(u,v)$ triangle parametric coordinates
    float b0 = 1 - b1 - b2;
    float tu = b0*triangleUnit[0].x + b1*triangleUnit[1].x + b2*triangleUnit[2].x;
    float tv = b0*triangleUnit[0].y + b1*triangleUnit[1].y + b2*triangleUnit[2].y;


    // Fill in _DifferentialGeometry_ from triangle hit
    *dg = DifferentialGeometry(ray(t), dpdu, dpdv,
                               Normal(0,0,0), Normal(0,0,0),
                               tu, tv, this);
    *tHit = t;
    *rayEpsilon = 1e-3f * *tHit;
    return true;
}


bool Heightfield2::IntersectP(const Ray &r) const{
  //copy form grid.cpp
    // Check ray against overall grid bounds
    float rayT;
    
    Ray ray_o;
    (*WorldToObject)(r,&ray_o);//convert ray to object coordinate 
    
    if (bounds.Inside(ray_o(ray_o.mint)))//
        rayT = ray_o.mint;
    else if (!bounds.IntersectP(ray_o, &rayT))
    {
        return false;
    }
    Point gridIntersect = ray_o(rayT);//position of 1st intersect of ray_o and bounds 

    // Set up 3D DDA for ray
    float NextCrossingT[2], DeltaT[2];
    int Step[2], Out[2], Pos[2];    
    float invWidth[2];
    float voxelToPos[2];

    for (int axis = 0; axis < 2; ++axis) {
        // Compute current voxel for axis //posToVoxel
        
        invWidth[axis] = (width[axis]==0.f) ? 0.f:1.f / width[axis];
        int v = Float2Int((gridIntersect[axis]-bounds.pMin[axis])*invWidth[axis]);
        Pos[axis] = Clamp(v,0,nVoxels[axis]-1);
        
        
        if (ray_o.d[axis] >= 0) {
            // Handle ray with positive direction for voxel stepping
            voxelToPos[axis] = bounds.pMin[axis] + (Pos[axis]+1)*width[axis];
            NextCrossingT[axis] = rayT +
                (voxelToPos[axis] - gridIntersect[axis]) / ray_o.d[axis];
            DeltaT[axis] = width[axis] / ray_o.d[axis];
            Step[axis] = 1;
            Out[axis] = nVoxels[axis];
        }
        else {
            // Handle ray with negative direction for voxel stepping
            voxelToPos[axis] = bounds.pMin[axis] + (Pos[axis])*width[axis];
            NextCrossingT[axis] = rayT +
                (voxelToPos[axis] - gridIntersect[axis]) / ray_o.d[axis];
            DeltaT[axis] = -width[axis] / ray_o.d[axis];
            Step[axis] = -1;
            Out[axis] = -1;
        }
    }

    Point* triangleUnit = new Point[3];
    // Walk ray through voxel grid
    for (;;) {
        // Check for intersection in current voxel and advance to next
        triangleUnit[0] = (*ObjectToWorld)(Point(Pos[0]*width[0],Pos[1]*width[1],z[Pos[0]+Pos[1]*nx]));
        triangleUnit[1] = (*ObjectToWorld)(Point((Pos[0]+1)*width[0],Pos[1]*width[1],z[Pos[0]+1+Pos[1]*nx]));
        triangleUnit[2] = (*ObjectToWorld)(Point(Pos[0]*width[0],(Pos[1]+1)*width[1],z[Pos[0]+(Pos[1]+1)*nx]));
        
        if (IntersectCkP(triangleUnit,r))
            return true;

        triangleUnit[0] = (*ObjectToWorld)(Point((Pos[0]+1)*width[0],(Pos[1]+1)*width[1],z[Pos[0]+1+(Pos[1]+1)*nx]));

        if (IntersectCkP(triangleUnit,r))
            return true;

        // Find _stepAxis_ for stepping to next voxel        
        int stepAxis = (NextCrossingT[1]<NextCrossingT[0])? 1:0;
            
        if (ray_o.maxt < NextCrossingT[stepAxis])
            break;
        Pos[stepAxis] += Step[stepAxis];
        if (Pos[stepAxis] == Out[stepAxis])
            break;
        NextCrossingT[stepAxis] += DeltaT[stepAxis];
    }
   return false;
}

bool Heightfield2::IntersectCkP(Point* triangleUnit,const Ray &ray) const {

    // Get triangle vertices in _p1_, _p2_, and _p3_
    const Point p1 = triangleUnit[0];
    const Point p2 = triangleUnit[1];
    const Point p3 = triangleUnit[2];

    Vector e1 = p2 - p1;
    Vector e2 = p3 - p1;
    Vector s1 = Cross(ray.d, e2);
    float divisor = Dot(s1, e1);
    
    if (divisor == 0.)
        return false;
    float invDivisor = 1.f / divisor;

    // Compute first barycentric coordinate
    Vector s = ray.o - p1;
    float b1 = Dot(s, s1) * invDivisor;
    if (b1 < 0. || b1 > 1.)
        return false;

    // Compute second barycentric coordinate
    Vector s2 = Cross(s, e1);
    float b2 = Dot(ray.d, s2) * invDivisor;
    if (b2 < 0. || b1 + b2 > 1.)
        return false;

    // Compute _t_ to intersection point
    float t = Dot(e2, s2) * invDivisor;
    if (t < ray.mint || t > ray.maxt)
        return false;

    return true;
}



void Heightfield2::Refine(vector<Reference<Shape> > &refined) const {
    int ntris = 2*(nx-1)*(ny-1);
    refined.reserve(ntris);
    int *verts = new int[3*ntris];
    Point *P = new Point[nx*ny];
    float *uvs = new float[2*nx*ny];
    int nverts = nx*ny;
    int x, y;
    // Compute heightfield vertex positions
    int pos = 0;
    for (y = 0; y < ny; ++y) {
        for (x = 0; x < nx; ++x) {
            P[pos].x = uvs[2*pos]   = (float)x / (float)(nx-1);
            P[pos].y = uvs[2*pos+1] = (float)y / (float)(ny-1);
            P[pos].z = z[pos];
            ++pos;
        }
    }

    // Fill in heightfield vertex offset array
    int *vp = verts;
    for (y = 0; y < ny-1; ++y) {
        for (x = 0; x < nx-1; ++x) {
#define VERT(x,y) ((x)+(y)*nx)
            *vp++ = VERT(x, y);
            *vp++ = VERT(x+1, y);
            *vp++ = VERT(x+1, y+1);
    
            *vp++ = VERT(x, y);
            *vp++ = VERT(x+1, y+1);
            *vp++ = VERT(x, y+1);
        }
#undef VERT
    }
    ParamSet paramSet;
    paramSet.AddInt("indices", verts, 3*ntris);
    paramSet.AddFloat("uv", uvs, 2 * nverts);
    paramSet.AddPoint("P", P, nverts);
    refined.push_back(CreateTriangleMeshShape(ObjectToWorld, WorldToObject, ReverseOrientation, paramSet));
    delete[] P;
    delete[] uvs;
    delete[] verts;
}


Heightfield2 *CreateHeightfield2Shape(const Transform *o2w, const Transform *w2o,
    bool reverseOrientation, const ParamSet &params) {
    int nu = params.FindOneInt("nu", -1);
    int nv = params.FindOneInt("nv", -1);
    int nitems;
    const float *Pz = params.FindFloat("Pz", &nitems);
    Assert(nitems == nu*nv);
    Assert(nu != -1 && nv != -1 && Pz != NULL);
    return new Heightfield2(o2w, w2o, reverseOrientation, nu, nv, Pz);
}

Normal Heightfield2::getNormal(int i, int j) const{
    
    bool textPlace[4] = {0,0,0,0}; // right up, right down, left down, left up

    if( i < (nx-1)){

        if( j < (ny-1))
            textPlace[0] = 1; //right up

        else if (j > 0)
            textPlace[1] = 1; //right down
    }

    if( i > 0){

        if( j <(ny-1))
            textPlace[3] = 1;//left up

        else if(j > 0)
            textPlace[2] =1;//left down
    }
    
    int countN =0; //count
    Normal avgNormal;
    for(int iter=0; iter<4; iter++){
        if(textPlace[iter]){
            avgNormal += getNormalUnit(iter,i,j); //not sure if one could do so or not 
            countN ++;
        }
    }

    return Normalize(Normal(avgNormal.x/countN, avgNormal.y/countN, avgNormal.z/countN));
}

Normal Heightfield2::getNormalUnit(int nu, int x, int y)const {
    
    Vector dx (1.f*width[0] ,0 ,0);
    Vector dy (0 ,1.f*width[1] ,0);
    switch(nu){

        case 0: //right up
            dx.z = z[(x+1) + y*nx] - z[x + y*nx];
            dy.z = z[x + (y+1)*nx] - z[x + y*nx];
            return Normalize(Normal(Cross(dx,dy)));
            break;

        case 1: //right down
            dx.z = z[(x+1) + y*nx] - z[x + y*nx];
            dy.z = z[x + (y-1)*nx] - z[x + y*nx];
            return Normalize(Normal(Cross(dy,dx)));
            break;

        case 2://left down
            dx.z = z[(x-1) + y*nx] - z[x + y*nx];
            dy.z = z[x + (y-1)*nx] - z[x + y*nx];
            return Normalize(Normal(Cross(dx,dy)));
            break;

        case 3://left up
            dx.z = z[(x-1) + y*nx] - z[x + y*nx];
            dy.z = z[x + (y+1)*nx] - z[x + y*nx];
            return Normalize(Normal(Cross(dy,dx)));
            break;

        default:
           break;
    }
}

void Heightfield2::initNormals() {
    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            all_normals[i + j*nx] = getNormal(i, j);
        }
    }
}

void Heightfield2::GetShadingGeometry(const Transform &obj2world,
        const DifferentialGeometry &dg,
        DifferentialGeometry *dgShading) const {

}
