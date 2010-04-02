/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#ifndef _LUXRAYS_INTERSECTIONDEVICE_H
#define	_LUXRAYS_INTERSECTIONDEVICE_H

#include <string>
#include <cstdlib>

#include <boost/thread/thread.hpp>

#include "luxrays/luxrays.h"
#include "luxrays/core/utils.h"
#include "luxrays/core/dataset.h"
#include "luxrays/core/context.h"

namespace luxrays {

typedef enum {
	DEVICE_TYPE_ALL, DEVICE_TYPE_OPENCL, DEVICE_TYPE_NATIVE_THREAD, DEVICE_TYPE_VIRTUAL
} DeviceType;

class DeviceDescription {
public:
	DeviceDescription() { }
	DeviceDescription(const std::string deviceName, const DeviceType deviceType) :
		name(deviceName), type(deviceType) { }

	const std::string &GetName() const { return name; }
	const DeviceType GetType() const { return type; };

	static void FilterOne(std::vector<DeviceDescription *> &deviceDescriptions);
	static void Filter(DeviceType type, std::vector<DeviceDescription *> &deviceDescriptions);
	static std::string GetDeviceType(const DeviceType type);

protected:
	std::string name;
	DeviceType type;
};

class IntersectionDevice {
public:
	const std::string &GetName() const { return deviceName; }
	const Context *GetContext() const { return deviceContext; }
	const DeviceType GetType() const { return deviceType; }
	const DataSet *GetDataSet() const { return dataSet; }

	virtual bool IsRunning() const { return started; };

	virtual RayBuffer *NewRayBuffer() = 0;
	virtual void PushRayBuffer(RayBuffer *rayBuffer) = 0;
	virtual RayBuffer *PopRayBuffer() = 0;
	virtual size_t GetQueueSize() = 0;

	double GetPerformance() const {
		double statsTotalRayTime = WallClockTime() - statsStartTime;
		return (statsTotalRayTime == 0.0) ?	1.0 : (statsTotalRayCount / statsTotalRayTime);
	}
	virtual double GetLoad() const = 0;

	friend class Context;
	friend class VirtualM2OHardwareIntersectionDevice;
	friend class VirtualM2MHardwareIntersectionDevice;

protected:
	IntersectionDevice(const Context *context, const DeviceType type, const unsigned int index);
	virtual ~IntersectionDevice();

	virtual void SetDataSet(const DataSet *newDataSet);
	virtual void Start();
	virtual void Interrupt() = 0;
	virtual void Stop();

	const Context *deviceContext;
	DeviceType deviceType;
	unsigned int deviceIndex;

	std::string deviceName;
	const DataSet *dataSet;

	double statsStartTime, statsTotalRayCount, statsDeviceIdleTime, statsDeviceTotalTime;

	bool started;
};

class HardwareIntersectionDevice : public IntersectionDevice {
protected:
	HardwareIntersectionDevice(const Context *context, const DeviceType type, const unsigned int index) :
		IntersectionDevice(context, type, index) { }
	virtual ~HardwareIntersectionDevice() { }

	virtual void SetExternalRayBufferQueue(RayBufferQueue *queue) = 0;

	friend class VirtualM2OHardwareIntersectionDevice;
	friend class VirtualM2MHardwareIntersectionDevice;
};

//------------------------------------------------------------------------------
// Native thread devices
//------------------------------------------------------------------------------

class NativeThreadDeviceDescription : public DeviceDescription {
public:
	NativeThreadDeviceDescription(const std::string deviceName, const unsigned int threadIdx) :
		DeviceDescription(deviceName, DEVICE_TYPE_NATIVE_THREAD), threadIndex(threadIdx) { }

	size_t GetThreadIndex() const { return threadIndex; };

protected:
	size_t threadIndex;
};

class NativeThreadIntersectionDevice : public IntersectionDevice {
public:
	NativeThreadIntersectionDevice(const Context *context, const size_t threadIndex,
			const unsigned int devIndex);
	~NativeThreadIntersectionDevice();

	void SetDataSet(const DataSet *newDataSet);
	void Start();
	void Interrupt();
	void Stop();

	RayBuffer *NewRayBuffer();
	size_t GetQueueSize() { return 0; }
	void PushRayBuffer(RayBuffer *rayBuffer);
	RayBuffer *PopRayBuffer();

	double GetLoad() const {
		return 1.0;
	}

	static size_t RayBufferSize;

