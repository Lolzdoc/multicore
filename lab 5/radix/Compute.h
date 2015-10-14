
#ifndef Compute_h
#define Compute_h

#ifdef _WIN32
#include <Windows.h>
#include <CL/OpenCL.h>
#include <gl/GL.h>
#elif defined(__APPLE__)
#include <OpenCL/OpenCL.h>
#include <OpenGL/OpenGL.h>
#else
#include <CL/opencl.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <string.h>
#endif

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <assert.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#endif

class ComputeContext {
private:
	cl_context context;
	
protected:
	ComputeContext(cl_context context) : context(context) {
		if (!context)
			throw std::runtime_error("opencl: unable to create context::error ");
	}
	
public:
	inline operator cl_context () const {
		return context;
	}
	
	cl_device_id getDevice(std::string vendor) const {
		cl_device_id devices[8];
		size_t deviceCount = 0;
		
		clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(devices), devices, &deviceCount);
		deviceCount /= sizeof(cl_device_id);
		//printf("Device count: %d\n", (int)deviceCount);
		if (!deviceCount)
			throw std::runtime_error("opencl: unable to query devices");
		unsigned index;
		for (index = 0; index < deviceCount; index++) {
			char vendors[1024];
			clGetDeviceInfo(devices[index], CL_DEVICE_VENDOR, 1024, vendors, NULL);
			if (strncmp(vendor.c_str(), vendors, vendor.size()) == 0) {
				//printf("vendor: %s\n",vendors);
				break;
			}
		}
		if (index == deviceCount)
			throw std::runtime_error("opencl: unable to query device '" + vendor + "'");
		return devices[index];
	}
	
	cl_device_id getFirstDevice() const {
		cl_device_id devices[4];
		size_t deviceCount = 0;
		
		clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(devices), devices, &deviceCount);
		deviceCount /= sizeof(cl_device_id);
		//printf("Device count: %d\n", (int)deviceCount);
		
		for (int i = 0; i < deviceCount; i++) {
			char string[1024];
			clGetDeviceInfo(devices[i], CL_DEVICE_VENDOR, 1024, string, NULL);
			//printf("Device %d, vendor: %s\n", i, string);
		}
		if (!deviceCount)
			throw std::runtime_error("opencl: unable to query devices");
		
		return devices[0];
	}
	
	virtual ~ComputeContext() {
		clReleaseContext(context);
	}
};

class ComputeGpuContext : public ComputeContext {
public:
	cl_int error;
	
	ComputeGpuContext() : ComputeContext(clCreateContextFromType(NULL, CL_DEVICE_TYPE_GPU, NULL, NULL, &error)) {
	}
	private:
	cl_context_properties mProps[3];
	
	cl_context_properties* getProperties() {
		mProps[0] = (cl_context_properties)CL_CONTEXT_PLATFORM;  // indicates that next element is platform
		mProps[1] = (cl_context_properties)getPlatformID();
		mProps[2] = (cl_context_properties)0;
		return mProps;
	}

	static cl_platform_id getPlatformID() {
		cl_uint platformCount;
		cl_int result = clGetPlatformIDs(0, NULL, &platformCount);
		assert(result == CL_SUCCESS);
		assert(platformCount > 0);
		
		cl_platform_id* platforms = (cl_platform_id*)malloc(platformCount * sizeof(cl_platform_id));
		clGetPlatformIDs(platformCount, platforms, NULL);
		
		cl_platform_id selectedPlatform = 0;
		
		for (cl_uint i = 0; i < platformCount; ++i) {
			char string[1024];
			result = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 1024, &string, NULL);
			if (result == CL_SUCCESS) {
				if (strstr(string, "NVIDIA") != 0) {
					selectedPlatform = platforms[i];
					break;
				}
			}
		}
		
		if (selectedPlatform == 0) {
			printf("WARNING: NVIDIA OpenCL platform not found - defaulting to first platform!\n");
			selectedPlatform = platforms[0];
		}
		
		free(platforms);
		
		char string[1024];
		result = clGetPlatformInfo(selectedPlatform, CL_PLATFORM_NAME, 1024, &string, NULL);
		assert(result == CL_SUCCESS);
		printf("%s\n", string);
		
		return selectedPlatform;
	}
};

