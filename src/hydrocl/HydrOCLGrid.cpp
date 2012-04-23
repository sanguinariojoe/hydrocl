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
Based on the Projected Grid concept from Claes Johanson thesis:
http://graphics.cs.lth.se/theses/projects/projgrid/
and Ren Cheng Ogre3D implementation:
http://www.cnblogs.com/ArenAK/archive/2007/11/07/951713.html
--------------------------------------------------------------------------------
*/

#include <hydrocl/HydrOCLGrid.h>
#include <hydrocl/HydrOCLUtils.h>
#include <hydrocl/HydrOCLPerlin.h>

#ifndef _def_MaxFarClipDistance
    #define _def_MaxFarClipDistance 99999
#endif

namespace Hydrax{namespace Module
{
	HydrOCL::HydrOCL(Hydrax *h, const Ogre::Plane &BasePlane)
		: Module("HydrOCL", new Noise::HydrOCLPerlin(), Mesh::Options(256, Size(0), Mesh::VT_POS_NORM), MaterialManager::NM_VERTEX)
		, mHydrax(h)
		, mVertices(0)
		, mVerticesChoppyBuffer(0)
		, mBasePlane(BasePlane)
		, mNormal(BasePlane.normal)
		, mPos(Ogre::Vector3(0,0,0))
		, mProjectingCamera(0)
		, mTmpRndrngCamera(0)
		, mRenderingCamera(h->getCamera())
        , mNumberOfPlatforms(0)
        , mPlatforms(NULL)
        , mPlatform(0)
        , mNumberOfDevices(0)
        , mDevices(NULL)
        , mContext(0)
        , mComQueue(NULL)
        , mAllocatedMem(0)
        , mVertexes(0)
        , mNormals(0)
        , mChoppyVertexes(0)
        , mChoppyNormals(0)
        , kGeometryGen(0)
        , kBasePlane(0)
        , kCopy(0)
        , kSmooth(0)
        , kNormals(0)
        , kChoppy(0)
        , hPos(NULL)
        , hNor(NULL)
	{
	}

	HydrOCL::HydrOCL(Hydrax *h, const Ogre::Plane &BasePlane, const Options &Options)
		: Module("HydrOCL", new Noise::HydrOCLPerlin(), Mesh::Options(Options.Complexity, Size(0), Mesh::VT_POS_NORM), MaterialManager::NM_VERTEX)
		, mHydrax(h)
		, mVertices(0)
		, mVerticesChoppyBuffer(0)
		, mBasePlane(BasePlane)
		, mNormal(BasePlane.normal)
		, mPos(Ogre::Vector3(0,0,0))
		, mProjectingCamera(0)
		, mTmpRndrngCamera(0)
		, mRenderingCamera(h->getCamera())
        , mNumberOfPlatforms(0)
        , mPlatforms(NULL)
        , mPlatform(0)
        , mNumberOfDevices(0)
        , mDevices(NULL)
        , mContext(0)
        , mComQueue(NULL)
        , mAllocatedMem(0)
        , mVertexes(0)
        , mNormals(0)
        , mChoppyVertexes(0)
        , mChoppyNormals(0)
        , kGeometryGen(0)
        , kBasePlane(0)
        , kCopy(0)
        , kSmooth(0)
        , kNormals(0)
        , kChoppy(0)
        , hPos(NULL)
        , hNor(NULL)
	{
		setOptions(Options);
	}

	HydrOCL::~HydrOCL()
	{
		remove();

		HydraxLOG(getName() + " destroyed.");
	}

	void HydrOCL::setOptions(const Options &Options)
	{
		// Size(0) -> Infinite mesh
		mMeshOptions.MeshSize     = Size(0);
		mMeshOptions.MeshStrength = Options.Strength;
		mMeshOptions.MeshComplexity = Options.Complexity;

		mHydrax->getMesh()->setOptions(mMeshOptions);
		mHydrax->_setStrength(Options.Strength);

		// Re-create geometry if it's needed
		if (isCreated() && Options.Complexity != mOptions.Complexity) {
			remove();
			mOptions = Options;
			create();

		    Ogre::String MaterialNameTmp = mHydrax->getMesh()->getMaterialName();
		    mHydrax->getMesh()->remove();
		    mHydrax->getMesh()->setOptions(getMeshOptions());
		    mHydrax->getMesh()->setMaterialName(MaterialNameTmp);
		    mHydrax->getMesh()->create();

			// Force to recalculate the geometry on next frame
			mLastPosition = Ogre::Vector3(0,0,0);
			mLastOrientation = Ogre::Quaternion();

			return;
		}

		mOptions = Options;
	}

