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

#include <hydrocl/HydrOCLUtils.h>

unsigned int roundUp(unsigned int n, unsigned int divisor)
{
    unsigned int N = n;
    unsigned int Rest = N%divisor;
    if(Rest) {
        N -= Rest;
        N += divisor;
    }
    return N;
}

size_t readFile(char* SourceCode, const char* FileName)
{
    size_t Length = 0;
    FILE *File = NULL;

    //! 1st.- Open the file
    File = fopen(FileName, "rb");
    if(File == NULL) {
        return 0;
    }
    //! 2nd.- Gets the file length
    fseek(File, 0, SEEK_END);
    Length = ftell(File);
    fseek(File, 0, SEEK_SET);
    if(Length == 0) {
        fclose(File);
        return 0;
    }
    //! 3rd.- Reads content
    if(!SourceCode) {
        fclose(File);
        return Length;
    }
    size_t readed = fread(SourceCode, Length, 1, File);
    SourceCode[Length] = '\0';

    fclose(File);
    return Length;
}

const char* fileFromResources(const char* fileName)
{
    if(!Ogre::ResourceGroupManager::getSingleton().resourceExists(HYDRAX_RESOURCE_GROUP, fileName))
        return NULL;
    Ogre::ResourceGroupManager::LocationList locs = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(HYDRAX_RESOURCE_GROUP);
    Ogre::ResourceGroupManager::LocationList::iterator loc;
    for(loc=locs.begin();loc!=locs.end();++loc){
        Ogre::ResourceGroupManager::ResourceLocation *kk = *loc;
        Ogre::String path = kk->archive->getName() + "/" + fileName;
        if(readFile(NULL, path.c_str()) <= 0)
            continue;
        static char out[512]; strcpy(out, path.c_str());
        return out;
    }
    return NULL;
}

cl_kernel loadKernelFromFile(cl_context clContext, cl_device_id clDevice,
                          const char* path, const char* entryPoint, const char* flags)
{
    char* clSource = NULL;
    char* clFlags = NULL;
    size_t clSourceLength;
    int clFlag;
    cl_program program = 0;
    cl_kernel kernel = 0;

    HydraxLOG(Ogre::String("Loading ") + path + "::" + entryPoint + "...");
    //! Get source code
    clSourceLength = readFile(NULL, path);
    if(clSourceLength <= 0){
        HydraxLOG("Can't read source file.");
        return 0;
    }
    clSource = new char[clSourceLength+1];
    if(!clSource) {
        HydraxLOG("Can't allocate memory on host for the source code.");
        return 0;
    }
    clSourceLength = readFile(clSource, path);
    if(clSourceLength <= 0){
        HydraxLOG("Can't read source file.");
        delete[] clSource; clSource=0;
        return 0;
    }
    //! Compile program
    program = clCreateProgramWithSource(clContext, 1, (const char **)&clSource, &clSourceLength, &clFlag);
    if(clFlag != CL_SUCCESS) {
        HydraxLOG("Can't create OpenCL program.");
        delete[] clSource; clSource=0;
        return 0;
    }
    clFlags = new char[512]; strcpy(clFlags, "");
    strcpy(clFlags, "-cl-mad-enable -cl-no-signed-zeros -cl-finite-math-only -cl-fast-relaxed-math ");
    strcat(clFlags, flags);
    clFlag = clBuildProgram(program, 0, NULL, clFlags, NULL, NULL);
    if(clFlag != CL_SUCCESS) {
        HydraxLOG("--- Build log ---------------------------------");
        char Log[10240];
        clGetProgramBuildInfo(program, clDevice, CL_PROGRAM_BUILD_LOG, 10240*sizeof(char), Log, NULL );
        HydraxLOG(Log);
        HydraxLOG("--------------------------------- Build log ---");
        delete[] clSource; clSource=0;
        delete[] clFlags; clFlags=0;
        return 0;
    }
    char Log[10240];
    clGetProgramBuildInfo(program, clDevice, CL_PROGRAM_BUILD_LOG, 10240*sizeof(char), Log, NULL );
    if(strcmp(Log, "") && (strcmp(Log, "\n")) && strcmp(Log, "\EOF")){
        HydraxLOG("--- Build log ---------------------------------");
        HydraxLOG(Log);
        HydraxLOG("--------------------------------- Build log ---");
    }

    kernel = clCreateKernel(program, entryPoint, &clFlag);
    if(clFlag != CL_SUCCESS) {
        HydraxLOG("Can't create the kernel.");
        if(clFlag == CL_OUT_OF_HOST_MEMORY) {
            HydraxLOG("\tNot enought kernel resources.");
        }
        else if(clFlag == CL_INVALID_KERNEL_NAME) {
            HydraxLOG(Ogre::String("\tCan't find ") + entryPoint + " function.");
        }
        else if(clFlag == CL_INVALID_KERNEL_DEFINITION) {
            HydraxLOG(Ogre::String("\tInvalid function: ") + entryPoint + ". Did you forgive __kernel modifier?");
        }
        if(program)clReleaseProgram(program); program=0;
        delete[] clSource; clSource=0;
        delete[] clFlags; clFlags=0;
        return 0;
    }
    if(program)clReleaseProgram(program); program=0;
    delete[] clSource; clSource=0;
    delete[] clFlags; clFlags=0;
    return kernel;
}

