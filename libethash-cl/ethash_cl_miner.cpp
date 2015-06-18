/*
  This file is part of c-ethash.

  c-ethash is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  c-ethash is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file ethash_cl_miner.cpp
* @author Tim Hughes <tim@twistedfury.com>
* @date 2015
*/


#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <queue>
#include <random>
#include <vector>
#include <libethash/util.h>
#include <libethash/ethash.h>
#include <libethash/internal.h>
#include "ethash_cl_miner.h"
#include "ethash_cl_miner_kernel.h"

#define ETHASH_BYTES 32

// workaround lame platforms
#if !CL_VERSION_1_2
#define CL_MAP_WRITE_INVALIDATE_REGION CL_MAP_WRITE
#define CL_MEM_HOST_READ_ONLY 0
#endif

#undef min
#undef max

using namespace std;

// TODO: If at any point we can use libdevcore in here then we should switch to using a LogChannel
#define ETHCL_LOG(_contents) cout << "[OPENCL]:" << _contents << endl
// Types of OpenCL devices we are interested in
#define ETHCL_QUERIED_DEVICE_TYPES (CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR)

static void addDefinition(string& _source, char const* _id, unsigned _value)
{
	char buf[256];
	sprintf(buf, "#define %s %uu\n", _id, _value);
	_source.insert(_source.begin(), buf, buf + strlen(buf));
}

ethash_cl_miner::search_hook::~search_hook() {}

ethash_cl_miner::ethash_cl_miner()
:	m_opencl_1_1()
{
}

ethash_cl_miner::~ethash_cl_miner()
{
	finish();
}

string ethash_cl_miner::platform_info(unsigned _platformId, unsigned _deviceId)
{
	vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (platforms.empty())
	{
		ETHCL_LOG("No OpenCL platforms found.");
		return string();
	}

	// get GPU device of the selected platform
	unsigned platform_num = min<unsigned>(_platformId, platforms.size() - 1);
	vector<cl::Device> devices = getDevices(platforms, _platformId);
	if (devices.empty())
	{
		ETHCL_LOG("No OpenCL devices found.");
		return string();
	}

	// use selected default device
	unsigned device_num = min<unsigned>(_deviceId, devices.size() - 1);
	cl::Device& device = devices[device_num];
	string device_version = device.getInfo<CL_DEVICE_VERSION>();

	return "{ \"platform\": \"" + platforms[platform_num].getInfo<CL_PLATFORM_NAME>() + "\", \"device\": \"" + device.getInfo<CL_DEVICE_NAME>() + "\", \"version\": \"" + device_version + "\" }";
}

std::vector<cl::Device> ethash_cl_miner::getDevices(std::vector<cl::Platform> const& _platforms, unsigned _platformId)
{
	vector<cl::Device> devices;
	unsigned platform_num = min<unsigned>(_platformId, _platforms.size() - 1);
	_platforms[platform_num].getDevices(
		s_allowCPU ? CL_DEVICE_TYPE_ALL : ETHCL_QUERIED_DEVICE_TYPES,
		&devices
	);
	return devices;
}

unsigned ethash_cl_miner::getNumPlatforms()
{
	vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	return platforms.size();
}

unsigned ethash_cl_miner::getNumDevices(unsigned _platformId)
{
	vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (platforms.empty())
	{
		ETHCL_LOG("No OpenCL platforms found.");
		return 0;
	}

	vector<cl::Device> devices = getDevices(platforms, _platformId);
	if (devices.empty())
	{
		ETHCL_LOG("No OpenCL devices found.");
		return 0;
	}
	return devices.size();
}

bool ethash_cl_miner::configureGPU(
	bool _allowCPU,
	unsigned _extraGPUMemory,
	bool _forceSingleChunk,
	boost::optional<uint64_t> _currentBlock
)
{
	s_allowCPU = _allowCPU;
	s_forceSingleChunk = _forceSingleChunk;
	s_extraRequiredGPUMem = _extraGPUMemory;
	// by default let's only consider the DAG of the first epoch
	uint64_t dagSize = _currentBlock ? ethash_get_datasize(*_currentBlock) : 1073739904U;
	uint64_t requiredSize =  dagSize + _extraGPUMemory;
	return searchForAllDevices([&requiredSize](cl::Device const _device) -> bool
		{
			cl_ulong result;
			_device.getInfo(CL_DEVICE_GLOBAL_MEM_SIZE, &result);
			if (result >= requiredSize)
			{
				ETHCL_LOG(
					"Found suitable OpenCL device [" << _device.getInfo<CL_DEVICE_NAME>()
					<< "] with " << result << " bytes of GPU memory"
				);
				return true;
			}
			
			ETHCL_LOG(
				"OpenCL device " << _device.getInfo<CL_DEVICE_NAME>()
				<< " has insufficient GPU memory." << result <<
				" bytes of memory found < " << requiredSize << " bytes of memory required"
			);
			return false;
		}
	);
}

