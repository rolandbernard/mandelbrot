/*  mandelbrot.cpp
 * Name Mandelbrot-Modul
 * Modul zum berechnen der Mandelbrot-Menge mithilfe von OpenCL
 * Autor: Roland Bernard
 * Lizenz: (C) Copyright 2018 by Roland Bernard. All rights reserved.
 * */

#include "mandelbrot.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#define DEBUG

#ifdef DEBUG
// Helper für die Debug-ausgabe
#define caseHelper(x, y) case x: fprintf(stderr, y); break;
#endif

// Maximale Länge der Datei mandelbrot.cl und der Fehlerberichte
#define MAX_STRING_SIZE 65536

// Funktion überprüft ob ein Fehler forliegt
void error(cl_int res, const char* err)
{
    if(res != CL_SUCCESS)
    {
#ifdef DEBUG
        fprintf(stderr, "%s (", err);
        switch(res)
        {
        caseHelper(CL_INVALID_BUFFER_SIZE, "size is 0.")
        caseHelper(CL_INVALID_HOST_PTR, "host_ptr is NULL and CL_MEM_USE_HOST_PTR or CL_MEM_COPY_HOST_PTR are set in flags or if host_ptr is not NULL but CL_MEM_COPY_HOST_PTR or CL_MEM_USE_HOST_PTR are not set in flags.")
        caseHelper(CL_MEM_OBJECT_ALLOCATION_FAILURE, "there is a failure to allocate memory for buffer object.")
        caseHelper(CL_INVALID_CONTEXT, "context is not a valid context.")
        caseHelper(CL_INVALID_DEVICE, "device is not a valid device or is not associated with context.")
        caseHelper(CL_INVALID_VALUE, "values specified in properties are not valid.")
        caseHelper(CL_INVALID_QUEUE_PROPERTIES, "values specified in properties are valid but are not supported by the device.")
        caseHelper(CL_OUT_OF_RESOURCES, "there is a failure to allocate resources required by the OpenCL implementation on the device.")
        caseHelper(CL_OUT_OF_HOST_MEMORY, "there is a failure to allocate resources required by the OpenCL implementation on the host.")
        }
        fprintf(stderr, ")\n");
#endif
        exit(1);
    }
}

mandelbrot::mandelbrot()
{
    cl_int res;

    // String für mögliche Buildfehler
    char *info = (char*)malloc(MAX_STRING_SIZE);

    FILE *fp;
    char fileName[] = "./kernel/mandelbrot.cl";
    char *source_str;
    size_t source_size;

    /* Laden der Kernel*/
    fp = fopen(fileName, "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_STRING_SIZE);
    source_size = fread(source_str, 1, MAX_STRING_SIZE, fp);
    fclose(fp);

    cl_uint num_platforms;

    // Abrufen der Platform
    res = clGetPlatformIDs(1, &_platform_id, &num_platforms);
    error(res, "Failed to get PlatformID.");

    cl_uint num_devices;

    // Abrufen des Devices
    res = clGetDeviceIDs(_platform_id, CL_DEVICE_TYPE_ALL, 1, &_device_id, &num_devices);
    error(res, "Failed to get DeviceID.");

    // Laden von Command Queue Eigenschaften
    cl_command_queue_properties queueProp;
    clGetDeviceInfo(_device_id, CL_DEVICE_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &queueProp, NULL);

    // Erstellen des Centexts
    _context = clCreateContext(NULL, 1, &_device_id, NULL, NULL, &res);
    error(res, "Failed to create OpenCL context.");

    // Erstellen des Command Queues
    _command_queue = clCreateCommandQueue(_context, _device_id, queueProp, &res);
    error(res, "Failed to create Command Queue.");

    // Erstellen des Programmes
    _program = clCreateProgramWithSource(_context, 1, (const char **)&source_str,
    (const size_t *)&source_size, &res);
    error(res, "Failed to create Program.");

    // Compilieren des Programmes
    res = clBuildProgram(_program, 1, &_device_id, NULL, NULL, NULL);

    // Ausgabe möglicher Fehler
    if(res != CL_SUCCESS)
    {
        clGetProgramBuildInfo(_program, _device_id, CL_PROGRAM_BUILD_LOG, MAX_STRING_SIZE, info, NULL);
        fprintf(stderr, "Failed to build Program:\n%s\n", info);
        exit(1);
    }

    // Erstellen der Kernel
    _kernel = clCreateKernel(_program, "computeColors", &res);
    error(res, "Failed to create Kernal.");

    // Freigebe der Strings
    free(source_str);
    free(info);
}

mandelbrot::~mandelbrot()
{
    cl_int res;

    /* Finalization */
    res = clFlush(_command_queue);
    res = clFinish(_command_queue);
    res = clReleaseKernel(_kernel);
    res = clReleaseProgram(_program);
    res = clReleaseCommandQueue(_command_queue);
    res = clReleaseContext(_context);
}

void mandelbrot::listDevices()
{
     int i, j;
    char* value;
    size_t valueSize;
    cl_uint platformCount;
    cl_platform_id* platforms;
    cl_uint deviceCount;
    cl_device_id* devices;
    cl_uint maxComputeUnits;

    // get all platforms
    clGetPlatformIDs(0, NULL, &platformCount);
    platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformCount);
    clGetPlatformIDs(platformCount, platforms, NULL);

    for (i = 0; i < platformCount; i++) {

        // get all devices
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
        devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

        // for each device print critical attributes
        for (j = 0; j < deviceCount; j++) {

            // print device name
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, NULL, &valueSize);
            value = (char*) malloc(valueSize);
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, valueSize, value, NULL);
            printf("%d. Device: %s\n", j+1, value);
            free(value);

            // print hardware device version
            clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, 0, NULL, &valueSize);
            value = (char*) malloc(valueSize);
            clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, valueSize, value, NULL);
            printf(" %d.%d Hardware version: %s\n", j+1, 1, value);
            free(value);

            // print software driver version
            clGetDeviceInfo(devices[j], CL_DRIVER_VERSION, 0, NULL, &valueSize);
            value = (char*) malloc(valueSize);
            clGetDeviceInfo(devices[j], CL_DRIVER_VERSION, valueSize, value, NULL);
            printf(" %d.%d Software version: %s\n", j+1, 2, value);
            free(value);

            // print c version supported by compiler for device
            clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &valueSize);
            value = (char*) malloc(valueSize);
            clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, valueSize, value, NULL);
            printf(" %d.%d OpenCL C version: %s\n", j+1, 3, value);
            free(value);

            // print parallel compute units
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS,
                    sizeof(maxComputeUnits), &maxComputeUnits, NULL);
            printf(" %d.%d Parallel compute units: %d\n", j+1, 4, maxComputeUnits);

        }

        free(devices);

    }

    free(platforms);
}

