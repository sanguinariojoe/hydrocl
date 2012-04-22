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

#ifndef DUMMYNOISE_H_INCLUDED
#define DUMMYNOISE_H_INCLUDED

// ----------------------------------------------------------------------------
// Hydrax plugin
// ----------------------------------------------------------------------------
#include <Hydrax/Prerequisites.h>
#include <Hydrax/Noise/Noise.h>

namespace Hydrax{ namespace Noise
{
	/** Dummy noise module class. Use it as empty noise module
	 */
	class DllExport Dummy : public Noise
	{
	public:
		/** Default constructor
		 */
		Dummy() : Noise("None", false){}

		/** Destructor
		 */
		~Dummy();

		/** Call it each frame
		    @param timeSinceLastFrame Time since last frame(delta)
		 */
		void update(const Ogre::Real &timeSinceLastFrame){}

		/** Get the especified x/y noise value
		    @param x X Coord
			@param y Y Coord
			@return Noise value
			@remarks range [~-0.2, ~0.2]
		 */
		float getValue(const float &x, const float &y){return 0.f;}
	};
}}  // namespace

#endif // DUMMYNOISE_H_INCLUDED