class ComputeCpuContext : public ComputeContext {
public:
	cl_int error;
	
	ComputeCpuContext() : ComputeContext(createCPUContext()) {}
	//ComputeCpuContext() : ComputeContext(clCreateContextFromType(nullptr, CL_DEVICE_TYPE_CPU, nullptr, nullptr, nullptr)) {}
		
private:
	static cl_context createCPUContext() {
		cl_uint platformIdCount = 0;
		cl_int err = clGetPlatformIDs(0, nullptr, &platformIdCount);
		assert(err == CL_SUCCESS);

		std::vector<cl_platform_id> platformIds(platformIdCount);
		err = clGetPlatformIDs(platformIdCount, platformIds.data (), nullptr);
		assert(err == 0);

		cl_uint deviceIdCount = 0;
		err = clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_CPU, 0, nullptr, &deviceIdCount);
		assert(err == 0);
		
		std::vector<cl_device_id> deviceIds(deviceIdCount);
		
		clGetDeviceIDs(platformIds[0],
			CL_DEVICE_TYPE_CPU,
			deviceIdCount,
			deviceIds.data (),
			nullptr);

		const cl_context_properties contextProperties [] = 
		{
			CL_CONTEXT_PLATFORM,
			reinterpret_cast<cl_context_properties> (platformIds[0]),
			0, 0
		};
		cl_context context = clCreateContext (
			contextProperties, deviceIdCount,
			deviceIds.data(), nullptr,
			nullptr, &err);
		assert(err == 0);
		return context;	
	}
	

		//ComputeContext(clCreateContextFromType(getProperties(), CL_DEVICE_TYPE_CPU, NULL, NULL, &error);

private:
	cl_context_properties mProps[3];
	cl_context_properties* getProperties() {
		mProps[0] = (cl_context_properties)CL_CONTEXT_PLATFORM;  // indicates that next element is platform
		mProps[1] = (cl_context_properties)getPlatformID();
		mProps[2] = (cl_context_properties)0;
		return mProps;
	}
	
	static cl_platform_id getPlatformID() {
		cl_uint platformCount;
		cl_int result = clGetPlatformIDs(0, NULL, &platformCount);
		assert(result == CL_SUCCESS);
		assert(platformCount > 0);
		
		cl_platform_id* platforms = (cl_platform_id*)malloc(platformCount * sizeof(cl_platform_id));
		clGetPlatformIDs(platformCount, platforms, NULL);
		
		cl_platform_id selectedPlatform = 0;
		
		for (cl_uint i = 0; i < platformCount; ++i) {
			char string[1024];
			result = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 1024, &string, NULL);
			if (result == CL_SUCCESS) {
				if (strstr(string, "NVIDIA") != 0) {
					selectedPlatform = platforms[i];
					break;
				}
			}
		}
				
		free(platforms);
		
		char string[1024];
		result = clGetPlatformInfo(selectedPlatform, CL_PLATFORM_NAME, 1024, &string, NULL);
		assert(result == CL_SUCCESS);
		printf("%s\n", string);
		
		return selectedPlatform;
	}
};

class ComputeGpuContextWin32 : public ComputeContext {
public:
	ComputeGpuContextWin32() : ComputeContext(createGpuContext()) {
	}

private:
	static cl_platform_id getPlatformID() {
		cl_uint platformCount;
		cl_int result = clGetPlatformIDs(0, NULL, &platformCount);
		assert(result == CL_SUCCESS);
		assert(platformCount > 0);

		cl_platform_id* platforms = (cl_platform_id*)malloc(platformCount * sizeof(cl_platform_id));
		clGetPlatformIDs(platformCount, platforms, NULL);

		cl_platform_id selectedPlatform = 0;

		for (cl_uint i = 0; i < platformCount; ++i) {
			char string[1024];
			result = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 1024, &string, NULL);
			if (result == CL_SUCCESS) {
				if (strstr(string, "NVIDIA") != 0) {
					selectedPlatform = platforms[i];
					break;
				} else if (strstr(string, "Intel GPU") != 0) {
					selectedPlatform = platforms[i];
					break;
				}
			}
		}

		if (selectedPlatform == 0) {
			printf("WARNING: NVIDIA OpenCL platform not found - defaulting to first platform!\n");
			selectedPlatform = platforms[0];
		}

