/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SmartVM.h"
#include <unordered_map>
#include <thread>
#include <libdevcore/concurrent_queue.h>
#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/Guards.h>
#include <evmjit/JIT.h>
#include "JitUtils.h"
#include "VMFactory.h"

namespace dev
{
namespace eth
{
namespace
{
	struct JitInfo: LogChannel { static const char* name() { return "JIT"; }; static const int verbosity = 11; };

	using HitMap = std::unordered_map<std::string, uint64_t>;

	HitMap& getHitMap()
	{
		static HitMap s_hitMap;
		return s_hitMap;
	}

	struct JitTask
	{
		bytes code;
		std::string codeIdentifier;
		evmjit::JITSchedule schedule;

		static JitTask createStopSentinel() { return JitTask(); }

		bool isStopSentinel()
		{
			assert((!code.empty() || codeIdentifier.empty()) && "'empty code => empty hash' invariant failed");
			return code.empty();
		}
	};

	class JitWorker
	{
		concurrent_queue<JitTask> m_queue;
		std::thread m_worker; // Worker must be last to initialize

		void work()
		{
			clog(JitInfo) << "JIT worker started.";
			JitTask task;
			while (!(task = m_queue.pop()).isStopSentinel())
			{
				clog(JitInfo) << "Compilation... " << task.codeIdentifier;
				evmjit::JIT::compile(task.code.data(), task.code.size(), task.codeIdentifier, task.schedule);
				clog(JitInfo) << "   ...finished " << task.codeIdentifier;
			}
			clog(JitInfo) << "JIT worker finished.";
		}

	public:
		JitWorker() noexcept: m_worker([this]{ work(); })
		{}

		~JitWorker()
		{
			push(JitTask::createStopSentinel());
			m_worker.join();
		}

		void push(JitTask&& _task) { m_queue.push(std::move(_task)); }
	};
}

bytesConstRef SmartVM::execImpl(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	auto vmKind = VMKind::Interpreter; // default VM
	evmjit::JITSchedule schedule;
	if (toJITSchedule(_ext.evmSchedule(), schedule))
	{
		std::string codeIdentifier = schedule.codeIdentifier(eth2jit(_ext.codeHash));
		// Jitted EVM code already in memory?
		if (evmjit::JIT::isCodeReady(codeIdentifier))
		{
			clog(JitInfo) << "JIT:           " << codeIdentifier;
			vmKind = VMKind::JIT;
		}
		else if (!_ext.code.empty()) // This check is needed for VM tests
		{
			static JitWorker s_worker;

			// Check EVM code hit count
			static const uint64_t c_hitTreshold = 2;
			auto& hits = getHitMap()[codeIdentifier];
			++hits;
			if (hits == c_hitTreshold)
			{
				clog(JitInfo) << "Schedule:      " << codeIdentifier;
				s_worker.push({_ext.code, codeIdentifier, schedule});
			}
			clog(JitInfo) << "Interpreter:   " << codeIdentifier;
		}
	}
	else
		clog(JitInfo) << "Interpreter:   " << _ext.codeHash;

	// TODO: Selected VM must be kept only because it returns reference to its internal memory.
	//       VM implementations should be stateless, without escaping memory reference.
	m_selectedVM = VMFactory::create(vmKind);
	return m_selectedVM->execImpl(io_gas, _ext, _onOp);
}

}
}
