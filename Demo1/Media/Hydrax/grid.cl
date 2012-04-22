/*
 * Copyright (C) 2012  Jose Luis Cercos Pita (jlcercos@gmail.com)
 *
 * This source file is part of SonSilentSea.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifndef uint
	#define uint unsigned int
#endif
#ifndef vec
	#define vec float4
#endif

#ifdef _g
	#error '_g' is already defined.
#endif
#define _g __global

#ifdef _c
	#error '_c' is already defined.
#endif
#define _c __constant

#ifdef _l
	#error '_l' is already defined.
#endif
#define _l __local

/** Copy vertexes from in to out. Useful for choppy vertexes computation.
 * @param outV Output vertexes.
 * @param outN Output normals.
 * @param inV Input vertexes.
 * @param inN Input normals.
 * @param N Total number of vertices at each direction.
 */
__kernel void copy( _g vec* outV, _g vec* outN, _g vec* inV, _g vec* inN, uint2 N )
{
	uint i  = get_global_id(0);
	uint j  = get_global_id(1);
	if( (i >= N.x) || (j >= N.y) )
		return;
	uint id = j*N.x + i;

	outV[id] = inV[id];
	outN[id] = inN[id];
}

/** Performs a simple vertex smoothing operation.
 * @param vertexes Vertexes to smooth.
 * @param N Total number of vertices at each direction.
 */
__kernel void smooth( _g vec* vertexes, uint2 N )
{
	uint i  = get_global_id(0);
	uint j  = get_global_id(1);
	if( (i < 1) || (j < 1) )
		return;
	if( (i >= N.x-1) || (j >= N.y-1) )
		return;
	uint id = j*N.x + i;

	vertexes[id].y = 0.2f*(
                     vertexes[id].y +
                     vertexes[id-1].y +
                     vertexes[id+1].y +
                     vertexes[id-N.x].y +
                     vertexes[id+N.x].y);
}

/** Normals computation.
 * @param vertex Geometry vertexes.
 * @param normal Resultant normals.
 * @param N Total number of vertices at each direction.
 */
__kernel void normals( _g vec* vertex, _g vec* normal, uint2 N )
{
	uint i  = get_global_id(0);
	uint j  = get_global_id(1);
	if( (i >= N.x) || (j >= N.y) )
		return;
	uint id = j*N.x + i;

	// Set boundaries with plane normal
	if( (i < 1) || (j < 1) || (i >= N.x-1) || (j >= N.y-1) ){
		normal[id] = (vec)(0.f, -1.f, 0.f, 0.f);
		return;
	}
	// Interpolate the rest of vertexes
	vec vec1, vec2;
	vec1       = vertex[id-1]   - vertex[id+1];
	vec2       = vertex[id-N.x] - vertex[id+N.x];
	normal[id] = normalize(cross(vec2, vec1));
}

/** Choppy waves computation.
 * @param vertex Geometry vertexes.
 * @param choppy Choppy waves geometry vertexes buffer.
 * @param normal Geometry vertexes normal.
 * @param camDir Camera direction.
 * @param strength Choppy waves strength.
 * @param underwater -1.f if frame is being rendered underwater, 1 otherwise.
 * @param N Total number of vertices at each direction.
 */
__kernel void choppyWaves( _g vec* vertex, _g vec* choppy, _g vec* normal, vec camDir, float strength, float underwater, uint2 N )
{
	uint i  = get_global_id(0);
	uint j  = get_global_id(1);
	if( (i < 1) || (j < 1) || (i >= N.x-1) || (j >= N.y-1) )
		return;
	uint id = j*N.x + i;

	float Dis1, Dis2;
	float2 Dir, Perp, Norm2;
	// Get directions
	Dir  = fabs(normalize(camDir.xz));
	Perp = (float2)(-Dir.y, Dir.x);
	// Get distances
	Dis1  = distance(choppy[id].xz, choppy[id+N.x].xz);
	Dis2  = distance(choppy[id].xz, choppy[id+1].xz);
	Norm2 = normal[id].xz * (Dir*Dis1 + Perp*Dis2) * strength;
	// Final result
	vertex[id].xz = choppy[id].xz + underwater*Norm2;
}

/** Fully geometry regeneration when camera has been moved.
 * @param vertexes Output vertexes.
 * @param corner0 1st grid bounds corner.
 * @param corner1 2nd grid bounds corner.
 * @param corner2 3rd grid bounds corner.
 * @param corner3 4th grid bounds corner.
 * @param N Total number of vertices at each direction.
 */
__kernel void geometry( _g vec* vertexes, vec corner0, vec corner1, vec corner2, vec corner3, uint2 N )
{
	uint i  = get_global_id(0);
	uint j  = get_global_id(1);
	if( (i >= N.x) || (j >= N.y) )
		return;
	uint id = j*N.x + i;

	// ---- | ------------------------ | ----
	// ---- V ---- Your code here ---- V ----

	float2 uv, uvDi;
	vec result;
	float divide;
	// Get uv coordinates
	uv.x = i/(float)N.x;
	uv.y = j/(float)N.y;
	uvDi = (float2)(1.f, 1.f) - uv;
	// Get base plane coordinates
    result.x = uvDi.y*(uvDi.x*corner0.x + uv.x*corner1.x) + uv.y*(uvDi.x*corner2.x + uv.x*corner3.x);
    result.z = uvDi.y*(uvDi.x*corner0.z + uv.x*corner1.z) + uv.y*(uvDi.x*corner2.z + uv.x*corner3.z);
    result.w = uvDi.y*(uvDi.x*corner0.w + uv.x*corner1.w) + uv.y*(uvDi.x*corner2.w + uv.x*corner3.w);

    divide = 1.f/result.w;
    result.x *= divide;
    result.z *= divide;

	// Set vertexes, but delegating all heigh operation to following kernels
    vertexes[id].x = result.x;
    vertexes[id].z = result.z;
    vertexes[id].y = 0.f;
	vertexes[id].w = 1.f;

	// ---- A ---- Your code here ---- A ----
	// ---- | ------------------------ | ----

}

/** Sets base plane coordinate to all vertexes.
 * @param vertexes Output vertexes.
 * @param h Base plane y coordinate.
 * @param N Total number of vertices at each direction.
 * @warning Y coordinate set will be -h, not h.
 */
__kernel void setBasePlane( _g vec* vertexes, float h, uint2 N )
{
	uint i  = get_global_id(0);
	uint j  = get_global_id(1);
	if( (i >= N.x) || (j >= N.y) )
		return;
	uint id = j*N.x + i;

	// ---- | ------------------------ | ----
	// ---- V ---- Your code here ---- V ----

    vertexes[id].y = -h;

	// ---- A ---- Your code here ---- A ----
	// ---- | ------------------------ | ----

}