void mandelbrot::createBuffer(mandelbrot::res resolution, void* ptr)
{
    cl_int res;
    // Erstellt den BUffer
    _image = clCreateBuffer(_context, CL_MEM_WRITE_ONLY, resolution.y*resolution.y*sizeof(cl_char4), nullptr, &res);
    error(res, "Failed to create Buffer.");
}

void mandelbrot::deleteBuffer()
{
    cl_int res;
    // Löscht den Buffer
    res = clReleaseMemObject(_image);
}

void mandelbrot::computeImage(mandelbrot::color* ret, mandelbrot::res resolution, mandelbrot::rect pos, size_t i, size_t samples)
{
    cl_int res;

    // Speicherung der Werte in OpenCL-Datentypen
    size_t size = resolution.x*resolution.y;
    cl_double2 delta;
    cl_double2 topLeft;
    cl_uint2 reso;
    cl_uint iter;
    cl_uint samp;

    delta.s[0] = (pos.br.x - pos.tl.x) / resolution.x;
    delta.s[1] = (pos.br.y - pos.tl.y) / resolution.y;

    topLeft.s[0] = pos.tl.x;
    topLeft.s[1] = pos.tl.y;

    reso.s[0] = resolution.x;
    reso.s[1] = resolution.y;

    iter = i;
    samp = samples;

    // Setzen der Kernel-Argumente
    res = clSetKernelArg(_kernel, 0, sizeof(cl_mem), (void*)&_image);
    error(res, "Failed to set Kernel Arguments.");
    res = clSetKernelArg(_kernel, 1, sizeof(cl_double2), (void*)&delta);
    error(res, "Failed to set Kernel Arguments.");
    res = clSetKernelArg(_kernel, 2, sizeof(cl_double2), (void*)&topLeft);
    error(res, "Failed to set Kernel Arguments.");
    res = clSetKernelArg(_kernel, 3, sizeof(cl_uint2), (void*)&reso);
    error(res, "Failed to set Kernel Arguments.");
    res = clSetKernelArg(_kernel, 4, sizeof(cl_uint), (void*)&iter);
    error(res, "Failed to set Kernel Arguments.");
    res = clSetKernelArg(_kernel, 5, sizeof(cl_uint), (void*)&samp);
    error(res, "Failed to set Kernel Arguments.");

    // Aufrufen der Kernel
    res = clEnqueueNDRangeKernel(_command_queue, _kernel, 1, NULL, &size, NULL, 0, NULL, NULL);
    error(res, "Failed to execute Kernel.");

    size_t origin[3] = {0};
    size_t region[3] = {3, size, 1};

    // Auslesen des Buffers
    res =  clEnqueueReadBufferRect (_command_queue, _image, CL_TRUE, origin, origin, region, sizeof(cl_uchar3), 0, sizeof(mandelbrot::color), 0, (void*)ret, 0, NULL, NULL);
    error(res, "Failed to read Buffer.");
}
