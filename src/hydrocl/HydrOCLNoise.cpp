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

#include <hydrocl/HydrOCLNoise.h>
#include <hydrocl/HydrOCLUtils.h>

#include <Hydrax/Hydrax.h>

#define _def_PackedNoise true

namespace Hydrax{namespace Noise
{
	HydrOCLNoise::HydrOCLNoise()
		: HydrOCLPerlin()
		, mTime(0.f)
		, mDir(0)
		, mA(0)
		, mT(0)
		, mP(0)
	{
	}

	HydrOCLNoise::HydrOCLNoise(const HydrOCLPerlin::Options &Options)
		: HydrOCLPerlin(Options)
		, mTime(0.f)
		, mDir(0)
		, mA(0)
		, mT(0)
		, mP(0)
	{
	}

	HydrOCLNoise::~HydrOCLNoise()
	{
	}

	void HydrOCLNoise::create()
	{
		if (isCreated()) {
			return;
		}

		HydrOCLPerlin::create();
	}

	void HydrOCLNoise::remove()
	{
	    unsigned int i;
	    for(i=0;i<mWaves.size();i++){
            delete mWaves.at(i);
	    }
	    mWaves.clear();
        if(kWaves)clReleaseKernel(kWaves); kWaves=0;
        if(mDir)clReleaseMemObject(mDir); mDir=0;
        if(mA)clReleaseMemObject(mA); mA=0;
        if(mT)clReleaseMemObject(mT); mT=0;
        if(mP)clReleaseMemObject(mP); mP=0;
        if(hDir) delete[] hDir; hDir=0;
        if(hA)   delete[] hA; hA=0;
        if(hT)   delete[] hT; hT=0;
        if(hP)   delete[] hP; hP=0;

		if (!isCreated()) {
			return;
		}

		HydrOCLPerlin::remove();
	}

	void HydrOCLNoise::update(const Ogre::Real &timeSinceLastFrame)
	{
		HydrOCLPerlin::update(timeSinceLastFrame);
		mTime += timeSinceLastFrame;
	}

    void HydrOCLNoise::wave(const HydrOCLNoise::Wave &w)
    {
        mWaves.push_back(new Wave(w.dir, w.A, w.T, w.P));
        if(!reallocate()){
            remove();
            return;
        }
        if(!send()){
            remove();
            return;
        }
    }

    HydrOCLNoise::Wave* HydrOCLNoise::wave(unsigned int id)
    {
        if(id >= mWaves.size())
            return NULL;
        return mWaves.at(id);
    }

    bool HydrOCLNoise::removeWave(unsigned int id)
    {
        if(id >= mWaves.size())
            return false;
        delete mWaves.at(id);
        mWaves.erase(mWaves.begin() + id);
        if(!reallocate()){
            remove();
            return false;
        }
        if(!send()){
            remove();
            return false;
        }
        return true;
    }

	float HydrOCLNoise::getValue(const float &x, const float &y)
	{
	    unsigned int i;
	    float value = HydrOCLPerlin::getValue(x,y);
	    for(i=0;i<mWaves.size();i++){
	        Wave* w = mWaves.at(i);
            float X = w->dir.x*x + w->dir.y*y;
            float L = 1.5625f*w->T*w->T;
            float F = 2.f*M_PI/w->T;
            float K = 2.f*M_PI/L;
            value += w->A * sin(F*mTime - K*X + w->P);
	    }
		return value;
	}

    bool HydrOCLNoise::setHeight(cl_mem v, cl_uint2 N, Ogre::Vector3 world)
    {
        if(!HydrOCLPerlin::setHeight(v, N, world))
            return false;
        if(!mWaves.size())
            return true;
        cl_int clFlag=0;
        //! @todo allow several devices usage
        size_t localWorkSize[2], globalWorkSize[2];
        localWorkSize[0] = 256;
        localWorkSize[1] = 256;
        globalWorkSize[0] = roundUp(N.x, localWorkSize[0]);
        globalWorkSize[1] = roundUp(N.y, localWorkSize[1]);
        // Launch kernel
        if(isModified()){
            if(!send())
                return false;
        }
        cl_uint nWaves = (unsigned int)mWaves.size();
        clFlag |= sendArgument(kWaves,  0, sizeof(cl_mem   ), (void*)&v);
        clFlag |= sendArgument(kWaves,  1, sizeof(cl_mem   ), (void*)&mDir);
        clFlag |= sendArgument(kWaves,  2, sizeof(cl_mem   ), (void*)&mA);
        clFlag |= sendArgument(kWaves,  3, sizeof(cl_mem   ), (void*)&mT);
        clFlag |= sendArgument(kWaves,  4, sizeof(cl_mem   ), (void*)&mP);
        clFlag |= sendArgument(kWaves,  5, sizeof(cl_float4), (void*)&world);
        clFlag |= sendArgument(kWaves,  6, sizeof(cl_float ), (void*)&mTime);
        clFlag |= sendArgument(kWaves,  7, sizeof(cl_uint  ), (void*)&nWaves);
        clFlag |= sendArgument(kWaves,  8, sizeof(cl_uint2 ), (void*)&N);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Can't send arguments to waves computation.");
            return false;
        }
        clFlag = clEnqueueNDRangeKernel(mComQueue[0], kWaves, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Waves computation execution fail.");
            return false;
        }
        return true;
    }