bool ethash_cl_miner::s_allowCPU = false;
bool ethash_cl_miner::s_forceSingleChunk = false;
unsigned ethash_cl_miner::s_extraRequiredGPUMem;

bool ethash_cl_miner::searchForAllDevices(function<bool(cl::Device const&)> _callback)
{
	vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (platforms.empty())
	{
		ETHCL_LOG("No OpenCL platforms found.");
		return false;
	}
	for (unsigned i = 0; i < platforms.size(); ++i)
		if (searchForAllDevices(i, _callback))
			return true;

	return false;
}

bool ethash_cl_miner::searchForAllDevices(unsigned _platformId, function<bool(cl::Device const&)> _callback)
{
	vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (_platformId >= platforms.size())
		return false;

	vector<cl::Device> devices = getDevices(platforms, _platformId);
	for (cl::Device const& device: devices)
		if (_callback(device))
			return true;
		
	return false;
}

void ethash_cl_miner::doForAllDevices(function<void(cl::Device const&)> _callback)
{
	vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (platforms.empty())
	{
		ETHCL_LOG("No OpenCL platforms found.");
		return;
	}
	for (unsigned i = 0; i < platforms.size(); ++i)
		doForAllDevices(i, _callback);
}

void ethash_cl_miner::doForAllDevices(unsigned _platformId, function<void(cl::Device const&)> _callback)
{
	vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (_platformId >= platforms.size())
		return;

	vector<cl::Device> devices = getDevices(platforms, _platformId);
	for (cl::Device const& device: devices)
		_callback(device);
}

void ethash_cl_miner::listDevices()
{
	string outString ="\nListing OpenCL devices.\nFORMAT: [deviceID] deviceName\n";
	unsigned int i = 0;
	doForAllDevices([&outString, &i](cl::Device const _device)
		{
			outString += "[" + to_string(i) + "] " + _device.getInfo<CL_DEVICE_NAME>() + "\n";
			++i;
		}
	);
	ETHCL_LOG(outString);
}

void ethash_cl_miner::finish()
{
	if (m_queue())
		m_queue.finish();
}

