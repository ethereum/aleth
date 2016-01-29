#include "evmjit/JIT.h"

#include <array>
#include <mutex>

#include "preprocessor/llvm_includes_start.h"
#include <llvm/IR/Module.h>
#include <llvm/ADT/Triple.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ManagedStatic.h>
#include "preprocessor/llvm_includes_end.h"

#include "Compiler.h"
#include "Optimizer.h"
#include "Cache.h"
#include "ExecStats.h"
#include "Utils.h"
#include "BuildInfo.gen.h"

namespace dev
{
namespace evmjit
{
using namespace eth::jit;

namespace
{
using ExecFunc = ReturnCode(*)(ExecutionContext*);

template <size_t _size>
std::string toHex(std::array<byte, _size> const& _data)
{
	static const auto hexChars = "0123456789abcdef";
	std::string str;
	str.resize(_size * 2);
	auto outIt = str.rbegin(); // reverse for BE
	for (auto b: _data)
	{
		*(outIt++) = hexChars[b & 0xf];
		*(outIt++) = hexChars[b >> 4];
	}
	return str;
}

void printVersion()
{
	std::cout << "Ethereum EVM JIT Compiler (http://github.com/ethereum/evmjit):\n"
			  << "  EVMJIT version " << EVMJIT_VERSION << "\n"
#ifdef NDEBUG
			  << "  Optimized build, "
#else
			  << "  DEBUG build, "
#endif
			  << __DATE__ << " (" << __TIME__ << ")\n"
			  << std::endl;
}

namespace cl = llvm::cl;
cl::opt<bool> g_optimize{"O", cl::desc{"Optimize"}};
cl::opt<CacheMode> g_cache{"cache", cl::desc{"Cache compiled EVM code on disk"},
	cl::values(
		clEnumValN(CacheMode::on,    "1", "Enabled"),
		clEnumValN(CacheMode::off,   "0", "Disabled"),
		clEnumValN(CacheMode::read,  "r", "Read only. No new objects are added to cache."),
		clEnumValN(CacheMode::write, "w", "Write only. No objects are loaded from cache."),
		clEnumValN(CacheMode::clear, "c", "Clear the cache storage. Cache is disabled."),
		clEnumValN(CacheMode::preload, "p", "Preload all cached objects."),
		clEnumValEnd)};
cl::opt<bool> g_stats{"st", cl::desc{"Statistics"}};
cl::opt<bool> g_dump{"dump", cl::desc{"Dump LLVM IR module"}};

void parseOptions()
{
	static llvm::llvm_shutdown_obj shutdownObj{};
	cl::AddExtraVersionPrinter(printVersion);
	cl::ParseEnvironmentOptions("evmjit", "EVMJIT", "Ethereum EVM JIT Compiler");
}

class JITImpl
{
	std::unique_ptr<llvm::ExecutionEngine> m_engine;
	mutable std::mutex x_codeMap;
	std::unordered_map<std::string, ExecFunc> m_codeMap;

public:
	static JITImpl& instance()
	{
		// We need to keep this a singleton.
		// so we only call changeVersion on it.
		static JITImpl s_instance;
		return s_instance;
	}

	JITImpl();

	llvm::ExecutionEngine& engine() { return *m_engine; }

	ExecFunc getExecFunc(std::string const& _codeIdentifier) const;
	void mapExecFunc(std::string const& _codeIdentifier, ExecFunc _funcAddr);