	friend class Context;

protected:
	static void AddDevices(std::vector<DeviceDescription *> &descriptions);

private:
	RayBufferSingleQueue doneRayBufferQueue;
};

//------------------------------------------------------------------------------
// OpenCL devices
//------------------------------------------------------------------------------

#define OPENCL_RAYBUFFER_SIZE 65536

#if !defined(LUXRAYS_DISABLE_OPENCL)

typedef enum {
	OCL_DEVICE_TYPE_ALL, OCL_DEVICE_TYPE_DEFAULT, OCL_DEVICE_TYPE_CPU,
			OCL_DEVICE_TYPE_GPU, OCL_DEVICE_TYPE_UNKNOWN
} OpenCLDeviceType;

class OpenCLDeviceDescription : public DeviceDescription {
public:
	OpenCLDeviceDescription(const std::string deviceName, const OpenCLDeviceType type,
		const size_t devIndex, const int deviceComputeUnits, const size_t deviceMaxMemory) :
		DeviceDescription(deviceName, DEVICE_TYPE_OPENCL), oclType(type), deviceIndex(devIndex),
		computeUnits(deviceComputeUnits), maxMemory(deviceMaxMemory), forceWorkGroupSize(0) { }

	OpenCLDeviceType GetOpenCLType() const { return oclType; }
	size_t GetDeviceIndex() const { return deviceIndex; }
	int GetComputeUnits() const { return computeUnits; }
	size_t GetMaxMemory() const { return maxMemory; }
	unsigned int GetForceWorkGroupSize() const { return forceWorkGroupSize; }

	void SetForceWorkGroupSize(const unsigned int size) const { forceWorkGroupSize = size; }

	static void Filter(const OpenCLDeviceType type, std::vector<DeviceDescription *> &deviceDescriptions);

protected:
	OpenCLDeviceType oclType;
	size_t deviceIndex;
	int computeUnits;
	size_t maxMemory;

	// The use of this field is not multi-thread safe (i.e. OpenCLDeviceDescription
	// is shared among all threads)
	mutable unsigned int forceWorkGroupSize;
};

class OpenCLIntersectionDevice : public HardwareIntersectionDevice {
public:
	OpenCLIntersectionDevice(const Context *context, const cl::Device &device,
			const unsigned int index, const unsigned int forceWorkGroupSize);
	~OpenCLIntersectionDevice();

	void SetDataSet(const DataSet *newDataSet);
	void Start();
	void Interrupt();
	void Stop();

	RayBuffer *NewRayBuffer();
	size_t GetQueueSize() { return rayBufferQueue.GetSizeToDo(); }
	void PushRayBuffer(RayBuffer *rayBuffer);
	RayBuffer *PopRayBuffer();

	OpenCLDeviceType GetOpenCLType() const { return oclType; }

	double GetLoad() const {
		return (statsDeviceTotalTime == 0.0) ? 0.0 : (1.0 - statsDeviceIdleTime / statsDeviceTotalTime);
	}

	friend class Context;

	static size_t RayBufferSize;

protected:
	void SetExternalRayBufferQueue(RayBufferQueue *queue);

	static std::string GetDeviceType(const cl_int type);
	static std::string GetDeviceType(const OpenCLDeviceType type);
	static OpenCLDeviceType GetOCLDeviceType(const cl_int type);
	static void AddDevices(const cl::Platform &oclPlatform, const OpenCLDeviceType filter,
		std::vector<DeviceDescription *> &descriptions);

private:
	static void IntersectionThread(OpenCLIntersectionDevice *renderDevice);

	void TraceRayBuffer(RayBuffer *rayBuffer, cl::Event *event);

	OpenCLDeviceType oclType;
	boost::thread *intersectionThread;

	// OpenCL items
	cl::Context *oclContext;
	cl::CommandQueue *oclQueue;

	// BVH fields
	cl::Kernel *bvhKernel;
	size_t bvhWorkGroupSize;
	cl::Buffer *vertsBuff;
	cl::Buffer *trisBuff;
	cl::Buffer *bvhBuff;

	// QBVH fields
	cl::Kernel *qbvhKernel;
	size_t qbvhWorkGroupSize;
	cl::Buffer *qbvhBuff;
	cl::Buffer *qbvhTrisBuff;

	cl::Buffer *raysBuff;
	cl::Buffer *hitsBuff;

	RayBufferQueueO2O rayBufferQueue;
	RayBufferQueue *externalRayBufferQueue;

	bool reportedPermissionError;
};

#endif

}

#endif	/* _LUXRAYS_INTERSECTIONDEVICE_H */