		free(platforms);

		char string[1024];
		result = clGetPlatformInfo(selectedPlatform, CL_PLATFORM_NAME, 1024, &string, NULL);
		assert(result == CL_SUCCESS);
		printf("%s\n", string);

		return selectedPlatform;
	}

	static cl_context createGpuContext() {
		cl_platform_id platform = getPlatformID();

		cl_uint devCount;
		cl_int result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &devCount);
		assert(result == CL_SUCCESS);

		cl_device_id* devices = new cl_device_id[devCount];
		result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, devCount, devices, NULL);
		assert(result == CL_SUCCESS);

		cl_context_properties props[] = {
			CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
			0
		};

		cl_context context = clCreateContext(props, 1, &devices[0], NULL, NULL, 0);
		delete[] devices;
		return context;
	}
};

class ComputeGlContext : public ComputeContext {
public:
	ComputeGlContext() : ComputeContext(createGlContext()) {
	}
	
private:
	cl_context_properties mProps[3];
	
	cl_context_properties* getProperties() {
		mProps[0] = (cl_context_properties)CL_CONTEXT_PLATFORM;  // indicates that next element is platform
		mProps[1] = (cl_context_properties)getPlatformID();
		mProps[2] = (cl_context_properties)0;
		return mProps;
	}

	static cl_platform_id getPlatformID() {
		cl_uint platformCount;
		cl_int result = clGetPlatformIDs(0, NULL, &platformCount);
		assert(result == CL_SUCCESS);
		assert(platformCount > 0);
		
		cl_platform_id* platforms = (cl_platform_id*)malloc(platformCount * sizeof(cl_platform_id));
		clGetPlatformIDs(platformCount, platforms, NULL);
		
		cl_platform_id selectedPlatform = 0;
		
		for (cl_uint i = 0; i < platformCount; ++i) {
			char string[1024];
			result = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 1024, &string, NULL);
			if (result == CL_SUCCESS) {
				if (strstr(string, "NVIDIA") != 0) {
					selectedPlatform = platforms[i];
					break;
				}
			}
		}
		
		if (selectedPlatform == 0) {
			printf("WARNING: NVIDIA OpenCL platform not found - defaulting to first platform!\n");
			selectedPlatform = platforms[0];
		}
		
		free(platforms);
		
		char string[1024];
		result = clGetPlatformInfo(selectedPlatform, CL_PLATFORM_NAME, 1024, &string, NULL);
		assert(result == CL_SUCCESS);
		printf("%s\n", string);
		
		return selectedPlatform;
	}
	
	static cl_context createGlContext() {
#ifdef __APPLE__
		CGLContextObj glContext = CGLGetCurrentContext();
		CGLShareGroupObj glShareGroup = CGLGetShareGroup(glContext);
		
		cl_context_properties properties[] = { CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)glShareGroup, 0 };
		return clCreateContext(properties, CL_DEVICE_TYPE_GPU, 0, 0, 0, 0);
#else
		cl_platform_id platform = getPlatformID();
		
		cl_uint devCount;
		cl_int result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &devCount);
		assert(result == CL_SUCCESS);
		
		cl_device_id* devices = new cl_device_id[devCount];
		result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, devCount, devices, NULL);
		assert(result == CL_SUCCESS);
		
#ifdef WIN32
		cl_context_properties props[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
			CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
			CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
			0
		};
#else
		cl_context_properties props[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
			CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
			CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
			0
		};
#endif
		
		cl_context context = clCreateContext(props, 1, &devices[0], NULL, NULL, 0);
		delete [] devices;
		return context;
#endif
	}
};

class ComputeQueue {
private:
	cl_command_queue queue;
	cl_device_id device;
	
public:
	ComputeQueue(ComputeContext& context) {
		device = context.getFirstDevice();
		queue = clCreateCommandQueue(context, context.getFirstDevice(), 0, NULL);	
		if (!queue)
			throw std::runtime_error("opencl: unable to create command queue");
	}
	
	ComputeQueue(ComputeContext& context, std::string sDevice) {
		device = context.getDevice(sDevice);
		queue = clCreateCommandQueue(context, device, 0, nullptr);
		if (!queue)
			throw std::runtime_error("opencl: unable to create command queue");
	}
	
