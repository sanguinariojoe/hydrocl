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

#ifndef HYDROCLNOISE_H_INCLUDED
#define HYDROCLNOISE_H_INCLUDED

// ----------------------------------------------------------------------------
// Standar libraries
// ----------------------------------------------------------------------------
#include <deque>
#include <math.h>

// ----------------------------------------------------------------------------
// Hydrax plugin
// ----------------------------------------------------------------------------
#include <Hydrax/Prerequisites.h>
#include <Hydrax/Noise/Noise.h>

// ----------------------------------------------------------------------------
// OpenCL accelerated perlin version
// ----------------------------------------------------------------------------
#include <hydrocl/HydrOCLPerlin.h>

// ----------------------------------------------------------------------------
// OpenCL libraries
// ----------------------------------------------------------------------------
#include <CL/cl.h>

namespace Hydrax{ namespace Noise
{
	/** OpenCL accelerated perlin noise module class
	 */
	class DllExport HydrOCLNoise : public HydrOCLPerlin
	{
	public:
		/** Struct wich contains wave parameters.
		 */
		struct Wave
		{
            /// Direction (must be normalised)
            Ogre::Vector2 dir;
            /// Amplitude [m]
            float A;
            /// Period [s]
            float T;
            /// Phase [rad]
            float P;

			/** Constructor
             * @param dir Wave direction (x,z axes).
             * @param A Wave amplitude [m].
             * @param T Wave period [s].
             * @param P Wave phase [rad].
			 */
			Wave(const Ogre::Vector2 &_dir,
				 const float &_A,
				 const float &_T,
				 const float &_P)
				 : dir(_dir)
				 , A(_A)
				 , T(_T)
				 , P(_P)
			{
			}
		};

		/** Default constructor
		 */
		HydrOCLNoise();

		/** Constructor
		 * @param Options HydrOCLNoise noise options
		 */
		HydrOCLNoise(const HydrOCLPerlin::Options &Options);

		/** Destructor
		 */
		~HydrOCLNoise();

		/** Create
		 */
		void create();

		/** Remove
		 */
		void remove();

		/** Call it each frame
         * @param timeSinceLastFrame Time since last frame(delta)
		 */
		void update(const Ogre::Real &timeSinceLastFrame);

        /** Add wave.
         * @param w Wave to add.
         * @warning Don't try to add waves until Hydrax has been
         * created, OpenCL must be already built.
         */
        void wave(const HydrOCLNoise::Wave &w);
        /** Add wave.
         * @param w Wave to add.
         * @note This method simply redirects to wave method.
         * @warning Don't try to add waves until Hydrax has been
         * created, OpenCL must be already built.
         */
        void addWave(const HydrOCLNoise::Wave &w){wave(w);}
        /** Get a wave.
         * @param id Wave index.
         * @return Selected wave. Null if not exist (i.e.- id out of bounds).
         * @note Use this method to modify waves.
         * @warning Don't destroy returned object.
         */
        HydrOCLNoise::Wave* wave(unsigned int id);
        /** Remove a wave.
         * @param id Wave index.
         * @return true if wave has been deleted, false otherwise (i.e.- id
         * out of bounds).
         * @note Use this method to modify waves.
         */
        bool removeWave(unsigned int id);

		/** Get the especified x/y noise value
         * @param x X Coord
         * @param y Y Coord
         * @return Noise value
         * @remarks range [~-0.2, ~0.2]
		 */
		float getValue(const float &x, const float &y);

		/** Set vertexes heights using OpenCL stuff.
         * @param v Vertexes array.
         * @param N Number of vertexes at each direction.
         * @param world Rendering camera position.
         * @return true if sucessful.
		 */
		bool setHeight(cl_mem v, cl_uint2 N, Ogre::Vector3 world);

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

	private:
        /** Reallocate memory for objects
         */
        bool reallocate();
        /** Sends data to device
         */
        bool send();
        /** Test if waves has been modified.
         * @return true If waves has been modified, false otherwise.
         */
        bool isModified();

        /// Set of waves.
        std::deque<Wave*> mWaves;
        /// Elapsed time
        float mTime;
        /// Device allocated waves direction
        cl_mem mDir;
        /// Device allocated waves amplitude
        cl_mem mA;
        /// Device allocated waves period
        cl_mem mT;
        /// Device allocated waves phase
        cl_mem mP;
        /// Host allocated directions
	    cl_float2 *hDir;
        /// Host allocated amplitudes
	    cl_float  *hA;
        /// Host allocated periods
	    cl_float  *hT;
        /// Host allocated phases
	    cl_float  *hP;
        /// OpenCL kernel.
        cl_kernel kWaves;

	};
}}  // namespace

#endif // HYDROCLNOISE_H_INCLUDED
