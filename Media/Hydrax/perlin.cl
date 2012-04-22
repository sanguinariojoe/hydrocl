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

#ifndef n_packsize
	#define n_packsize 4
#endif
#ifndef n_bits
	#define n_bits 5
#endif
#ifndef n_size
	#define n_size (1<<(n_bits-1))
#endif
#ifndef n_size_sq
	#define n_size_sq (n_size*n_size)
#endif
#ifndef n_size_m1
	#define n_size_m1 (n_size - 1)
#endif
#ifndef n_dec_bits
	#define n_dec_bits 12
#endif
#ifndef n_dec_magn
	#define n_dec_magn 4096
#endif
#ifndef n_dec_magn_m1
	#define n_dec_magn_m1 4095
#endif
#ifndef np_bits
	#define np_bits (n_bits+n_packsize-1)
#endif
#ifndef np_size
	#define np_size (1<<(np_bits-1))
#endif
#ifndef np_size_m1
	#define np_size_m1 (np_size-1)
#endif
#ifndef np_size_sq
	#define np_size_sq (np_size*np_size)
#endif
#ifndef np_size_sq_m1
	#define np_size_sq_m1 (np_size_sq-1)
#endif
#ifndef noise_decimalbits
	#define noise_decimalbits 15
#endif
#ifndef noise_magnitude
	#define noise_magnitude (1<<(noise_decimalbits-1))
#endif


/** Reads a noise texel.
 * @param uv UV coordinates.
 * @param noise Perlin noise.
 * @return Height value.
 */
int readTexelLinearDual(int2 uv, _g int* noise){
	int iu, iup, iv, ivp, fu, fv,
		ut01, ut23;

	iu = (uv.x>>n_dec_bits)&np_size_m1;
	iv = ((uv.y>>n_dec_bits)&np_size_m1)*np_size;

	iup = ((uv.x>>n_dec_bits) + 1)&np_size_m1;
	ivp = (((uv.y>>n_dec_bits) + 1)&np_size_m1)*np_size;

	fu = uv.x & n_dec_magn_m1;
	fv = uv.y & n_dec_magn_m1;

	ut01 = ((n_dec_magn-fu)*noise[iv + iu] + fu*noise[iv + iup])>>n_dec_bits;
	ut23 = ((n_dec_magn-fu)*noise[ivp + iu] + fu*noise[ivp + iup])>>n_dec_bits;

	return ((n_dec_magn-fv)*ut01 + fv*ut23) >> n_dec_bits;

}

/** Compute vertex height.
 * @param vertex Geometry vertexes.
 * @param noise Perlin noise.
 * @param world Rendering camera position.
 * @param magnitude Perlin octaves allocator.
 * @param N Total number of vertices at each direction.
 */
__kernel void height( _g vec* vertex, _g int* noise, vec world, float magnitude, uint octaves, uint2 N )
{
	uint i  = get_global_id(0);
	uint j  = get_global_id(1);
	if( (i >= N.x) || (j >= N.y) )
		return;
	uint id = j*N.x + i;

	// ---- | ------------------------ | ----
	// ---- V ---- Your code here ---- V ----

	_g int* r_noise = noise;
	float2 uv  = world.xz + vertex[id].xz;
	int2   uvi = (int2)((int)(uv.x*magnitude), (int)(uv.y*magnitude));
	uint   o, hoct = octaves / n_packsize;
	float value=0.f;
	for(o=0;o<hoct;o++){
		value += (float)readTexelLinearDual(uvi, r_noise);
		uvi.x = uvi.x << n_packsize;
		uvi.y = uvi.y << n_packsize;
		r_noise += np_size_sq;
	}

	vertex[id].y += 3.5f*value/noise_magnitude;

	// ---- A ---- Your code here ---- A ----
	// ---- | ------------------------ | ----

}