	bool HydrOCLNoise::setupOpenCL(cl_uint n, cl_context context, cl_device_id *devices, cl_command_queue *comQueue)
	{
        if(!HydrOCLPerlin::setupOpenCL(n, context, devices, comQueue))
            return false;
        // Load kernel
        //! @todo allow several devices usage
        const char* path = fileFromResources("waves.cl");
        if(!path){
            HydraxLOG("\tPerlin OpenCL program can't be found!");
            return false;
        }
        kWaves = loadKernelFromFile(mContext, mDevices[0], path, "height", "");
        if( !kWaves ){
            return false;
        }
        return true;
	}

	bool HydrOCLNoise::reallocate()
	{
        cl_int clFlag=0;
        if(mDir)clReleaseMemObject(mDir); mDir=0;
        if(mA)clReleaseMemObject(mA); mA=0;
        if(mT)clReleaseMemObject(mT); mT=0;
        if(mP)clReleaseMemObject(mP); mP=0;
        if(hDir) delete[] hDir; hDir=0;
        if(hA)   delete[] hA; hA=0;
        if(hT)   delete[] hT; hT=0;
        if(hP)   delete[] hP; hP=0;
        unsigned int N = mWaves.size();
        if(!N)
            return true;
        mDir = clCreateBuffer(mContext, CL_MEM_READ_WRITE, N*sizeof(cl_float2), NULL, &clFlag);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("\t\tWaves directions allocation failure.");
            mDir = 0;
            return false;
        }
        mA = clCreateBuffer(mContext, CL_MEM_READ_WRITE, N*sizeof(cl_float), NULL, &clFlag);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("\t\tWaves amplitudes allocation failure.");
            mA = 0;
            return false;
        }
        mT = clCreateBuffer(mContext, CL_MEM_READ_WRITE, N*sizeof(cl_float), NULL, &clFlag);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("\t\tWaves periods allocation failure.");
            mT = 0;
            return false;
        }
        mP = clCreateBuffer(mContext, CL_MEM_READ_WRITE, N*sizeof(cl_float), NULL, &clFlag);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("\t\tWaves phases allocation failure.");
            mP = 0;
            return false;
        }
	    hDir = new cl_float2[N];
	    hA   = new cl_float[N];
	    hT   = new cl_float[N];
	    hP   = new cl_float[N];
        return true;
	}

	bool HydrOCLNoise::send()
	{
        cl_int clFlag=0;
        unsigned int i, N = mWaves.size();
        if(!N)
            return true;
	    for(i=0;i<N;i++){
            hDir[i].x = mWaves.at(i)->dir.x;
            hDir[i].y = mWaves.at(i)->dir.y;
            hA[i]     = mWaves.at(i)->A;
            hT[i]     = mWaves.at(i)->T;
            hP[i]     = mWaves.at(i)->P;
	    }
        clFlag |= sendData(mComQueue[0], mDir, hDir, N*sizeof( cl_float2 ));
        clFlag |= sendData(mComQueue[0], mA,   hA,   N*sizeof( cl_float  ));
        clFlag |= sendData(mComQueue[0], mT,   hT,   N*sizeof( cl_float  ));
        clFlag |= sendData(mComQueue[0], mP,   hP,   N*sizeof( cl_float  ));
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Can't send waves data to device.");
            return false;
        }
        return true;
	}

	bool HydrOCLNoise::isModified()
	{
        unsigned int i, N = mWaves.size();
	    for(i=0;i<N;i++){
            bool modified = false;
	        modified |= hDir[i].x != mWaves.at(i)->dir.x;
	        modified |= hDir[i].y != mWaves.at(i)->dir.y;
	        modified |= hA[i]     != mWaves.at(i)->A;
	        modified |= hT[i]     != mWaves.at(i)->T;
	        modified |= hP[i]     != mWaves.at(i)->P;
            if(modified)
                return true;
	    }
        return false;
	}
}}
