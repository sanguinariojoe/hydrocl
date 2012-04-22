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

#ifndef HYDROCLPERLIN_H_INCLUDED
#define HYDROCLPERLIN_H_INCLUDED

// ----------------------------------------------------------------------------
// Hydrax plugin
// ----------------------------------------------------------------------------
#include <Hydrax/Prerequisites.h>
#include <Hydrax/Noise/Noise.h>

// ----------------------------------------------------------------------------
// OpenCL libraries
// ----------------------------------------------------------------------------
#include <CL/cl.h>

#define n_bits				5
#define n_size				(1<<(n_bits-1))
#define n_size_m1			(n_size - 1)
#define n_size_sq			(n_size*n_size)
#define n_size_sq_m1		(n_size_sq - 1)

#define n_packsize			4

#define np_bits				(n_bits+n_packsize-1)
#define np_size				(1<<(np_bits-1))
#define np_size_m1			(np_size-1)
#define np_size_sq			(np_size*np_size)
#define np_size_sq_m1		(np_size_sq-1)

#define n_dec_bits			12
#define n_dec_magn			4096
#define n_dec_magn_m1		4095

#define max_octaves			32

#define noise_frames		256
#define noise_frames_m1		(noise_frames-1)

#define noise_decimalbits	15
#define noise_magnitude		(1<<(noise_decimalbits-1))

#define scale_decimalbits	15
#define scale_magnitude		(1<<(scale_decimalbits-1))

namespace Hydrax{ namespace Noise
{
	/** OpenCL accelerated perlin noise module class
	 */
	class DllExport HydrOCLPerlin : public Noise
	{
	public:
		/** Struct wich contains HydrOCLPerlin noise module options
		 */
		struct Options
		{
			/// Octaves
			int Octaves;
			/// Scale
			float Scale;
			/// Falloff
			float Falloff;
			/// Animspeed
			float Animspeed;
			/// Timemulti
			float Timemulti;

			/** GPU Normal map generator parameters
			    Only if GPU normal map generation is active
		     */
			/// Representes the strength of the normals (i.e. Amplitude)
			float GPU_Strength;
			/** LOD Parameters, in order to obtain a smooth normal map we need to
                decrease the detail level when the pixel is far to the camera.
				This parameters are stored in an Ogre::Vector3:
				x -> Initial LOD value (Bigger values -> less detail)
				y -> Final LOD value
				z -> Final distance
			 */
			Ogre::Vector3 GPU_LODParameters;

			/** Default constructor
			 */
			Options()
				: Octaves(8)
				, Scale(0.085f)
				, Falloff(0.49f)
				, Animspeed(1.4f)
				, Timemulti(1.27f)
				, GPU_Strength(2.0f)
				, GPU_LODParameters(Ogre::Vector3(0.5f, 50, 150000))
			{
			}

			/** Constructor
				@param _Octaves HydrOCLPerlin noise octaves
				@param _Scale Noise scale
				@param _Falloff Noise fall off
				@param _Animspeed Animation speed
				@param _Timemulti Timemulti
			 */
			Options(const int   &_Octaves,
					const float &_Scale,
					const float &_Falloff,
					const float &_Animspeed,
					const float &_Timemulti)
				: Octaves(_Octaves)
				, Scale(_Scale)
				, Falloff(_Falloff)
				, Animspeed(_Animspeed)
				, Timemulti(_Timemulti)
				, GPU_Strength(2.0f)
				, GPU_LODParameters(Ogre::Vector3(0.5f, 50, 150000))
			{
			}