bool ethash_cl_miner::init(
	uint8_t const* _dag,
	uint64_t _dagSize,
	unsigned workgroup_size,
	unsigned _platformId,
	unsigned _deviceId
)
{
	// get all platforms
	try
	{
		vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);
		if (platforms.empty())
		{
			ETHCL_LOG("No OpenCL platforms found.");
			return false;
		}

		// use selected platform
		_platformId = min<unsigned>(_platformId, platforms.size() - 1);
		ETHCL_LOG("Using platform: " << platforms[_platformId].getInfo<CL_PLATFORM_NAME>().c_str());

		// get GPU device of the default platform
		vector<cl::Device> devices = getDevices(platforms, _platformId);
		if (devices.empty())
		{
			ETHCL_LOG("No OpenCL devices found.");
			return false;
		}

		// use selected device
		cl::Device& device = devices[min<unsigned>(_deviceId, devices.size() - 1)];
		string device_version = device.getInfo<CL_DEVICE_VERSION>();
		ETHCL_LOG("Using device: " << device.getInfo<CL_DEVICE_NAME>().c_str() << "(" << device_version.c_str() << ")");

		// configure chunk number depending on max allocateable memory
		cl_ulong result;
		device.getInfo(CL_DEVICE_MAX_MEM_ALLOC_SIZE, &result);
		if (s_forceSingleChunk || result >= _dagSize)
		{
			m_dagChunksNum = 1;
			ETHCL_LOG(
				((result <= _dagSize && s_forceSingleChunk) ? "Forcing single chunk. Good luck!\n" : "") <<
				"Using 1 big chunk. Max OpenCL allocateable memory is " << result
			);
		}
		else
		{
			m_dagChunksNum = 4;
			ETHCL_LOG("Using 4 chunks. Max OpenCL allocateable memory is " << result);
		}

		if (strncmp("OpenCL 1.0", device_version.c_str(), 10) == 0)
		{
			ETHCL_LOG("OpenCL 1.0 is not supported.");
			return false;
		}
		if (strncmp("OpenCL 1.1", device_version.c_str(), 10) == 0)
			m_opencl_1_1 = true;

		// create context
		m_context = cl::Context(vector<cl::Device>(&device, &device + 1));
		m_queue = cl::CommandQueue(m_context, device);

		// use requested workgroup size, but we require multiple of 8
		m_workgroup_size = ((workgroup_size + 7) / 8) * 8;

		// patch source code
		// note: ETHASH_CL_MINER_KERNEL is simply ethash_cl_miner_kernel.cl compiled
		// into a byte array by bin2h.cmake. There is no need to load the file by hand in runtime
		string code(ETHASH_CL_MINER_KERNEL, ETHASH_CL_MINER_KERNEL + ETHASH_CL_MINER_KERNEL_SIZE);
		addDefinition(code, "GROUP_SIZE", m_workgroup_size);
		addDefinition(code, "DAG_SIZE", (unsigned)(_dagSize / ETHASH_MIX_BYTES));
		addDefinition(code, "ACCESSES", ETHASH_ACCESSES);
		addDefinition(code, "MAX_OUTPUTS", c_max_search_results);
		//debugf("%s", code.c_str());

		// create miner OpenCL program
		cl::Program::Sources sources;
		sources.push_back({ code.c_str(), code.size() });

		cl::Program program(m_context, sources);
		try
		{
			program.build({ device });
			ETHCL_LOG("Printing program log");
			ETHCL_LOG(program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device).c_str());
		}
		catch (cl::Error err)
		{
			ETHCL_LOG(program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device).c_str());
			return false;
		}
		if (m_dagChunksNum == 1)
		{
			ETHCL_LOG("Loading single big chunk kernels");
			m_hash_kernel = cl::Kernel(program, "ethash_hash");
			m_search_kernel = cl::Kernel(program, "ethash_search");
		}
		else
		{
			ETHCL_LOG("Loading chunk kernels");
			m_hash_kernel = cl::Kernel(program, "ethash_hash_chunks");
			m_search_kernel = cl::Kernel(program, "ethash_search_chunks");
		}

		// create buffer for dag
		if (m_dagChunksNum == 1)
		{
			ETHCL_LOG("Creating one big buffer");
			m_dagChunks.push_back(cl::Buffer(m_context, CL_MEM_READ_ONLY, _dagSize));
		}
		else
			for (unsigned i = 0; i < m_dagChunksNum; i++)
			{
				// TODO Note: If we ever change to _dagChunksNum other than 4, then the size would need recalculation
				ETHCL_LOG("Creating buffer for chunk " << i);
				m_dagChunks.push_back(cl::Buffer(
					m_context,
					CL_MEM_READ_ONLY,
					(i == 3) ? (_dagSize - 3 * ((_dagSize >> 9) << 7)) : (_dagSize >> 9) << 7
				));
			}

		// create buffer for header
		ETHCL_LOG("Creating buffer for header.");
		m_header = cl::Buffer(m_context, CL_MEM_READ_ONLY, 32);

		if (m_dagChunksNum == 1)
		{
			ETHCL_LOG("Mapping one big chunk.");
			m_queue.enqueueWriteBuffer(m_dagChunks[0], CL_TRUE, 0, _dagSize, _dag);
		}
		else
		{
			// TODO Note: If we ever change to _dagChunksNum other than 4, then the size would need recalculation
			void* dag_ptr[4];
			for (unsigned i = 0; i < m_dagChunksNum; i++)
			{
				ETHCL_LOG("Mapping chunk " << i);
				dag_ptr[i] = m_queue.enqueueMapBuffer(m_dagChunks[i], true, m_opencl_1_1 ? CL_MAP_WRITE : CL_MAP_WRITE_INVALIDATE_REGION, 0, (i == 3) ? (_dagSize - 3 * ((_dagSize >> 9) << 7)) : (_dagSize >> 9) << 7);
			}
			for (unsigned i = 0; i < m_dagChunksNum; i++)
			{
				memcpy(dag_ptr[i], (char *)_dag + i*((_dagSize >> 9) << 7), (i == 3) ? (_dagSize - 3 * ((_dagSize >> 9) << 7)) : (_dagSize >> 9) << 7);
				m_queue.enqueueUnmapMemObject(m_dagChunks[i], dag_ptr[i]);
			}
		}

		// create mining buffers
		for (unsigned i = 0; i != c_num_buffers; ++i)
		{
			ETHCL_LOG("Creating mining buffer " << i);
			m_hash_buf[i] = cl::Buffer(m_context, CL_MEM_WRITE_ONLY | (!m_opencl_1_1 ? CL_MEM_HOST_READ_ONLY : 0), 32 * c_hash_batch_size);
			m_search_buf[i] = cl::Buffer(m_context, CL_MEM_WRITE_ONLY, (c_max_search_results + 1) * sizeof(uint32_t));
		}
	}
	catch (cl::Error err)
	{
		ETHCL_LOG(err.what() << "(" << err.err() << ")");
		return false;
	}
	return true;
}

