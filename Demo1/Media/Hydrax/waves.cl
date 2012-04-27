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

/** Compute vertex height due to waves.
 * @param vertex Geometry vertexes.
 * @param wDir Waves direction.
 * @param wA Waves amplitude [m].
 * @param wT Waves period [s].
 * @param wP Waves phase [rad].
 * @param N Total number of vertices at each direction.
 */
__kernel void height( _g vec* vertex, _g float2* wDir, _g float* wA, _g float* wT, _g float* wP, vec world, float time, uint n, uint2 N )
{
	uint i  = get_global_id(0);
	uint j  = get_global_id(1);
	if( (i >= N.x) || (j >= N.y) )
		return;
	uint id = j*N.x + i;

	// ---- | ------------------------ | ----
	// ---- V ---- Your code here ---- V ----

	float2 uv = world.xz + vertex[id].xz;
	float R, L, F, K;
	uint k;
	for(k=0;k<n;k++){
		R = dot(wDir[k], uv);
		L = 1.5625f*wT[k]*wT[k];
		vertex[id].y += wA[k]*sin( 2.f*M_PI*( time/wT[k] - R/L ) + wP[k] );
	}

	// ---- A ---- Your code here ---- A ----
	// ---- | ------------------------ | ----

}