	inline operator cl_command_queue () const {
		return queue;
	}
	
	void queryDeviceInfo(cl_device_info param_name) {
		
		char param_value[1024];
		size_t param_value_size = 1024*sizeof(char);
		size_t param_value_size_ret;
		cl_int err = clGetDeviceInfo(device, param_name, param_value_size, (void*)param_value, &param_value_size_ret);
		if (err != CL_SUCCESS)
			throw std::runtime_error("opencl: unable to query device info");
		//std::string msg(param_value, param_value_size_ret);
		std::cout << ((cl_ulong*)param_value)[0] << std::endl;
		//printf("%s\n",param_value);

	}
	
	void executeKernel(cl_kernel kernel, unsigned dimensions, const size_t* globalWorkSize, const size_t* localWorkSize) {
		int err = clEnqueueNDRangeKernel(queue, kernel, dimensions, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
		if (err != 0) {
			printf("error code: %d\n", err);
			throw std::runtime_error("opencl: unable to execute kernel");
		}
	}
	
	void writeBuffer(cl_mem buffer, unsigned offset, unsigned size, void* input) {
		if (clEnqueueWriteBuffer(queue, buffer, true, offset, size, input, 0, 0, 0))
			throw std::runtime_error("opencl: unable to write buffer");
	}
	
	void readBuffer(cl_mem buffer, unsigned offset, unsigned size, void* output) {
		if (clEnqueueReadBuffer(queue, buffer, true, offset, size, output, 0, 0, 0))
			throw std::runtime_error("opencl: unable to read buffer");
	}
	
	void* mapBuffer(cl_mem buffer, unsigned size) {
		return clEnqueueMapBuffer(queue, buffer, true, CL_MAP_READ | CL_MAP_WRITE, 0, size, 0, 0, 0, 0);
	}
	
	void unmapBuffer(cl_mem buffer, void* pointer) {
		clEnqueueUnmapMemObject(queue, buffer, pointer, 0, 0, 0);
	}
	
	void aquireGlObject(cl_mem buffer) {
		if (clEnqueueAcquireGLObjects(queue, 1, &buffer, 0, NULL, NULL))
			throw std::runtime_error("opencl: unable to aquire gl object");
	}
	
	void releaseGlObject(cl_mem buffer) {
		if (clEnqueueReleaseGLObjects(queue, 1, &buffer, 0, NULL, NULL))
			throw std::runtime_error("opencl: unable to release gl object");
	}
	
	void barrier() {
		if (clEnqueueBarrierWithWaitList(queue, 0, 0, 0))
			throw std::runtime_error("opencl: unable to create barrier");
	}
	
	void flush() {
		if (clFlush(queue))
			throw std::runtime_error("opencl: unable to flush");
	}
	
	void waitForCompletion() {
		if (clFinish(queue))
			throw std::runtime_error("opencl: unable to finish");
	}
	
	~ComputeQueue() {
		clReleaseCommandQueue(queue);
	}
};

class ComputeProgram {
private:
	ComputeContext& context;
	cl_program program;
	
public:
	ComputeProgram(ComputeContext& context, const std::string& source, const std::string& args) : context(context) {
		std::vector<std::string> sources;
		sources.push_back(source);
		createProgram(sources, args);
	}
	
	ComputeProgram(ComputeContext& context, const std::vector<std::string>& sources, const std::string& args) : context(context) {
		createProgram(sources, args);
	}
	
	inline operator cl_program () const {
		return program;
	}
	
	inline ComputeContext& getContext() const {
		return context;
	}
	
	static std::string sourceFromFile(const std::string& fileName) {
		std::ifstream input(fileName.c_str(), std::ifstream::in);
		
		if (!input)
			throw std::runtime_error("opencl: unable to open source file");
		
		std::string source;
		std::string line;
		
		while (std::getline(input, line)) {
			source.append(line);
			source.push_back('\n');
		}
		
		return source;
	}
	
