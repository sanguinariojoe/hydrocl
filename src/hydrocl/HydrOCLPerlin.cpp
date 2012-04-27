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

--------------------------------------------------------------------------------
Based on the perlin noise code from Claes Johanson thesis:
http://graphics.cs.lth.se/theses/projects/projgrid/
--------------------------------------------------------------------------------
 */

#include <hydrocl/HydrOCLPerlin.h>
#include <hydrocl/HydrOCLUtils.h>

#include <Hydrax/Hydrax.h>

#define _def_PackedNoise true

namespace Hydrax{namespace Noise
{
	HydrOCLPerlin::HydrOCLPerlin()
		: Noise("HydrOCLNoise", false)
		, time(0)
		, r_noise(0)
		, magnitude(n_dec_magn * 0.085f)
		, mNumberOfDevices(0)
		, mDevices(NULL)
		, mContext(0)
		, mComQueue(NULL)
		, clNoise(0)
		, kHeight(0)
	{
	}

	HydrOCLPerlin::HydrOCLPerlin(const Options &Options)
		: Noise("HydrOCLNoise", false)
		, mOptions(Options)
		, time(0)
		, r_noise(0)
		, magnitude(n_dec_magn * Options.Scale)
		, mNumberOfDevices(0)
		, mDevices(NULL)
		, mContext(0)
		, mComQueue(NULL)
		, clNoise(0)
		, kHeight(0)
	{
	}

	HydrOCLPerlin::~HydrOCLPerlin()
	{
		remove();

		HydraxLOG(getName() + " destroyed.");
	}

	void HydrOCLPerlin::create()
	{
		if (isCreated()) {
			return;
		}

		Noise::create();
		_initNoise();
	}

	void HydrOCLPerlin::remove()
	{
		if (!isCreated()) {
			return;
		}

		time = 0;

		Noise::remove();

		mNumberOfDevices = 0;
		mDevices = NULL;
		mContext = 0;
		mComQueue = NULL;
        if(kHeight)clReleaseKernel(kHeight); kHeight=0;
        if(clNoise)clReleaseMemObject(clNoise); clNoise=0;
	}

	void HydrOCLPerlin::setOptions(const Options &Options)
	{
		if (isCreated()) {
			int Octaves_ = Options.Octaves;
			mOptions = Options;
			mOptions.Octaves = Octaves_;
		}
		else {
			mOptions = Options;
			mOptions.Octaves = (mOptions.Octaves<max_octaves) ? mOptions.Octaves : max_octaves;
		}

		magnitude = n_dec_magn * mOptions.Scale;
	}

	void HydrOCLPerlin::saveCfg(Ogre::String &Data)
	{
		Noise::saveCfg(Data);

		Data += CfgFileManager::_getCfgString("Perlin_Octaves", mOptions.Octaves);
		Data += CfgFileManager::_getCfgString("Perlin_Scale", mOptions.Scale);
		Data += CfgFileManager::_getCfgString("Perlin_Falloff", mOptions.Falloff);
		Data += CfgFileManager::_getCfgString("Perlin_Animspeed", mOptions.Animspeed);
		Data += CfgFileManager::_getCfgString("Perlin_Timemulti", mOptions.Timemulti);
		Data += CfgFileManager::_getCfgString("Perlin_Strength", mOptions.GPU_Strength); Data += "\n";
	}

	bool HydrOCLPerlin::loadCfg(Ogre::ConfigFile &CfgFile)
	{
		if (!Noise::loadCfg(CfgFile)) {
			return false;
		}

		setOptions(
			Options(CfgFileManager::_getIntValue(CfgFile,"Perlin_Octaves"),
			        CfgFileManager::_getFloatValue(CfgFile,"Perlin_Scale"),
					CfgFileManager::_getFloatValue(CfgFile,"Perlin_Falloff"),
					CfgFileManager::_getFloatValue(CfgFile,"Perlin_Animspeed"),
					CfgFileManager::_getFloatValue(CfgFile,"Perlin_Timemulti"),
					CfgFileManager::_getFloatValue(CfgFile,"Perlin_Strength"),
					Ogre::Vector3::ZERO));
		return true;
	}

	void HydrOCLPerlin::update(const Ogre::Real &timeSinceLastFrame)
	{
		time += timeSinceLastFrame*mOptions.Animspeed;
		_calculeNoise();
	}

	float HydrOCLPerlin::getValue(const float &x, const float &y)
	{
		return _getHeigthDual(x,y);
	}