	ExecFunc compile(byte const* _code, uint64_t _codeSize, std::string const& _codeIdentifier, JITSchedule const& _schedule);
};

JITImpl::JITImpl()
{
	parseOptions();

	bool preloadCache = g_cache == CacheMode::preload;
	if (preloadCache)
		g_cache = CacheMode::on;

	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();

	auto module = llvm::make_unique<llvm::Module>(llvm::StringRef{}, llvm::getGlobalContext());

	// FIXME: LLVM 3.7: test on Windows
	auto triple = llvm::Triple(llvm::sys::getProcessTriple());
	if (triple.getOS() == llvm::Triple::OSType::Win32)
		triple.setObjectFormat(llvm::Triple::ObjectFormatType::ELF);  // MCJIT does not support COFF format
	module->setTargetTriple(triple.str());

	llvm::EngineBuilder builder(std::move(module));
	builder.setEngineKind(llvm::EngineKind::JIT);
	builder.setOptLevel(g_optimize ? llvm::CodeGenOpt::Default : llvm::CodeGenOpt::None);

	m_engine.reset(builder.create());

	// TODO: Update cache listener
	m_engine->setObjectCache(Cache::init(g_cache, nullptr));

	// FIXME: Disabled during API changes
	//if (preloadCache)
	//	Cache::preload(*m_engine, funcCache);
}

ExecFunc JITImpl::getExecFunc(std::string const& _codeIdentifier) const
{
	std::lock_guard<std::mutex> lock{x_codeMap};
	auto it = m_codeMap.find(_codeIdentifier);
	if (it != m_codeMap.end())
		return it->second;
	return nullptr;
}

void JITImpl::mapExecFunc(std::string const& _codeIdentifier, ExecFunc _funcAddr)
{
	std::lock_guard<std::mutex> lock{x_codeMap};
	m_codeMap.emplace(_codeIdentifier, _funcAddr);
}

ExecFunc JITImpl::compile(byte const* _code, uint64_t _codeSize, std::string const& _codeIdentifier, JITSchedule const& _schedule)
{
	auto module = Cache::getObject(_codeIdentifier);
	if (!module)
	{
		// TODO: Listener support must be redesigned. These should be a feature of JITImpl
		//listener->stateChanged(ExecState::Compilation);
		assert(_code || !_codeSize);
		module = Compiler({}, _schedule).compile(_code, _code + _codeSize, _codeIdentifier);

		if (g_optimize)
		{
			//listener->stateChanged(ExecState::Optimization);
			optimize(*module);
		}

		prepare(*module);
	}
	if (g_dump)
		module->dump();

	m_engine->addModule(std::move(module));
	//listener->stateChanged(ExecState::CodeGen);
	return (ExecFunc)m_engine->getFunctionAddress(_codeIdentifier);
}

} // anonymous namespace

bool JIT::isCodeReady(std::string const& _codeIdentifier)
{
	return JITImpl::instance().getExecFunc(_codeIdentifier) != nullptr;
}

void JIT::compile(byte const* _code, uint64_t _codeSize, std::string const& _codeIdentifier, JITSchedule const& _schedule)
{
	auto& jit = JITImpl::instance();
	auto execFunc = jit.compile(_code, _codeSize, _codeIdentifier, _schedule);
	if (execFunc) // FIXME: What with error?
		jit.mapExecFunc(_codeIdentifier, execFunc);
}

ReturnCode JIT::exec(ExecutionContext& _context, JITSchedule const& _schedule)
{
	//std::unique_ptr<ExecStats> listener{new ExecStats};
	//listener->stateChanged(ExecState::Started);
	//static StatsCollector statsCollector;

	auto& jit = JITImpl::instance();
	auto codeIdentifier = _schedule.codeIdentifier(_context.codeHash());
	auto execFunc = jit.getExecFunc(codeIdentifier);
	if (!execFunc)
	{
		execFunc = jit.compile(_context.code(), _context.codeSize(), codeIdentifier, _schedule);
		if (!execFunc)
			return ReturnCode::LLVMError;
		jit.mapExecFunc(codeIdentifier, execFunc);
	}

	//listener->stateChanged(ExecState::Execution);
	auto returnCode = execFunc(&_context);
	//listener->stateChanged(ExecState::Return);

	if (returnCode == ReturnCode::Return)
		_context.returnData = _context.getReturnData(); // Save reference to return data

	//listener->stateChanged(ExecState::Finished);
	// if (g_stats)
	// 	statsCollector.stats.push_back(std::move(listener));

	return returnCode;
}


extern "C" void ext_free(void* _data) noexcept;

ExecutionContext::~ExecutionContext() noexcept
{
	if (m_memData)
		ext_free(m_memData); // Use helper free to check memory leaks
}

bytes_ref ExecutionContext::getReturnData() const
{
	auto data = m_data->callData;
	auto size = static_cast<size_t>(m_data->callDataSize);

	if (data < m_memData || data >= m_memData + m_memSize || size == 0)
	{
		assert(size == 0); // data can be an invalid pointer only if size is 0
		m_data->callData = nullptr;
		return {};
	}

	return bytes_ref{data, size};
}

int64_t JITSchedule::id() const
{
	int64_t hash = 0;
	hash = hash * 37 + stepGas0::value;
	hash = hash * 37 + stepGas1::value;
	hash = hash * 37 + stepGas2::value;
	hash = hash * 37 + stepGas3::value;
	hash = hash * 37 + stepGas4::value;
	hash = hash * 37 + stepGas5::value;
	hash = hash * 37 + stepGas6::value;
	hash = hash * 37 + stepGas7::value;
	hash = hash * 37 + stackLimit::value;
	hash = hash * 37 + expByteGas::value;
	hash = hash * 37 + sha3Gas::value;
	hash = hash * 37 + sha3WordGas::value;
	hash = hash * 37 + sloadGas::value;
	hash = hash * 37 + sstoreSetGas::value;
	hash = hash * 37 + sstoreResetGas::value;
	hash = hash * 37 + sstoreClearGas::value;
	hash = hash * 37 + jumpdestGas::value;
	hash = hash * 37 + logGas::value;
	hash = hash * 37 + logDataGas::value;
	hash = hash * 37 + logTopicGas::value;
	hash = hash * 37 + createGas::value;
	hash = hash * 37 + callGas::value;
	hash = hash * 37 + memoryGas::value;
	hash = hash * 37 + copyGas::value;
	hash = hash * 37 + (haveDelegateCall ? 7 : 11);
	return hash;
}

std::string JITSchedule::codeIdentifier(h256 const& _codeHash) const
{
	int64_t scheduleId = id();
	return
		toHex(*(std::array<byte, 32>*)&_codeHash) +
		"-" +
		toHex(*(std::array<byte, 8>*)&scheduleId);
}


}
}