	~ComputeProgram() {
		clReleaseProgram(program);
	}
	
private:
	void createProgram(const std::vector<std::string>& sources, const std::string& args) {
		const char** sourceArray = new const char*[sources.size()+1];
		
		for (size_t i = 0; i < sources.size(); ++i)
			sourceArray[i] = sources[i].c_str();
		
		program = clCreateProgramWithSource(context, (unsigned)sources.size(), sourceArray, NULL, NULL);
		
		delete [] sourceArray;
		
		if (!program)
			throw std::runtime_error("opencl: unable to create program");
	
#if 1
		std::string flags = "-cl-mad-enable -cl-auto-vectorize-disable";
#else
		std::string flags = "-cl-mad-enable";
#endif
		//std::string flags = "-cl-mad-enable -cl-unsafe-math-optimizations -cl-fast-relaxed-math";
		//std::string flags = "-cl-mad-enable";
		if (args.size())
			flags += " " + args;
		
		cl_int result = clBuildProgram(program, 0, NULL, flags.c_str(), NULL, NULL);
		
		if (result) {
			char log[10240] = "";
			clGetProgramBuildInfo(program, context.getFirstDevice(), CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
			
			std::string message("opencl: build failed with error:\n\n");
			message.append(log);
			
			throw std::runtime_error(message);
		}
		else {
			char log[10240] = "";
			clGetProgramBuildInfo(program, context.getFirstDevice(), CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
			printf("%s\n", log);
		}
	}
};

class ComputeKernel {
private:
	ComputeProgram& program;
	cl_kernel kernel;
	unsigned argumentCount;
	
public:
	ComputeKernel(ComputeProgram& program, const std::string& entryPoint) : program(program) {
		kernel = clCreateKernel(program, entryPoint.c_str(), NULL);
		
		if (!kernel)
			throw std::runtime_error("opencl: unable to create kernel");
		
		argumentCount = 0;
	}
	
	inline operator cl_kernel () const {
		return kernel;
	}
	
	void clearArguments() {
		argumentCount = 0;
	}
	
	void addArgument(cl_mem buffer) {
		clSetKernelArg(kernel, argumentCount++, sizeof(cl_mem), (void*)&buffer);
	}
	
	void addArgument(cl_int value) {
		clSetKernelArg(kernel, argumentCount++, sizeof(cl_int), (void*)&value);
	}
	
	void addArgument(cl_uint value) {
		clSetKernelArg(kernel, argumentCount++, sizeof(cl_uint), (void*)&value);
	}

	void addArgument(cl_float value) {
		clSetKernelArg(kernel, argumentCount++, sizeof(cl_float), (void*)&value);
	}
	
	void addLocalBufferArgument(unsigned size) {
		clSetKernelArg(kernel, argumentCount++, size, NULL);
	}
	
	void setArgument(unsigned index, cl_mem buffer) {
		clSetKernelArg(kernel, index, sizeof(cl_mem), (void*)&buffer);
	}
	
	void setArgument(unsigned index, cl_int value) {
		clSetKernelArg(kernel, index, sizeof(cl_int), (void*)&value);
	}

	void setArgument(unsigned index, cl_float value) {
		clSetKernelArg(kernel, index, sizeof(cl_float), (void*)&value);
	}
	
	void setArgument(unsigned index, cl_uint value) {
		clSetKernelArg(kernel, index, sizeof(cl_uint), (void*)&value);
	}
	
	void setLocalBufferArgument(unsigned index, unsigned size) {
		clSetKernelArg(kernel, index, size, NULL);
	}
	
	~ComputeKernel() {
		clReleaseKernel(kernel);
	}
};

class ComputeBuffer {
protected:
	cl_mem buffer;
	
protected:
	ComputeBuffer(cl_mem buffer) {
		if (!buffer)
			throw std::runtime_error("opencl: unable to create buffer");
		
		this->buffer = buffer;
	}
	
	virtual ~ComputeBuffer() {
		clReleaseMemObject(buffer);
	}
	
public:
	inline operator cl_mem () const {
		return buffer;
	}
};

class ComputeSharedBuffer : public ComputeBuffer {
private:
	void* data;
	void* mapped_ptr;
	size_t mSize;
	
public:
	ComputeSharedBuffer(cl_context context, unsigned size) : ComputeBuffer(createBuffer(context, size, data)) {
		mSize = size;
	}
	
	size_t getSize() const {
		return mSize;
	}
	
	void* getData() const {
		return data;
	}
	
	void* map(ComputeQueue* q, size_t size) {
		if (!size)
			size = mSize;
		mapped_ptr = clEnqueueMapBuffer(*q, buffer, CL_TRUE, CL_MEM_READ_WRITE, 0, size, 0, NULL, NULL, NULL);
		if (!mapped_ptr)
			throw std::runtime_error("opencl: unable to map buffer");
		return mapped_ptr;
	}
	
	void unmap(ComputeQueue* q) {
		clEnqueueUnmapMemObject(*q, buffer, mapped_ptr, 0, NULL, NULL);
	}
	
	~ComputeSharedBuffer() {
		_mm_free(data);
	}
	
private:
	static cl_mem createBuffer(cl_context context, unsigned size, void*& data) {
		size = (size + (4096-1)) & (~(4096-1));
		data = _mm_malloc(size, 4096);
		return clCreateBuffer(context, CL_MEM_USE_HOST_PTR, size, data, NULL);
	}
};

class ComputeInputOutputBuffer : public ComputeBuffer {
public:
	ComputeInputOutputBuffer(cl_context context, unsigned size) : ComputeBuffer(clCreateBuffer(context, CL_MEM_READ_WRITE, size, 0, NULL)) {
	}
	
	ComputeInputOutputBuffer(cl_context context, unsigned size, const void* data) : ComputeBuffer(clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, size, const_cast<void*>(data), NULL)) {
	}
};

class ComputeInputBuffer : public ComputeBuffer {
public:
	ComputeInputBuffer(cl_context context, unsigned size) : ComputeBuffer(clCreateBuffer(context, CL_MEM_READ_ONLY, size, 0, NULL)) {
	}
	