    bool HydrOCLPerlin::setHeight(cl_mem v, cl_uint2 N, Ogre::Vector3 world)
    {
        cl_int clFlag=0;
        //! @todo allow several devices usage
        size_t localWorkSize[2], globalWorkSize[2];
        localWorkSize[0] = 256;
        localWorkSize[1] = 256;
        globalWorkSize[0] = roundUp(N.x, localWorkSize[0]);
        globalWorkSize[1] = roundUp(N.y, localWorkSize[1]);
        // Launch kernel
        cl_uint octaves = (unsigned int)mOptions.Octaves;
        cl_float4 w;
        w.x=world.x; w.y=world.y; w.z=world.z; w.w=0.f;
        float strength = mOptions.GPU_Strength;
        clFlag |= sendData(mComQueue[0], clNoise, p_noise, np_size_sq*(max_octaves>>(n_packsize-1))*sizeof( cl_int ));
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Can't send noise to perlin computation.");
            return false;
        }
        clFlag |= sendArgument(kHeight,  0, sizeof(cl_mem   ), (void*)&v);
        clFlag |= sendArgument(kHeight,  1, sizeof(cl_mem   ), (void*)&clNoise);
        clFlag |= sendArgument(kHeight,  2, sizeof(cl_float4), (void*)&world);
        clFlag |= sendArgument(kHeight,  3, sizeof(cl_float ), (void*)&strength);
        clFlag |= sendArgument(kHeight,  4, sizeof(cl_float ), (void*)&magnitude);
        clFlag |= sendArgument(kHeight,  5, sizeof(cl_uint  ), (void*)&octaves);
        clFlag |= sendArgument(kHeight,  6, sizeof(cl_uint2 ), (void*)&N);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Can't send arguments to perlin computation.");
            return false;
        }
        clFlag = clEnqueueNDRangeKernel(mComQueue[0], kHeight, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Perlin vertexes modifier execution fail.");
            return false;
        }
        return true;
    }

	void HydrOCLPerlin::_initNoise()
	{
		// Create noise (uniform)
		float tempnoise[n_size_sq*noise_frames], temp;

		int i, frame, v, u,
            v0, v1, v2, u0, u1, u2, f;

		for(i=0; i<(n_size_sq*noise_frames); i++) {
			temp = static_cast<float>(rand())/RAND_MAX;
			tempnoise[i] = 4*(temp - 0.5f);
		}

		for(frame=0; frame<noise_frames; frame++) {
			for(v=0; v<n_size; v++) {
				for(u=0; u<n_size; u++) {
					v0 = ((v-1)&n_size_m1)*n_size;
					v1 = v*n_size;
					v2 = ((v+1)&n_size_m1)*n_size;
					u0 = ((u-1)&n_size_m1);
					u1 = u;
					u2 = ((u+1)&n_size_m1);
					f  = frame*n_size_sq;

					temp = (1.0f/14.0f) *
					   (tempnoise[f + v0 + u0] +      tempnoise[f + v0 + u1] + tempnoise[f + v0 + u2] +
						tempnoise[f + v1 + u0] + 6.0f*tempnoise[f + v1 + u1] + tempnoise[f + v1 + u2] +
						tempnoise[f + v2 + u0] +      tempnoise[f + v2 + u1] + tempnoise[f + v2 + u2]);

					noise[frame*n_size_sq + v*n_size + u] = noise_magnitude*temp;
				}
			}
		}
	}

	void HydrOCLPerlin::_calculeNoise()
	{
		int i, o, v, u,
			multitable[max_octaves],
			amount[3],
			iImage;

		unsigned int image[3];

		float sum = 0.0f,
			  f_multitable[max_octaves];

		double dImage, fraction;

		// calculate the strength of each octave
		for(i=0; i<mOptions.Octaves; i++) {
			f_multitable[i] = powf(mOptions.Falloff,1.0f*i);
			sum += f_multitable[i];
		}

		for(i=0; i<mOptions.Octaves; i++) {
			f_multitable[i] /= sum;
		}

		for(i=0; i<mOptions.Octaves; i++) {
			multitable[i] = scale_magnitude*f_multitable[i];
		}

		double r_timemulti = 1.0;
		const float PI_3 = Ogre::Math::PI/3;

		for(o=0; o<mOptions.Octaves; o++) {
			fraction = modf(time*r_timemulti,&dImage);
			iImage = static_cast<int>(dImage);

			amount[0] = scale_magnitude*f_multitable[o]*(pow(sin((fraction+2)*PI_3),2)/1.5);
			amount[1] = scale_magnitude*f_multitable[o]*(pow(sin((fraction+1)*PI_3),2)/1.5);
			amount[2] = scale_magnitude*f_multitable[o]*(pow(sin((fraction  )*PI_3),2)/1.5);

			image[0] = (iImage  ) & noise_frames_m1;
			image[1] = (iImage+1) & noise_frames_m1;
			image[2] = (iImage+2) & noise_frames_m1;

			for (i=0; i<n_size_sq; i++) {
			    o_noise[i + n_size_sq*o] = (
				   ((amount[0] * noise[i + n_size_sq * image[0]])>>scale_decimalbits) +
				   ((amount[1] * noise[i + n_size_sq * image[1]])>>scale_decimalbits) +
				   ((amount[2] * noise[i + n_size_sq * image[2]])>>scale_decimalbits));
			}

			r_timemulti *= mOptions.Timemulti;
		}

		if(_def_PackedNoise) {
			int octavepack = 0;
			for(o=0; o<mOptions.Octaves; o+=n_packsize) {
				for(v=0; v<np_size; v++) {
					for(u=0; u<np_size; u++) {
						p_noise[v*np_size+u+octavepack*np_size_sq]  = o_noise[(o+3)*n_size_sq + (v&n_size_m1)*n_size + (u&n_size_m1)];
						p_noise[v*np_size+u+octavepack*np_size_sq] += _mapSample( u, v, 3, o);
						p_noise[v*np_size+u+octavepack*np_size_sq] += _mapSample( u, v, 2, o+1);
						p_noise[v*np_size+u+octavepack*np_size_sq] += _mapSample( u, v, 1, o+2);
					}
				}

				octavepack++;
			}
		}
	}

	int HydrOCLPerlin::_readTexelLinearDual(const int &u, const int &v,const int &o)
	{
		int iu, iup, iv, ivp, fu, fv,
			ut01, ut23, ut;

		iu = (u>>n_dec_bits)&np_size_m1;
		iv = ((v>>n_dec_bits)&np_size_m1)*np_size;

		iup = ((u>>n_dec_bits) + 1)&np_size_m1;
		ivp = (((v>>n_dec_bits) + 1)&np_size_m1)*np_size;

		fu = u & n_dec_magn_m1;
		fv = v & n_dec_magn_m1;

		ut01 = ((n_dec_magn-fu)*r_noise[iv + iu] + fu*r_noise[iv + iup])>>n_dec_bits;
		ut23 = ((n_dec_magn-fu)*r_noise[ivp + iu] + fu*r_noise[ivp + iup])>>n_dec_bits;
		ut = ((n_dec_magn-fv)*ut01 + fv*ut23) >> n_dec_bits;

		return ut;
	}

	float HydrOCLPerlin::_getHeigthDual(float u, float v)
	{
		// Pointer to the current noise source octave
		r_noise = p_noise;

		int ui = u*magnitude,
		    vi = v*magnitude,
			i,
			value = 0,
			hoct = mOptions.Octaves / n_packsize;

		for(i=0; i<hoct; i++) {
			value += _readTexelLinearDual(ui,vi,0);
			ui = ui << n_packsize;
			vi = vi << n_packsize;
			r_noise += np_size_sq;
		}

		return static_cast<float>(value)/noise_magnitude;
	}

	int HydrOCLPerlin::_mapSample(const int &u, const int &v, const int &upsamplepower, const int &octave)
	{
		int magnitude = 1<<upsamplepower,

		    pu = u >> upsamplepower,
		    pv = v >> upsamplepower,

		    fu = u & (magnitude-1),
		    fv = v & (magnitude-1),

		    fu_m = magnitude - fu,
		    fv_m = magnitude - fv,

		    o = fu_m*fv_m*o_noise[octave*n_size_sq + ((pv)  &n_size_m1)*n_size + ((pu)  &n_size_m1)] +
			    fu*  fv_m*o_noise[octave*n_size_sq + ((pv)  &n_size_m1)*n_size + ((pu+1)&n_size_m1)] +
			    fu_m*fv*  o_noise[octave*n_size_sq + ((pv+1)&n_size_m1)*n_size + ((pu)  &n_size_m1)] +
			    fu*  fv*  o_noise[octave*n_size_sq + ((pv+1)&n_size_m1)*n_size + ((pu+1)&n_size_m1)];

		return o >> (upsamplepower+upsamplepower);
	}

	bool HydrOCLPerlin::setupOpenCL(cl_uint n, cl_context context, cl_device_id *devices, cl_command_queue *comQueue)
	{
        cl_int clFlag=0;
	    // Store data
        mNumberOfDevices = n;
        mDevices         = devices;
        mContext         = context;
        mComQueue        = comQueue;
        // Create memory objects
        size_t size = np_size_sq*(max_octaves>>(n_packsize-1))*sizeof(int);
        clNoise = clCreateBuffer(mContext, CL_MEM_READ_WRITE, size, NULL, &clFlag);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("\t\tPerlin noise memory allocation fail.");
            return false;
        }
        // Load kernels
        //! @todo allow several devices usage
        const char* path = fileFromResources("perlin.cl");
        if(!path){
            HydraxLOG("\tPerlin OpenCL program can't be found!");
            return false;
        }
        char* flags = new char[1024];
        sprintf(flags, "-Dn_packsize=%u -Dn_bits=%u -Dn_dec_bits=%u -Dn_dec_magn=%u -Dn_dec_magn_m1=%u -Dnoise_decimalbits=%u",
                n_packsize, n_bits, n_dec_bits, n_dec_magn, n_dec_magn_m1, noise_decimalbits);
        kHeight = loadKernelFromFile(mContext, mDevices[0], path, "height", flags);
        delete[] flags; flags=0;
        if( !kHeight ){
            return false;
        }
        return true;
	}
}}