int sendArgument(cl_kernel kernel, int index, size_t size, void* ptr)
{
    int clFlag;
    clFlag = clSetKernelArg(kernel, index, size, ptr);
    if(clFlag != CL_SUCCESS) {
        HydraxLOG("Can't send argument to kernel.");
        if(clFlag == CL_INVALID_KERNEL) {
            HydraxLOG("\tInvalid kernel.");
        }
        else if(clFlag == CL_INVALID_ARG_INDEX) {
            HydraxLOG("\tInvalid argument index.");
        }
        else if(clFlag == CL_INVALID_ARG_VALUE) {
            HydraxLOG("\tInvalid argument value.");
        }
        else if(clFlag == CL_INVALID_MEM_OBJECT) {
            HydraxLOG("\tcl_mem mismatch fail.");
        }
        else if(clFlag == CL_INVALID_SAMPLER) {
            HydraxLOG("\tcl_sampler mismatch fail.");
        }
        else if(clFlag == CL_INVALID_ARG_SIZE) {
            HydraxLOG("\tArgument type doesn't match.");
        }
        return 1;
    }
    return 0;
}

bool getData(cl_command_queue Queue, void *Dest, cl_mem Orig, size_t Size)
{
    cl_int clFlag;
    clFlag  = clEnqueueReadBuffer(Queue, Orig, CL_TRUE, 0, Size, Dest, 0, NULL, NULL);
    if(clFlag != CL_SUCCESS) {
        HydraxLOG("Failure retrieving memory from server.");
        if(clFlag == CL_INVALID_COMMAND_QUEUE){
            HydraxLOG("\tInvalid command queue.");
        }
        else if(clFlag == CL_INVALID_CONTEXT){
            HydraxLOG("\tInvalid context.");
        }
        else if(clFlag == CL_INVALID_MEM_OBJECT){
            HydraxLOG("\tInvalid buffer object.");
        }
        else if(clFlag == CL_INVALID_VALUE){
            if(Dest == NULL){
                HydraxLOG("\tMemory address to write is a NULL pointer");
            }
            else{
                HydraxLOG("\tUnreadable region (Probably Size is out of bounds).");
            }
        }
        else if(clFlag == CL_INVALID_EVENT_WAIT_LIST){
            HydraxLOG("\tUnhandled event wait list error.");
        }
        else if(clFlag == CL_MEM_OBJECT_ALLOCATION_FAILURE){
            HydraxLOG("\tFailure to allocate memory for data store associated with buffer.");
        }
        else if(clFlag == CL_OUT_OF_HOST_MEMORY){
            HydraxLOG("\tFailure to allocate resources required by the OpenCL implementation on the host.");
        }
        else{
            HydraxLOG("\tUnhandled failure.");
        }
        return true;
    }
    return false;
}

bool sendData(cl_command_queue Queue, cl_mem Dest, void* Orig, size_t Size)
{
    cl_int clFlag;
    clFlag  = clEnqueueWriteBuffer(Queue, Dest, CL_TRUE, 0, Size, Orig, 0, NULL, NULL);
    if(clFlag != CL_SUCCESS) {
        HydraxLOG("Failure sending memory to server.");
        if(clFlag == CL_INVALID_COMMAND_QUEUE){
            HydraxLOG("\tInvalid command queue.");
        }
        else if(clFlag == CL_INVALID_CONTEXT){
            HydraxLOG("\tInvalid context.");
        }
        else if(clFlag == CL_INVALID_MEM_OBJECT){
            HydraxLOG("\tInvalid buffer object.");
        }
        else if(clFlag == CL_INVALID_VALUE){
            HydraxLOG("\tUnreadable region or invalid memory addres to write.");
        }
        else if(clFlag == CL_INVALID_EVENT_WAIT_LIST){
            HydraxLOG("\tUnhandled event wait list error.");
        }
        else if(clFlag == CL_MEM_OBJECT_ALLOCATION_FAILURE){
            HydraxLOG("\tFailure to allocate memory for data store associated with buffer.");
        }
        else if(clFlag == CL_OUT_OF_HOST_MEMORY){
            HydraxLOG("\tFailure to allocate resources required by the OpenCL implementation on the host.");
        }
        else{
            HydraxLOG("\tUnhandled failure.");
        }
        return true;
    }
    return false;
}
