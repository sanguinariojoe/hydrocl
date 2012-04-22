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

#ifndef HYDROCLGRID_H_INCLUDED
#define HYDROCLGRID_H_INCLUDED

// ----------------------------------------------------------------------------
// Hydrax plugin
// ----------------------------------------------------------------------------
#include <Hydrax/Prerequisites.h>
#include <Hydrax/Hydrax.h>
#include <Hydrax/Mesh.h>
#include <Hydrax/Modules/Module.h>

// ----------------------------------------------------------------------------
// OpenCL libraries
// ----------------------------------------------------------------------------
#include <CL/cl.h>

namespace Hydrax{ namespace Module
{
	/** Hydrax projected grid module
	 */
	class DllExport HydrOCL : public Module
	{
	public:
		/** Struct wich contains Hydrax projected grid module options
		 */
		struct Options
		{
		    // --------------------------------------------
		    // Projected grid options
		    // --------------------------------------------
			/// Projected grid complexity (N*N)
			int Complexity;
			/// Strength
			float Strength;
			/// Elevation
			float Elevation;
			/// Smooth
			bool Smooth;
			/// Force recalculate mesh geometry each frame
			bool ForceRecalculateGeometry;
			/// Choppy waves
			bool ChoppyWaves;
			/// Choppy waves strength
			float ChoppyStrength;
		    // --------------------------------------------
		    // OpenCL options
		    // --------------------------------------------
		    /** Device type used \n
		     * CL_DEVICE_TYPE_DEFAULT \n
		     * CL_DEVICE_TYPE_ALL \n
		     * CL_DEVICE_TYPE_GPU \n
		     * CL_DEVICE_TYPE_CPU \n
		     * CL_DEVICE_TYPE_ACCELERATOR
		     */
            cl_device_type DeviceType;

			/** Default constructor
			 */
			Options()
				: Complexity(256)
				, Strength(35.0f)
				, Elevation(50.0f)
				, Smooth(false)
				, ForceRecalculateGeometry(false)
				, ChoppyWaves(true)
				, ChoppyStrength(3.75f)
				, DeviceType(CL_DEVICE_TYPE_ALL)
			{
			}

			/** Constructor
			    @param _Complexity Projected grid complexity
			 */
			Options(const int &_Complexity)
				: Complexity(_Complexity)
				, Strength(35.0f)
				, Elevation(50.0f)
				, Smooth(false)
				, ForceRecalculateGeometry(false)
				, ChoppyWaves(true)
				, ChoppyStrength(3.75f)
				, DeviceType(CL_DEVICE_TYPE_ALL)
			{
			}

			/** Constructor
			    @param _Complexity Projected grid complexity
				@param _Strength Perlin noise strength
				@param _Elevation Elevation
				@param _Smooth Smooth vertex?
			 */
			Options(const int   &_Complexity,
				    const float &_Strength,
					const float &_Elevation,
					const bool  &_Smooth)
				: Complexity(_Complexity)
				, Strength(_Strength)
				, Elevation(_Elevation)
				, Smooth(_Smooth)
				, ForceRecalculateGeometry(false)
				, ChoppyWaves(true)
				, ChoppyStrength(3.75f)
				, DeviceType(CL_DEVICE_TYPE_ALL)
			{
			}

			/** Constructor
			    @param _Complexity Projected grid complexity
				@param _Strength Perlin noise strength
				@param _Elevation Elevation
				@param _Smooth Smooth vertex?
				@param _ForceRecalculateGeometry Force to recalculate the projected grid geometry each frame
				@param _ChoppyWaves Choppy waves enabled? Note: Only with Materialmanager::NM_VERTEX normal mode.
				@param _ChoppyStrength Choppy waves strength
			 */
			Options(const int            &_Complexity,
				    const float          &_Strength,
					const float          &_Elevation,
					const bool           &_Smooth,
					const bool           &_ForceRecalculateGeometry,
					const bool           &_ChoppyWaves,
					const float          &_ChoppyStrength,
					const cl_device_type &_DeviceType)
				: Complexity(_Complexity)
				, Strength(_Strength)
				, Elevation(_Elevation)
				, Smooth(_Smooth)
				, ForceRecalculateGeometry(_ForceRecalculateGeometry)
				, ChoppyWaves(_ChoppyWaves)
				, ChoppyStrength(_ChoppyStrength)
				, DeviceType(_DeviceType)
			{
			}
		};

		/** Constructor
		    @param h Hydrax manager pointer
			@param BasePlane Noise base plane
		 */
		HydrOCL(Hydrax *h, const Ogre::Plane &BasePlane);

		/** Constructor
		    @param h Hydrax manager pointer
			@param BasePlane Noise base plane
			@param Options Perlin options
		 */
		HydrOCL(Hydrax *h, const Ogre::Plane &BasePlane, const Options &Options);

		/** Destructor
		 */
        ~HydrOCL();

		/** Create
		 */
		void create();

		/** Remove
		 */
		void remove();

		/** Call it each frame
		    @param timeSinceLastFrame Time since last frame(delta)
		 */
		void update(const Ogre::Real &timeSinceLastFrame);

