#include "Cache.h"

#include <mutex>

#include "preprocessor/llvm_includes_start.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_os_ostream.h>
#include "preprocessor/llvm_includes_end.h"

#include "support/Path.h"
#include "ExecStats.h"
#include "Utils.h"
#include "BuildInfo.gen.h"

namespace dev
{
namespace evmjit
{

namespace
{
	using Guard = std::lock_guard<std::mutex>;
	std::mutex x_cacheMutex;
	CacheMode g_mode;
	std::unique_ptr<llvm::MemoryBuffer> g_lastObject;
	JITListener* g_listener;

	std::string getVersionedCacheDir()
	{
		llvm::SmallString<256> path{path::user_cache_directory()};
		static const auto c_ethereumAppName = UTILS_OS_LINUX ? "ethereum" : "Ethereum";
		llvm::sys::path::append(path, c_ethereumAppName, "evmjit", EVMJIT_VERSION);
		return path.str();
	}

}

ObjectCache* Cache::init(CacheMode _mode, JITListener* _listener)
{
	DLOG(cache) << "Cache dir: " << getVersionedCacheDir() << "\n";

	Guard g{x_cacheMutex};

	g_mode = _mode;
	g_listener = _listener;

	if (g_mode == CacheMode::clear)
	{
		Cache::clear();
		g_mode = CacheMode::off;
	}

	if (g_mode != CacheMode::off)
	{
		static ObjectCache objectCache;
		return &objectCache;
	}
	return nullptr;
}

void Cache::clear()
{
	Guard g{x_cacheMutex};

	auto cachePath = getVersionedCacheDir();
	std::error_code err;
	for (auto it = llvm::sys::fs::directory_iterator{cachePath, err}; it != decltype(it){}; it.increment(err))
		llvm::sys::fs::remove(it->path());
}

void Cache::preload(llvm::ExecutionEngine& _ee, std::unordered_map<std::string, uint64_t>& _funcCache)
{
	Guard g{x_cacheMutex};

	// Disable listener
	auto listener = g_listener;
	g_listener = nullptr;

	auto cachePath = getVersionedCacheDir();
	std::error_code err;
	for (auto it = llvm::sys::fs::directory_iterator{cachePath, err}; it != decltype(it){}; it.increment(err))
	{
		auto name = it->path().substr(cachePath.size() + 1);
		if (auto module = getObject(name))
		{
			DLOG(cache) << "Preload: " << name << "\n";
			_ee.addModule(std::move(module));
			auto addr = _ee.getFunctionAddress(name);
			assert(addr);
			_funcCache[std::move(name)] = addr;
		}
	}

	g_listener = listener;
}

std::unique_ptr<llvm::Module> Cache::getObject(std::string const& id)
{
	Guard g{x_cacheMutex};

	if (g_mode != CacheMode::on && g_mode != CacheMode::read)
		return nullptr;

	// TODO: Disabled because is not thread-safe.
	//if (g_listener)
	//	g_listener->stateChanged(ExecState::CacheLoad);

	DLOG(cache) << id << ": search\n";
	if (!CHECK(!g_lastObject))
		g_lastObject = nullptr;

	llvm::SmallString<256> cachePath{getVersionedCacheDir()};
	llvm::sys::path::append(cachePath, id);

	if (auto r = llvm::MemoryBuffer::getFile(cachePath, -1, false))
		g_lastObject = llvm::MemoryBuffer::getMemBufferCopy(r.get()->getBuffer());
	else if (r.getError() != std::make_error_code(std::errc::no_such_file_or_directory))
		DLOG(cache) << r.getError().message(); // TODO: Add warning log

	if (g_lastObject)  // if object found create fake module
	{
		DLOG(cache) << id << ": found\n";
		auto&& context = llvm::getGlobalContext();
		auto module = llvm::make_unique<llvm::Module>(id, context);
		auto mainFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {}, false);
		auto mainFunc = llvm::Function::Create(mainFuncType, llvm::Function::ExternalLinkage, id, module.get());
		auto bb = llvm::BasicBlock::Create(context, {}, mainFunc);
		bb->getInstList().push_back(new llvm::UnreachableInst{context});
		return module;
	}
	DLOG(cache) << id << ": not found\n";
	return nullptr;
}


void ObjectCache::notifyObjectCompiled(llvm::Module const* _module, llvm::MemoryBufferRef _object)
{
	Guard g{x_cacheMutex};

	// Only in "on" and "write" mode
	if (g_mode != CacheMode::on && g_mode != CacheMode::write)
		return;

	// TODO: Disabled because is not thread-safe.
	// if (g_listener)
		// g_listener->stateChanged(ExecState::CacheWrite);

	auto&& id = _module->getModuleIdentifier();
	llvm::SmallString<256> cachePath{getVersionedCacheDir()};
	if (auto err = llvm::sys::fs::create_directories(cachePath))
	{
		DLOG(cache) << "Cannot create cache dir " << cachePath.str().str() << " (error: " << err.message() << "\n";
		return;
	}

	llvm::sys::path::append(cachePath, id);

	DLOG(cache) << id << ": write\n";
	std::error_code error;
	llvm::raw_fd_ostream cacheFile(cachePath, error, llvm::sys::fs::F_None);
	cacheFile << _object.getBuffer();
}

std::unique_ptr<llvm::MemoryBuffer> ObjectCache::getObject(llvm::Module const* _module)
{
	Guard g{x_cacheMutex};

	DLOG(cache) << _module->getModuleIdentifier() << ": use\n";
	return std::move(g_lastObject);
}

}
}