			/** Constructor
				@param _Octaves HydrOCLPerlin noise octaves
				@param _Scale Noise scale
				@param _Falloff Noise fall off
				@param _Animspeed Animation speed
				@param _Timemulti Timemulti
				@param _GPU_Strength GPU_Strength
				@param _GPU_LODParameters GPU_LODParameters
			 */
			Options(const int   &_Octaves,
					const float &_Scale,
					const float &_Falloff,
					const float &_Animspeed,
					const float &_Timemulti,
					const float &_GPU_Strength,
					const Ogre::Vector3 &_GPU_LODParameters)
				: Octaves(_Octaves)
				, Scale(_Scale)
				, Falloff(_Falloff)
				, Animspeed(_Animspeed)
				, Timemulti(_Timemulti)
				, GPU_Strength(_GPU_Strength)
				, GPU_LODParameters(_GPU_LODParameters)
			{
			}
		};

		/** Default constructor
		 */
		HydrOCLPerlin();

		/** Constructor
		    @param Options HydrOCLPerlin noise options
		 */
		HydrOCLPerlin(const Options &Options);

		/** Destructor
		 */
		~HydrOCLPerlin();

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

		/** Save config
		    @param Data String reference
		 */
		void saveCfg(Ogre::String &Data);

		/** Load config
		    @param CgfFile Ogre::ConfigFile reference
			@return True if is the correct noise config
		 */
		bool loadCfg(Ogre::ConfigFile &CfgFile);

		/** Get the especified x/y noise value
		    @param x X Coord
			@param y Y Coord
			@return Noise value
			@remarks range [~-0.2, ~0.2]
		 */
		float getValue(const float &x, const float &y);

		/** Set vertexes heights using OpenCL stuff.
		    @param v Vertexes array.
		    @param N Number of vertexes at each direction.
			@param world Rendering camera position.
			@return true if sucessful.
		 */
		bool setHeight(cl_mem v, cl_uint2 N, Ogre::Vector3 world);

		/** Set/Update perlin noise options
		    @param Options HydrOCLPerlin noise options
			@remarks If create() have been already called, Octaves option doesn't be updated.
		 */
		void setOptions(const Options &Options);

		/** Get current HydrOCLPerlin noise options
		    @return Current perlin noise options
		 */
		inline const Options& getOptions() const
		{
			return mOptions;
		}

        /** Sets the OpenCL stuff.
         * @param n Number of devices available.
         * @param context OpenCL context
         * @param devices Devices array.
         * @param comQueue Commands queues array.
         * @note This object will not modify or destroy
         * OpenCL stuff, do it externally.
         * @return true if OpenCL is ready to work, false if errors
         * found (i.e.- Compiling kernels).
         */
        bool setupOpenCL(cl_uint n, cl_context context, cl_device_id *devices, cl_command_queue *comQueue);

    protected:
        /// Number of devices
        cl_uint mNumberOfDevices;
        /// Array of devices
        cl_device_id *mDevices;
        /// OpenCL context
        cl_context mContext;
        /// OpenCL context
        cl_command_queue *mComQueue;

	private:
		/** Initialize noise
		 */
		void _initNoise();

		/** Calcule noise
		 */
		void _calculeNoise();

		/** Read texel linear dual
		    @param u u
			@param v v
			@param o Octave
			@return int
		 */
	    int _readTexelLinearDual(const int &u, const int &v, const int &o);

		/** Read texel linear
		    @param u u
			@param v v
			@return Heigth
		 */
		float _getHeigthDual(float u, float v);

		/** Map sample
		    @param u u
			@param v v
			@param level Level
			@param octave Octave
			@return Map sample
		 */
		int _mapSample(const int &u, const int &v, const int &upsamplepower, const int &octave);

		/// HydrOCLPerlin noise variables
		int noise[n_size_sq*noise_frames];
		int o_noise[n_size_sq*max_octaves];
		int p_noise[np_size_sq*(max_octaves>>(n_packsize-1))];
		int *r_noise;
		float magnitude;

		/// Elapsed time
		double time;

		/// HydrOCLPerlin noise options
		Options mOptions;

		/// OpenCL noise storage
		cl_mem clNoise;
        /// OpenCL kernel.
        cl_kernel kHeight;

	};
}}  // namespace

#endif // HYDROCLPERLIN_H_INCLUDED