		/** Set options
		    @param Options Options
		 */
		void setOptions(const Options &Options);

		/** Save config
		    @param Data String reference
		 */
		void saveCfg(Ogre::String &Data);

		/** Load config
		    @param CgfFile Ogre::ConfigFile reference
			@return True if is the correct module config
		 */
		bool loadCfg(Ogre::ConfigFile &CfgFile);

		/** Get the current heigth at a especified world-space point
		    @param Position X/Z World position
			@return Heigth at the given position in y-World coordinates, if it's outside of the water return -1
		 */
		float getHeigth(const Ogre::Vector2 &Position);

		/** Get current options
		    @return Current options
		 */
		inline const Options& getOptions() const
		{
			return mOptions;
		}

	private:
		/** Calcule current normals
		 */
		void _calculeNormals();

		/** Perform choppy waves
		 */
		void _performChoppyWaves();

		/** Render geometry
		    @param m Range
			@param _viewMat View matrix
			@param WorldPos Origin world position
			@return true if it's sucesfful
		 */
		bool _renderGeometry(const Ogre::Matrix4& m,const Ogre::Matrix4& _viewMat, const Ogre::Vector3& WorldPos);

		/** Calcule world position
		    @param uv uv
			@param m Range
			@param _viewMat View matrix
			@return The position in homogenous coordinates
		 */
		Ogre::Vector4 _calculeWorldPosition(const Ogre::Vector2 &uv, const Ogre::Matrix4& m,const Ogre::Matrix4& _viewMat);

		/** Get min/max
		    @param range Range
			@return true if it's in min/max
		 */
	    bool _getMinMax(Ogre::Matrix4 *range);

		/** Set displacement amplitude
		    @param Amplitude Amplitude to set
		 */
		void _setDisplacementAmplitude(const float &Amplitude);

        /** Creates OpenCL computational context.
         * @return true if OpenCL has been already initializated.
         */
        bool setupOpenCL();
        /** Method that looks for a valid platform.
         * @return true if OpenCL platform has been already selected.
         */
        bool getPlatform();
        /** Method that looks for a valid device.
         * @return true if valid device has been already selected.
         */
        bool getDevices();

        /** Allocates memory into the context.
         * @return true if memory has been allocated.
         */
        bool allocMemory(cl_mem *clID, size_t size);

		/// Vertex pointer (Mesh::POS_NORM_VERTEX or Mesh::POS_VERTEX)
		void *mVertices;
		/// Use it to store vertex positions when choppy displacement is enabled
		Mesh::POS_NORM_VERTEX* mVerticesChoppyBuffer;
		/// For corners
		Ogre::Vector4 t_corners0,t_corners1,t_corners2,t_corners3;
		/// Range matrix
		Ogre::Matrix4 mRange;
		/// Planes
	    Ogre::Plane	mBasePlane,
			        mUpperBoundPlane,
					mLowerBoundPlane;
		/// Cameras
	    Ogre::Camera *mProjectingCamera,	// The camera that does the actual projection
		             *mRenderingCamera,		// The camera whose frustum the projection is created for
					 *mTmpRndrngCamera;     // Used to allow cameras with any inherited from a node or nodes
		/// Normal and position
	    Ogre::Vector3 mNormal, mPos;
		/// Last camera position, orientation
		Ogre::Vector3 mLastPosition;
		Ogre::Quaternion mLastOrientation;
		bool mLastMinMax;
		/// Our projected grid options
		Options mOptions;
		/// Our Hydrax pointer
		Hydrax* mHydrax;

        // ------------------------------
        // OpenCL stuff
        // ------------------------------
        /// Number of available platforms
        cl_uint mNumberOfPlatforms;
        /// Array of platforms
        cl_platform_id *mPlatforms;
        /// Selected platform
        cl_platform_id mPlatform;
        /// Number of devices
        cl_uint mNumberOfDevices;
        /// Array of devices
        cl_device_id *mDevices;
        /// OpenCL context
        cl_context mContext;
        /// OpenCL context
        cl_command_queue *mComQueue;
        /// Device allocated memory
        size_t mAllocatedMem;
        /// In device vertexes
        cl_mem mVertexes;
        /// In device normals
        cl_mem mNormals;
        /// In device vertexes
        cl_mem mChoppyVertexes;
        /// In device normals
        cl_mem mChoppyNormals;
        /// OpenCL geometry regeneration kernel.
        cl_kernel kGeometryGen;
        /// OpenCL base plane set.
        cl_kernel kBasePlane;
        /// OpenCL vertexes & normals copy operation.
        cl_kernel kCopy;
        /// OpenCL smoothing kernel.
        cl_kernel kSmooth;
        /// OpenCL normals computation kernel.
        cl_kernel kNormals;
        /// OpenCL choppy waves computation kernel.
        cl_kernel kChoppy;
        /// Positions transfer layer
        cl_float4 *hPos;
        /// Normals transfer layer
        cl_float4 *hNor;
	};
}}

#endif  // HYDROCLGRID_H_INCLUDED
