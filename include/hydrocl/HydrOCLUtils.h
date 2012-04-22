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

#ifndef HYDROCLUTILS_H_INCLUDED
#define HYDROCLUTILS_H_INCLUDED

// ----------------------------------------------------------------------------
// Hydrax plugin
// ----------------------------------------------------------------------------
#include <Hydrax/Prerequisites.h>
#include <Hydrax/Hydrax.h>

// ----------------------------------------------------------------------------
// OpenCL libraries
// ----------------------------------------------------------------------------
#include <CL/cl.h>

/** Method that returns the next number to n that is divisible by divisor.
 * @param n Number to rounded up.
 * @param divisor Divisor.
 * @return Rounded up number.
 */
unsigned int roundUp(unsigned int n, unsigned int divisor);

/** Resource file path. Looks for into resources manager specified file
 * and returns the location.
 * @param fileName File name.
 * @return file path, NULL if can't be find.
 */
const char* fileFromResources(const char* fileName);

/** Loads an OpenCL kernel.
 * @param clContext Context where the program must loaded.
 * @param clDevide Device that must use the kernel.
 * @param path Path of the kernel file.
 * @param entryPoint Method into the kernel that must be called.
 * @param flags Preprocessor flags.
 * @return Loaded kernel, 0 if can't be loaded.
 */
cl_kernel loadKernelFromFile(cl_context clContext, cl_device_id clDevice,
                          const char* path, const char* entryPoint, const char* flags);

/** Method that sends an argument to OpenCL kernel.
 * @param kernel Kernel that must receive the argument.
 * @param index Index of the argument into the kernel.
 * @param size Memory size of the argument.
 * @param ptr Pointer to the argument.
 * @return 0 if all gone right, 1 otherwise.
 */
int sendArgument(cl_kernel kernel, int index, size_t size, void* ptr);

/** Recover data from device.
 * @param Queue Command queue.
 * @param Dest Host allocated memory.
 * @param Orig Device allocated memopry.
 * @param Size Data size to transfer.
 * @return true if sucessfully transfer.
 */
bool getData(cl_command_queue Queue, void *Dest, cl_mem Orig, size_t Size);

/** Send data to device.
 * @param Queue Command queue.
 * @param Dest Device allocated memory.
 * @param Orig Host allocated memory.
 * @param Size Data size to send.
 * @return true if sucessfully transfer.
 */
bool sendData(cl_command_queue Queue, cl_mem Dest, void* Orig, size_t Size);

#endif // HYDROCLUTILS_H_INCLUDED