	void HydrOCL::create()
	{
	    int i;
	    // Create base module
		HydraxLOG("Creating " + getName() + " module.");
		Module::create();

	    // Create Vertexes buffers
        mVertices = new Mesh::POS_NORM_VERTEX[mOptions.Complexity*mOptions.Complexity];
        Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);
        for (int i = 0; i < mOptions.Complexity*mOptions.Complexity; i++) {
            Vertices[i].nx = 0;
            Vertices[i].ny = -1;
            Vertices[i].nz = 0;
        }
        mVerticesChoppyBuffer = new Mesh::POS_NORM_VERTEX[mOptions.Complexity*mOptions.Complexity];
	    _setDisplacementAmplitude(0.0f);
	    // Set rendering cameras
		mTmpRndrngCamera  = new Ogre::Camera("PG_TmpRndrngCamera", NULL);
		mProjectingCamera = new Ogre::Camera("PG_ProjectingCamera", NULL);
        // Start OpenCL platform
        if(!setupOpenCL()) {
            remove();
            return;
        }
        bool Error=false;
        // Use float4, is faster than float3
        Error |= !allocMemory(&mVertexes,       mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
        Error |= !allocMemory(&mNormals,        mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
        Error |= !allocMemory(&mChoppyVertexes, mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
        Error |= !allocMemory(&mChoppyNormals,  mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
        if(Error) {
            remove();
            return;
        }
        // Send initial values
        cl_uint clFlag=0;
        hPos = new cl_float4[mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 )];
        hNor = new cl_float4[mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 )];
        for(i=0;i<mOptions.Complexity*mOptions.Complexity;i++){
            hPos[i].x=0.f; hPos[i].y=0.f; hPos[i].z=0.f; hPos[i].w=1.f;
            hNor[i].x=0.f; hNor[i].y=-1.f; hNor[i].z=0.f; hNor[i].w=0.f;
        }
        //! @todo allow several devices usage
        clFlag |= sendData(mComQueue[0], mVertexes, hPos, mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
        clFlag |= sendData(mComQueue[0], mNormals,  hNor, mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
        clFlag |= sendData(mComQueue[0], mChoppyVertexes, hPos, mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
        clFlag |= sendData(mComQueue[0], mChoppyNormals,  hNor, mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Fail sending initial data to device.");
            remove();
            return;
        }
        // Send OpenCL stuff to noise module.
        if(! ((Noise::HydrOCLPerlin*)mNoise)->setupOpenCL(mNumberOfDevices, mContext, mDevices, mComQueue)){
            remove();
            return;
        }

		HydraxLOG(getName() + " created.");
	}

	void HydrOCL::remove()
	{
	    unsigned int i;
		if (!isCreated()) {
			return;
		}

		Module::remove();

		if (mVertices) {
            delete [] static_cast<Mesh::POS_NORM_VERTEX*>(mVertices); mVertices=NULL;
		}
		if (mVerticesChoppyBuffer) {
			delete [] mVerticesChoppyBuffer; mVerticesChoppyBuffer=NULL;
		}

		if (mTmpRndrngCamera) {
			delete mTmpRndrngCamera; mTmpRndrngCamera=NULL;
			delete mProjectingCamera; mProjectingCamera=NULL;
		}
		mLastPosition = Ogre::Vector3(0,0,0);
		mLastOrientation = Ogre::Quaternion();

		// Destroy OpenCL
		if(hPos) delete[] hPos; hPos=NULL;
		if(hNor) delete[] hNor; hNor=NULL;
        if(mVertexes)clReleaseMemObject(mVertexes); mVertexes=0;
        if(mNormals)clReleaseMemObject(mNormals); mNormals=0;
        if(mChoppyVertexes)clReleaseMemObject(mChoppyVertexes); mChoppyVertexes=0;
        if(mChoppyNormals)clReleaseMemObject(mChoppyNormals); mChoppyNormals=0;
        mAllocatedMem = 0;
        HydraxLOG("\tShutting down OpenCL...");
        if(kGeometryGen)clReleaseKernel(kGeometryGen); kGeometryGen=0;
        if(kBasePlane)clReleaseKernel(kBasePlane); kBasePlane=0;
        if(kCopy)clReleaseKernel(kCopy); kCopy=0;
        if(kSmooth)clReleaseKernel(kSmooth); kSmooth=0;
        if(kNormals)clReleaseKernel(kNormals); kNormals=0;
        if(kChoppy)clReleaseKernel(kChoppy); kChoppy=0;
        for(i=0;i<mNumberOfDevices;i++) {
            if(mComQueue[i])clReleaseCommandQueue(mComQueue[i]);
        }
        delete[] mComQueue; mComQueue=NULL;
        if(mContext) clReleaseContext(mContext); mContext=0;
        if(mDevices) delete[] mDevices; mDevices=NULL;
        mNumberOfDevices = 0;
        if(mPlatforms) delete[] mPlatforms; mPlatforms=NULL;
        mPlatform=0;
        mNumberOfPlatforms=0;
	}

	void HydrOCL::saveCfg(Ogre::String &Data)
	{
		Module::saveCfg(Data);

		Data += CfgFileManager::_getCfgString("PG_ChoopyStrength", mOptions.ChoppyStrength);
		Data += CfgFileManager::_getCfgString("PG_ChoppyWaves", mOptions.ChoppyWaves);
		Data += CfgFileManager::_getCfgString("PG_Complexity", mOptions.Complexity);
		Data += CfgFileManager::_getCfgString("PG_Elevation", mOptions.Elevation);
		Data += CfgFileManager::_getCfgString("PG_ForceRecalculateGeometry", mOptions.ForceRecalculateGeometry);
		Data += CfgFileManager::_getCfgString("PG_Smooth", mOptions.Smooth);
		Data += CfgFileManager::_getCfgString("PG_Strength", mOptions.Strength); Data += "\n";
		Data += CfgFileManager::_getCfgString("OCL_DeviceType", (int)mOptions.DeviceType); Data += "\n";
	}

	bool HydrOCL::loadCfg(Ogre::ConfigFile &CfgFile)
	{
		if (!Module::loadCfg(CfgFile))
		{
			return false;
		}

        HydraxLOG("\tReading options...");
		setOptions(
			Options(CfgFileManager::_getIntValue(CfgFile,   "PG_Complexity"),
			        CfgFileManager::_getFloatValue(CfgFile, "PG_Strength"),
					CfgFileManager::_getFloatValue(CfgFile, "PG_Elevation"),
					CfgFileManager::_getBoolValue(CfgFile,  "PG_Smooth"),
					CfgFileManager::_getBoolValue(CfgFile,  "PG_ForceRecalculateGeometry"),
					CfgFileManager::_getBoolValue(CfgFile,  "PG_ChoppyWaves"),
					CfgFileManager::_getFloatValue(CfgFile, "PG_ChoopyStrength"),
					(cl_device_type)CfgFileManager::_getIntValue(CfgFile, "OCL_DeviceType")));

        HydraxLOG("\tOptions readed.");

		return true;
	}

	void HydrOCL::update(const Ogre::Real &timeSinceLastFrame)
	{
		if (!isCreated()) {
			return;
		}

		Module::update(timeSinceLastFrame);

		Ogre::Vector3 RenderingCameraPos = mRenderingCamera->getDerivedPosition();

		if (mLastPosition    != RenderingCameraPos    ||
			mLastOrientation != mRenderingCamera->getDerivedOrientation() ||
			mOptions.ForceRecalculateGeometry)
		{
			if (mLastPosition != RenderingCameraPos) {
				Ogre::Vector3 HydraxPos = Ogre::Vector3(RenderingCameraPos.x,mHydrax->getPosition().y,RenderingCameraPos.z);

		        mHydrax->getMesh()->getSceneNode()->setPosition(HydraxPos);
		        mHydrax->getRttManager()->getPlanesSceneNode()->setPosition(HydraxPos);

		        // For world-space -> object-space conversion
				mHydrax->setSunPosition(mHydrax->getSunPosition());
			}

			float RenderingFarClipDistance = mRenderingCamera->getFarClipDistance();

		    if (RenderingFarClipDistance > _def_MaxFarClipDistance) {
			    mRenderingCamera->setFarClipDistance(_def_MaxFarClipDistance);
		    }

			mLastMinMax = _getMinMax(&mRange);

		    if (mLastMinMax) {
			    _renderGeometry(mRange, mProjectingCamera->getViewMatrix(), RenderingCameraPos);
			    mHydrax->getMesh()->updateGeometry(mOptions.Complexity*mOptions.Complexity, mVertices);
		    }

			mRenderingCamera->setFarClipDistance(RenderingFarClipDistance);
		}
		else if (mLastMinMax) {
            int i;
            cl_int clFlag=0;
            // Recover data from server. We will update geometry now in order to allow OpenCL compute next time step
            // while we wait for a new frame. So free surface height (y component) have one time step of delay.
            //! @todo allow several devices usage
            clFlag |= getData(mComQueue[0], hPos, mVertexes, mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
            clFlag |= getData(mComQueue[0], hNor, mNormals,  mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
            if(clFlag != CL_SUCCESS) {
                HydraxLOG("Can't get data from device.");
                return;
            }
            Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);
            for(i=0;i<mOptions.Complexity*mOptions.Complexity;i++){
                Vertices[i].x  = hPos[i].x; Vertices[i].y  = hPos[i].y; Vertices[i].z  = hPos[i].z;
                Vertices[i].nx = hNor[i].x; Vertices[i].ny = hNor[i].y; Vertices[i].nz = hNor[i].z;
            }
			mHydrax->getMesh()->updateGeometry(mOptions.Complexity*mOptions.Complexity, mVertices);
            //! @todo allow several devices usage
            cl_uint2 N;
            N.x = (unsigned int)mOptions.Complexity;
            N.y = (unsigned int)mOptions.Complexity;
            size_t localWorkSize[2], globalWorkSize[2];
            localWorkSize[0] = 256;
            localWorkSize[1] = 256;
            globalWorkSize[0] = roundUp(N.x, localWorkSize[0]);
            globalWorkSize[1] = roundUp(N.y, localWorkSize[1]);
            // Backup
            if (mOptions.ChoppyWaves) {
                clFlag |= sendArgument(kCopy,  0, sizeof(cl_mem   ), (void*)&mVertexes);
                clFlag |= sendArgument(kCopy,  1, sizeof(cl_mem   ), (void*)&mNormals);
                clFlag |= sendArgument(kCopy,  2, sizeof(cl_mem   ), (void*)&mChoppyVertexes);
                clFlag |= sendArgument(kCopy,  3, sizeof(cl_mem   ), (void*)&mChoppyNormals);
                clFlag |= sendArgument(kCopy,  4, sizeof(cl_uint2 ), (void*)&N);
                if(clFlag != CL_SUCCESS) {
                    HydraxLOG("Can't send arguments to copy processor.");
                    return;
                }
                clFlag = clEnqueueNDRangeKernel(mComQueue[0], kCopy, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
                if(clFlag != CL_SUCCESS) {
                    HydraxLOG("Copy data execution fail.");
                    return;
                }
            }
            // Base plane set
            float h = mBasePlane.d;
            clFlag |= sendArgument(kBasePlane,  0, sizeof(cl_mem   ), (void*)&mVertexes);
            clFlag |= sendArgument(kBasePlane,  1, sizeof(cl_float ), (void*)&h);
            clFlag |= sendArgument(kBasePlane,  2, sizeof(cl_uint2 ), (void*)&N);
            if(clFlag != CL_SUCCESS) {
                HydraxLOG("Can't send arguments to base plane set processor.");
                return;
            }
            clFlag = clEnqueueNDRangeKernel(mComQueue[0], kBasePlane, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
            if(clFlag != CL_SUCCESS) {
                HydraxLOG("Set base plane execution fail.");
                return;
            }
            // Noise computation
            ((Noise::HydrOCLPerlin*)mNoise)->setHeight(mVertexes, N, RenderingCameraPos);
            // Smooth the height data
            if (mOptions.Smooth) {
                clFlag |= sendArgument(kSmooth,  0, sizeof(cl_mem   ), (void*)&mVertexes);
                clFlag |= sendArgument(kSmooth,  1, sizeof(cl_uint2 ), (void*)&N);
                if(clFlag != CL_SUCCESS) {
                    HydraxLOG("Can't send arguments to copy processor.");
                    return;
                }
                clFlag = clEnqueueNDRangeKernel(mComQueue[0], kSmooth, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
                if(clFlag != CL_SUCCESS) {
                    HydraxLOG("Copy data execution fail.");
                    return;
                }
            }

            _calculeNormals();
            _performChoppyWaves();
		}

		mLastPosition = RenderingCameraPos;
		mLastOrientation = mRenderingCamera->getDerivedOrientation();
	}

	bool HydrOCL::_renderGeometry(const Ogre::Matrix4& m,const Ogre::Matrix4& _viewMat, const Ogre::Vector3& WorldPos)
	{
	    int i;
        cl_int clFlag=0;
		t_corners0 = _calculeWorldPosition(Ogre::Vector2( 0.0f, 0.0f),m,_viewMat);
		t_corners1 = _calculeWorldPosition(Ogre::Vector2(+1.0f, 0.0f),m,_viewMat);
		t_corners2 = _calculeWorldPosition(Ogre::Vector2( 0.0f,+1.0f),m,_viewMat);
		t_corners3 = _calculeWorldPosition(Ogre::Vector2(+1.0f,+1.0f),m,_viewMat);

        //! @todo allow several devices usage
        cl_uint2 N;
        N.x = (unsigned int)mOptions.Complexity;
        N.y = (unsigned int)mOptions.Complexity;
        size_t localWorkSize[2], globalWorkSize[2];
        localWorkSize[0] = 256;
        localWorkSize[1] = 256;
        globalWorkSize[0] = roundUp(N.x, localWorkSize[0]);
        globalWorkSize[1] = roundUp(N.y, localWorkSize[1]);
        // Geometry regeneration
        cl_float4 c0, c1, c2, c3;
        c0.x=t_corners0.x; c0.y=t_corners0.y; c0.z=t_corners0.z; c0.w=t_corners0.w;
        c1.x=t_corners1.x; c1.y=t_corners1.y; c1.z=t_corners1.z; c1.w=t_corners1.w;
        c2.x=t_corners2.x; c2.y=t_corners2.y; c2.z=t_corners2.z; c2.w=t_corners2.w;
        c3.x=t_corners3.x; c3.y=t_corners3.y; c3.z=t_corners3.z; c3.w=t_corners3.w;
        clFlag |= sendArgument(kGeometryGen,  0, sizeof(cl_mem   ), (void*)&mVertexes);
        clFlag |= sendArgument(kGeometryGen,  1, sizeof(cl_float4), (void*)&c0);
        clFlag |= sendArgument(kGeometryGen,  2, sizeof(cl_float4), (void*)&c1);
        clFlag |= sendArgument(kGeometryGen,  3, sizeof(cl_float4), (void*)&c2);
        clFlag |= sendArgument(kGeometryGen,  4, sizeof(cl_float4), (void*)&c3);
        clFlag |= sendArgument(kGeometryGen,  5, sizeof(cl_uint2 ), (void*)&N);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Can't send arguments to geometry generator.");
            return false;
        }
        clFlag = clEnqueueNDRangeKernel(mComQueue[0], kGeometryGen, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Geometry generator execution fail.");
            return false;
        }
        /* Recover data from server. We will update geometry now in order to allow OpenCL compute next time step
         * while we wait for a new frame. So free surface height (y component) have one time step of delay,
         * but vertexes position have been already updated (in order to avoid holes when camera is moved).
         */
        //! @todo allow several devices usage
        clFlag |= getData(mComQueue[0], hPos, mVertexes, mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
        clFlag |= getData(mComQueue[0], hNor, mNormals,  mOptions.Complexity*mOptions.Complexity*sizeof( cl_float4 ));
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Can't get data from device.");
            return false;
        }
        Mesh::POS_NORM_VERTEX* Vertices = static_cast<Mesh::POS_NORM_VERTEX*>(mVertices);
        for(i=0;i<mOptions.Complexity*mOptions.Complexity;i++){
            Vertices[i].x  = hPos[i].x; Vertices[i].y  = hPos[i].y; Vertices[i].z  = hPos[i].z;
            Vertices[i].nx = hNor[i].x; Vertices[i].ny = hNor[i].y; Vertices[i].nz = hNor[i].z;
        }
        // Base plane set
        float h = mBasePlane.d;
        clFlag |= sendArgument(kBasePlane,  0, sizeof(cl_mem   ), (void*)&mVertexes);
        clFlag |= sendArgument(kBasePlane,  1, sizeof(cl_float ), (void*)&h);
        clFlag |= sendArgument(kBasePlane,  2, sizeof(cl_uint2 ), (void*)&N);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Can't send arguments to base plane set processor.");
            return false;
        }
        clFlag = clEnqueueNDRangeKernel(mComQueue[0], kBasePlane, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Set base plane execution fail.");
            return false;
        }
        // Noise computation
        ((Noise::HydrOCLPerlin*)mNoise)->setHeight(mVertexes, N, WorldPos);
        // Backup
        if (mOptions.ChoppyWaves) {
            clFlag |= sendArgument(kCopy,  0, sizeof(cl_mem   ), (void*)&mChoppyVertexes);
            clFlag |= sendArgument(kCopy,  1, sizeof(cl_mem   ), (void*)&mChoppyNormals);
            clFlag |= sendArgument(kCopy,  2, sizeof(cl_mem   ), (void*)&mVertexes);
            clFlag |= sendArgument(kCopy,  3, sizeof(cl_mem   ), (void*)&mNormals);
            clFlag |= sendArgument(kCopy,  4, sizeof(cl_uint2 ), (void*)&N);
            if(clFlag != CL_SUCCESS) {
                HydraxLOG("Can't send arguments to copy processor.");
                return false;
            }
            clFlag = clEnqueueNDRangeKernel(mComQueue[0], kCopy, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
            if(clFlag != CL_SUCCESS) {
                HydraxLOG("Copy data execution fail.");
                return false;
            }
        }
		// Smooth the heightdata
		if (mOptions.Smooth) {
            clFlag |= sendArgument(kSmooth,  0, sizeof(cl_mem   ), (void*)&mVertexes);
            clFlag |= sendArgument(kSmooth,  1, sizeof(cl_uint2 ), (void*)&N);
            if(clFlag != CL_SUCCESS) {
                HydraxLOG("Can't send arguments to copy processor.");
                return false;
            }
            clFlag = clEnqueueNDRangeKernel(mComQueue[0], kSmooth, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
            if(clFlag != CL_SUCCESS) {
                HydraxLOG("Copy data execution fail.");
                return false;
            }
		}

		_calculeNormals();
		_performChoppyWaves();

		return true;
	}

	void HydrOCL::_calculeNormals()
	{
        cl_int clFlag=0;
        //! @todo allow several devices usage
        cl_uint2 N;
        N.x = (unsigned int)mOptions.Complexity;
        N.y = (unsigned int)mOptions.Complexity;
        size_t localWorkSize[2], globalWorkSize[2];
        localWorkSize[0] = 256;
        localWorkSize[1] = 256;
        globalWorkSize[0] = roundUp(N.x, localWorkSize[0]);
        globalWorkSize[1] = roundUp(N.y, localWorkSize[1]);
        // Normals computation
        clFlag |= sendArgument(kNormals,  0, sizeof(cl_mem   ), (void*)&mVertexes);
        clFlag |= sendArgument(kNormals,  1, sizeof(cl_mem   ), (void*)&mNormals);
        clFlag |= sendArgument(kNormals,  2, sizeof(cl_uint2 ), (void*)&N);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Can't send arguments to normals computator.");
            return;
        }
        clFlag = clEnqueueNDRangeKernel(mComQueue[0], kNormals, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Normals computation execution fail.");
            return;
        }
	}

	void HydrOCL::_performChoppyWaves()
	{
		if (!mOptions.ChoppyWaves) {
			return;
		}

        cl_int clFlag=0;
        //! @todo allow several devices usage
        cl_uint2 N;
        N.x = (unsigned int)mOptions.Complexity;
        N.y = (unsigned int)mOptions.Complexity;
        size_t localWorkSize[2], globalWorkSize[2];
        localWorkSize[0] = 256;
        localWorkSize[1] = 256;
        globalWorkSize[0] = roundUp(N.x, localWorkSize[0]);
        globalWorkSize[1] = roundUp(N.y, localWorkSize[1]);
        // Choppy waves computation
        float underwater = 1.f;
        if (mHydrax->_isCurrentFrameUnderwater()) {
			underwater = -1.f;
		}
        Ogre::Vector3 CameraDir = mRenderingCamera->getDerivedDirection();
        cl_float4 camDir;
        camDir.z = CameraDir.z; camDir.z = CameraDir.z; camDir.z = CameraDir.z; camDir.w = 0.f;
        clFlag |= sendArgument(kChoppy,  0, sizeof(cl_mem   ), (void*)&mVertexes);
        clFlag |= sendArgument(kChoppy,  1, sizeof(cl_mem   ), (void*)&mChoppyVertexes);
        clFlag |= sendArgument(kChoppy,  2, sizeof(cl_mem   ), (void*)&mNormals);
        clFlag |= sendArgument(kChoppy,  3, sizeof(cl_float4), (void*)&camDir);
        clFlag |= sendArgument(kChoppy,  4, sizeof(cl_float ), (void*)&mOptions.ChoppyStrength);
        clFlag |= sendArgument(kChoppy,  5, sizeof(cl_float ), (void*)&underwater);
        clFlag |= sendArgument(kChoppy,  6, sizeof(cl_uint2 ), (void*)&N);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Can't send arguments to choppy waves computation.");
            return;
        }
        clFlag = clEnqueueNDRangeKernel(mComQueue[0], kChoppy, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("Choppy waves execution fail.");
            return;
        }
	}

	// Check the point of intersection with the plane (0,1,0,0) and return the position in homogenous coordinates
	Ogre::Vector4 HydrOCL::_calculeWorldPosition(const Ogre::Vector2 &uv, const Ogre::Matrix4& m, const Ogre::Matrix4& _viewMat)
	{
		Ogre::Vector4 origin(uv.x,uv.y,-1,1);
		Ogre::Vector4 direction(uv.x,uv.y,1,1);

		origin = m*origin;
		direction = m*direction;

		Ogre::Vector3 _org(origin.x/origin.w,origin.y/origin.w,origin.z/origin.w);
		Ogre::Vector3 _dir(direction.x/direction.w,direction.y/direction.w,direction.z/direction.w);
		_dir -= _org;
		_dir.normalise();

		Ogre::Ray _ray(_org,_dir);
		std::pair<bool,Ogre::Real> _result = _ray.intersects(mBasePlane);
		float l = _result.second;
		Ogre::Vector3 worldPos = _org + _dir*l;
		Ogre::Vector4 _tempVec = _viewMat*Ogre::Vector4(worldPos);
		float _temp = -_tempVec.z/_tempVec.w;
		Ogre::Vector4 retPos(worldPos);
		retPos /= _temp;

		return retPos;
	}

	bool HydrOCL::_getMinMax(Ogre::Matrix4 *range)
	{
		_setDisplacementAmplitude(mOptions.Strength);

		float x_min,y_min,x_max,y_max;
		Ogre::Vector3 frustum[8],proj_points[24];

		int i,
			n_points = 0,
			src, dst;

		int cube[] =
		   {0,1,	0,2,	2,3,	1,3,
		    0,4,	2,6,	3,7,	1,5,
		    4,6,	4,5,	5,7,	6,7};

		Ogre::Vector3 _testLine;
		Ogre::Real _dist;
		Ogre::Ray _ray;

		std::pair<bool,Ogre::Real> _result;

		// Set temporal rendering camera parameters
		mTmpRndrngCamera->setFrustumOffset(mRenderingCamera->getFrustumOffset());
		mTmpRndrngCamera->setAspectRatio(mRenderingCamera->getAspectRatio());
		mTmpRndrngCamera->setDirection(mRenderingCamera->getDerivedDirection());
		mTmpRndrngCamera->setFarClipDistance(mRenderingCamera->getFarClipDistance());
		mTmpRndrngCamera->setFOVy(mRenderingCamera->getFOVy());
		mTmpRndrngCamera->setNearClipDistance(mRenderingCamera->getNearClipDistance());
		mTmpRndrngCamera->setOrientation(mRenderingCamera->getDerivedOrientation());
		mTmpRndrngCamera->setPosition(0, mRenderingCamera->getDerivedPosition().y - mHydrax->getPosition().y, 0);

		Ogre::Matrix4 invviewproj = (mTmpRndrngCamera->getProjectionMatrixWithRSDepth()*mTmpRndrngCamera->getViewMatrix()).inverse();
		frustum[0] = invviewproj * Ogre::Vector3(-1,-1,0);
		frustum[1] = invviewproj * Ogre::Vector3(+1,-1,0);
		frustum[2] = invviewproj * Ogre::Vector3(-1,+1,0);
		frustum[3] = invviewproj * Ogre::Vector3(+1,+1,0);
		frustum[4] = invviewproj * Ogre::Vector3(-1,-1,+1);
		frustum[5] = invviewproj * Ogre::Vector3(+1,-1,+1);
		frustum[6] = invviewproj * Ogre::Vector3(-1,+1,+1);
		frustum[7] = invviewproj * Ogre::Vector3(+1,+1,+1);

		// Check intersections with upper_bound and lower_bound
		for(i=0; i<12; i++) {
			src=cube[i*2]; dst=cube[i*2+1];
			_testLine = frustum[dst]-frustum[src];
			_dist = _testLine.normalise();
			_ray = Ogre::Ray(frustum[src], _testLine);
			_result = Ogre::Math::intersects(_ray,mUpperBoundPlane);
			if ((_result.first) && (_result.second<_dist+0.00001)) {
				proj_points[n_points++] = frustum[src] + _result.second * _testLine;
			}
			_result = Ogre::Math::intersects(_ray,mLowerBoundPlane);
			if ((_result.first) && (_result.second<_dist+0.00001)) {
				proj_points[n_points++] = frustum[src] + _result.second * _testLine;
			}
		}

		// Check if any of the frustums vertices lie between the upper_bound and lower_bound planes
		for(i=0; i<8; i++) {
			if(mUpperBoundPlane.getDistance(frustum[i])/mLowerBoundPlane.getDistance(frustum[i]) < 0) {
				proj_points[n_points++] = frustum[i];
			}
		}

		// Set projecting camera parameters
		mProjectingCamera->setFrustumOffset(mTmpRndrngCamera->getFrustumOffset());
		mProjectingCamera->setAspectRatio(mTmpRndrngCamera->getAspectRatio());
		mProjectingCamera->setDirection(mTmpRndrngCamera->getDerivedDirection());
		mProjectingCamera->setFarClipDistance(mTmpRndrngCamera->getFarClipDistance());
		mProjectingCamera->setFOVy(mTmpRndrngCamera->getFOVy());
		mProjectingCamera->setNearClipDistance(mTmpRndrngCamera->getNearClipDistance());
		mProjectingCamera->setOrientation(mTmpRndrngCamera->getDerivedOrientation());
		mProjectingCamera->setPosition(mTmpRndrngCamera->getDerivedPosition());

		// Make sure the camera isn't too close to the plane
		float height_in_plane = mBasePlane.getDistance(mProjectingCamera->getRealPosition());

		bool keep_it_simple = false,
			 underwater     = false;

		if (height_in_plane < 0.0f) {
			underwater = true;
		}

		if (keep_it_simple) {
			mProjectingCamera->setDirection(mTmpRndrngCamera->getDerivedDirection());
		}
		else {
			Ogre::Vector3 aimpoint, aimpoint2;

			if (height_in_plane < (mOptions.Strength + mOptions.Elevation)) {
				if (underwater) {
					mProjectingCamera->setPosition(mProjectingCamera->getRealPosition()+mLowerBoundPlane.normal*(mOptions.Strength + mOptions.Elevation - 2*height_in_plane));
				}
				else {
					mProjectingCamera->setPosition(mProjectingCamera->getRealPosition()+mLowerBoundPlane.normal*(mOptions.Strength + mOptions.Elevation - height_in_plane));
				}
			}

			// Aim the projector at the point where the camera view-vector intersects the plane
			// if the camera is aimed away from the plane, mirror it's view-vector against the plane
			if (((mBasePlane.normal).dotProduct(mTmpRndrngCamera->getDerivedDirection()) < 0.0f) || ((mBasePlane.normal).dotProduct(mTmpRndrngCamera->getDerivedPosition()) < 0.0f ) ) {
				_ray = Ogre::Ray(mTmpRndrngCamera->getDerivedPosition(), mTmpRndrngCamera->getDerivedDirection());
				_result = Ogre::Math::intersects(_ray,mBasePlane);

				if(!_result.first) {
					_result.second = -_result.second;
				}

				aimpoint = mTmpRndrngCamera->getDerivedPosition() + _result.second * mTmpRndrngCamera->getDerivedDirection();
			}
			else {
				Ogre::Vector3 flipped = mTmpRndrngCamera->getDerivedDirection() - 2*mNormal* (mTmpRndrngCamera->getDerivedDirection()).dotProduct(mNormal);
				flipped.normalise();
				_ray = Ogre::Ray( mTmpRndrngCamera->getDerivedPosition(), flipped);
				_result = Ogre::Math::intersects(_ray,mBasePlane);

				aimpoint = mTmpRndrngCamera->getDerivedPosition() + _result.second * flipped;
			}

			// Force the point the camera is looking at in a plane, and have the projector look at it
			// works well against horizon, even when camera is looking upwards
			// doesn't work straight down/up
			float af = fabs((mBasePlane.normal).dotProduct(mTmpRndrngCamera->getDerivedDirection()));
			aimpoint2 = mTmpRndrngCamera->getDerivedPosition() + 10.0*mTmpRndrngCamera->getDerivedDirection();
			aimpoint2 = aimpoint2 - mNormal* (aimpoint2.dotProduct(mNormal));

			// Fade between aimpoint & aimpoint2 depending on view angle
			aimpoint = aimpoint*af + aimpoint2*(1.0f-af);

			mProjectingCamera->setDirection(aimpoint-mProjectingCamera->getRealPosition());
		}

		for(i=0; i<n_points; i++) {
			// Project the point onto the surface plane
			proj_points[i] = proj_points[i] - mBasePlane.normal*mBasePlane.getDistance(proj_points[i]);
			proj_points[i] = mProjectingCamera->getViewMatrix() * proj_points[i];
			proj_points[i] = mProjectingCamera->getProjectionMatrixWithRSDepth() * proj_points[i];
		}

		// Get max/min x & y-values to determine how big the "projection window" must be
		if (n_points > 0) {
			x_min = proj_points[0].x;
			x_max = proj_points[0].x;
			y_min = proj_points[0].y;
			y_max = proj_points[0].y;

			for(i=1; i<n_points; i++) {
				if (proj_points[i].x > x_max) x_max = proj_points[i].x;
				if (proj_points[i].x < x_min) x_min = proj_points[i].x;
				if (proj_points[i].y > y_max) y_max = proj_points[i].y;
				if (proj_points[i].y < y_min) y_min = proj_points[i].y;
			}

			// Build the packing matrix that spreads the grid across the "projection window"
			Ogre::Matrix4 pack(x_max-x_min,	0,				0,		x_min,
				               0,			y_max-y_min,	0,		y_min,
				               0,			0,				1,		0,
				               0,			0,				0,		1);

			Ogre::Matrix4 invviewproj = (mProjectingCamera->getProjectionMatrixWithRSDepth()*mProjectingCamera->getViewMatrix()).inverse();
			*range = invviewproj * pack;

			return true;
		}

		return false;
	}

	void HydrOCL::_setDisplacementAmplitude(const float &Amplitude)
	{
		mUpperBoundPlane = Ogre::Plane( mNormal, mPos + Amplitude * mNormal);
		mLowerBoundPlane = Ogre::Plane( mNormal, mPos - Amplitude * mNormal);
	}

	float HydrOCL::getHeigth(const Ogre::Vector2 &Position)
	{
		return mHydrax->getPosition().y + mNoise->getValue(Position.x, Position.y)*mOptions.Strength;
	}

    bool HydrOCL::setupOpenCL()
    {
        HydraxLOG("\tInitializating OpenCL...");

        //! Get platform
        if(!getPlatform()){
            return false;
        }
        //! Get available devices
        if(!getDevices()){
            return false;
        }
        //! Build kernels
        const char* path = fileFromResources("grid.cl");
        if(!path){
            HydraxLOG("\tGrid OpenCL program can't be found!");
            return false;
        }
        //! @todo Allow several devices use.
        kGeometryGen = loadKernelFromFile(mContext, mDevices[0], path, "geometry", "");
        kBasePlane   = loadKernelFromFile(mContext, mDevices[0], path, "setBasePlane", "");
        kCopy        = loadKernelFromFile(mContext, mDevices[0], path, "copy", "");
        kSmooth      = loadKernelFromFile(mContext, mDevices[0], path, "smooth", "");
        kNormals     = loadKernelFromFile(mContext, mDevices[0], path, "normals", "");
        kChoppy      = loadKernelFromFile(mContext, mDevices[0], path, "choppyWaves", "");
        if( !kGeometryGen || !kBasePlane || !kCopy || !kSmooth || !kNormals || !kChoppy ){
            return false;
        }

        HydraxLOG("\tOpenCL ready to work!");
        return true;
    }

    bool HydrOCL::getPlatform()
    {
        cl_uint i;
        cl_int clFlag;
        char PlatformName[1024];
        bool HavePlatform=false;

        // Gets the number of valid platforms
        clFlag = clGetPlatformIDs(0, NULL, &mNumberOfPlatforms);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("\t\tCan't get number of platforms.");
            return false;
        }
        if(mNumberOfPlatforms <= 0) {
            HydraxLOG("\t\tNot valid platforms present.");
            return false;
        }
        // Gets the platform array
        mPlatforms = new cl_platform_id[mNumberOfPlatforms];
        clFlag = clGetPlatformIDs(mNumberOfPlatforms, mPlatforms, NULL);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("\t\tPlatforms can't be written.");
            return false;
        }
        // Gets the names of platforms
        for(i=0;i<mNumberOfPlatforms;i++) {
            clFlag = clGetPlatformInfo(mPlatforms[i], CL_PLATFORM_NAME, 1024*sizeof(char), &PlatformName, NULL);
            if(clFlag != CL_SUCCESS) {
                continue;
            }
            HydraxLOG(Ogre::String("\t\tFound platform: ") + PlatformName);
            // Look for valid devices into the platform
            clFlag = clGetDeviceIDs (mPlatforms[i], mOptions.DeviceType, 0, NULL, &mNumberOfDevices);
            if( (clFlag != CL_SUCCESS) || (mNumberOfDevices <= 0) ){
                HydraxLOG("\t\tDiscarded.");
                continue;
            }
            mPlatform = mPlatforms[i];
            HydraxLOG("\t\tPlatform selected!");
            HavePlatform=true;
            return true;
        }
        if(!HavePlatform) {
            HydraxLOG("\t\tAny platform matchs with requested platform (probaly because device type is not available).");
            return false;
        }
        return true;
    }

    bool HydrOCL::getDevices()
    {
        cl_uint i;
        cl_int clFlag;
        char DeviceName[1024];

        // Gets the number of valid devices
        clFlag = clGetDeviceIDs (mPlatform, mOptions.DeviceType, 0, NULL, &mNumberOfDevices);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("\t\tCan't take the number of devices.");
            return false;
        }
        if(mNumberOfDevices <= 0) {
            HydraxLOG("\t\tCan't find any valid device of selected type.");
            return false;
        }
        // Gets the devices array
        mDevices = new cl_device_id[mNumberOfDevices];
        clFlag = clGetDeviceIDs(mPlatform, mOptions.DeviceType, mNumberOfDevices, mDevices, &mNumberOfDevices);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("\t\tCan't write devices array.");
            return false;
        }
        // Create an OpenCL context
        mContext = clCreateContext(0, mNumberOfDevices, mDevices, NULL, NULL, &clFlag);
        if(clFlag != CL_SUCCESS) {
            if(clFlag == CL_DEVICE_NOT_AVAILABLE){
                HydraxLOG("\t\tCan't create the context, selected devices are not availables.");
            }
            else if(clFlag == CL_OUT_OF_HOST_MEMORY){
                HydraxLOG("\t\tCan't create the context, host is out of memory.");
            }
            return false;
        }
        // Create command queues for each device
        mComQueue = new cl_command_queue[mNumberOfDevices];
        for(i=0;i<mNumberOfDevices;i++) {
            mComQueue[i] = clCreateCommandQueue(mContext, mDevices[i], 0, &clFlag);
            if(clFlag != CL_SUCCESS) {
                HydraxLOG("\t\tCan't create command queue.");
                return false;
            }
        }
        // Gets devices name
        HydraxLOG("\t\tDevices found:");
        for(i=0;i<mNumberOfDevices;i++) {
            clGetDeviceInfo(mDevices[i], CL_DEVICE_NAME, 1024*sizeof(char), &DeviceName, NULL);
            HydraxLOG(Ogre::String("\t\t\t") + DeviceName);
        }
        return true;
    }

    bool HydrOCL::allocMemory(cl_mem *clID, size_t size)
    {
        int clFlag;
        *clID = clCreateBuffer(mContext, CL_MEM_READ_WRITE, size, NULL, &clFlag);
        if(clFlag != CL_SUCCESS) {
            HydraxLOG("\t\tDevice memory allocation fail.");
            return false;
        }

        mAllocatedMem += size;
        return true;
    }

}}