void ethash_cl_miner::search(uint8_t const* header, uint64_t target, search_hook& hook)
{
	try
	{
		struct pending_batch
		{
			uint64_t start_nonce;
			unsigned buf;
		};
		queue<pending_batch> pending;

		// this can't be a static because in MacOSX OpenCL implementation a segfault occurs when a static is passed to OpenCL functions
		uint32_t const c_zero = 0;

		// update header constant buffer
		m_queue.enqueueWriteBuffer(m_header, false, 0, 32, header);
		for (unsigned i = 0; i != c_num_buffers; ++i)
			m_queue.enqueueWriteBuffer(m_search_buf[i], false, 0, 4, &c_zero);

#if CL_VERSION_1_2 && 0
		cl::Event pre_return_event;
		if (!m_opencl_1_1)
			m_queue.enqueueBarrierWithWaitList(NULL, &pre_return_event);
		else
#endif
			m_queue.finish();

		unsigned argPos = 2;
		m_search_kernel.setArg(1, m_header);
		for (unsigned i = 0; i < m_dagChunksNum; ++i, ++argPos)
			m_search_kernel.setArg(argPos, m_dagChunks[i]);
		// pass these to stop the compiler unrolling the loops
		m_search_kernel.setArg(argPos + 1, target);
		m_search_kernel.setArg(argPos + 2, ~0u);

		unsigned buf = 0;
		random_device engine;
		uint64_t start_nonce = uniform_int_distribution<uint64_t>()(engine);
		for (;; start_nonce += c_search_batch_size)
		{
			// supply output buffer to kernel
			m_search_kernel.setArg(0, m_search_buf[buf]);
			if (m_dagChunksNum == 1)
				m_search_kernel.setArg(3, start_nonce);
			else
				m_search_kernel.setArg(6, start_nonce);

			// execute it!
			m_queue.enqueueNDRangeKernel(m_search_kernel, cl::NullRange, c_search_batch_size, m_workgroup_size);

			pending.push({ start_nonce, buf });
			buf = (buf + 1) % c_num_buffers;

			// read results
			if (pending.size() == c_num_buffers)
			{
				pending_batch const& batch = pending.front();

				// could use pinned host pointer instead
				uint32_t* results = (uint32_t*)m_queue.enqueueMapBuffer(m_search_buf[batch.buf], true, CL_MAP_READ, 0, (1 + c_max_search_results) * sizeof(uint32_t));
				unsigned num_found = min<unsigned>(results[0], c_max_search_results);

				uint64_t nonces[c_max_search_results];
				for (unsigned i = 0; i != num_found; ++i)
					nonces[i] = batch.start_nonce + results[i + 1];

				m_queue.enqueueUnmapMemObject(m_search_buf[batch.buf], results);
				bool exit = num_found && hook.found(nonces, num_found);
				exit |= hook.searched(batch.start_nonce, c_search_batch_size); // always report searched before exit
				if (exit)
					break;

				// reset search buffer if we're still going
				if (num_found)
					m_queue.enqueueWriteBuffer(m_search_buf[batch.buf], true, 0, 4, &c_zero);

				pending.pop();
			}
		}

		// not safe to return until this is ready
#if CL_VERSION_1_2 && 0
		if (!m_opencl_1_1)
			pre_return_event.wait();
#endif
	}
	catch (cl::Error err)
	{
		ETHCL_LOG(err.what() << "(" << err.err() << ")");
	}
}