	ComputeInputBuffer(cl_context context, unsigned size, const void* data) : ComputeBuffer(clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size, const_cast<void*>(data), NULL)) {
	}
};

class ComputeOutputBuffer : public ComputeBuffer {
public:
	ComputeOutputBuffer(ComputeContext& context, unsigned size) : ComputeBuffer(clCreateBuffer(context, CL_MEM_WRITE_ONLY, size, NULL, NULL)) {
	}
	
	ComputeOutputBuffer(ComputeContext& context, unsigned size, const void* data) : ComputeBuffer(clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, size, const_cast<void*>(data), NULL)) {
	}
};

class ComputeImage1d : public ComputeBuffer {
private:
	ComputeInputBuffer* buffer;
	
public:
	ComputeImage1d(ComputeContext& context, unsigned size, const void* ptr, cl_channel_type channelType) : ComputeBuffer(createImage(buffer, context, size, ptr, channelType)) {
	}
	
	~ComputeImage1d() {
		delete buffer;
	}
	
private:
	static cl_mem createImage(ComputeInputBuffer*& buffer, ComputeContext& context, unsigned size, const void* ptr, cl_channel_type channelType) {
		buffer = new ComputeInputBuffer(context, size*sizeof(float)*4, ptr);
		
		cl_image_format fmt = {
			CL_RGBA,
			channelType,
		};
		
		cl_image_desc desc = {
			CL_MEM_OBJECT_IMAGE1D_BUFFER,
			size,
			0, 0, 0, 0, 0, 0, 0,
			(cl_mem)*buffer
		};
		
		cl_mem mem = clCreateImage(context, CL_MEM_READ_ONLY, &fmt, &desc, 0, 0);
		
		if (!mem)
			throw std::exception();
		
		return mem;
	}
};

class ComputeImage2d : public ComputeBuffer {
public:
	ComputeImage2d(ComputeContext& context, unsigned width, unsigned height, const void* ptr, cl_channel_type channelType) : ComputeBuffer(createImage(context, width, height, ptr, channelType)) {
	}
	
private:
	static cl_mem createImage(ComputeContext& context, unsigned width, unsigned height, const void* ptr, cl_channel_type channelType) {
		cl_image_format fmt = {
			CL_RGBA,
			channelType,
		};
		
		cl_image_desc desc = {
			CL_MEM_OBJECT_IMAGE2D,
			width,
			height,
			0,
			0,
			width*4*4
		};
		
		cl_mem mem = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &fmt, &desc, (void*)ptr, 0);
		
		if (!mem)
			throw std::exception();
		
		return mem;
	}
};

#endif
