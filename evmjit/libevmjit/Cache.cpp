#include "Cache.h"

#include "preprocessor/llvm_includes_start.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_os_ostream.h>
#include "preprocessor/llvm_includes_end.h"

#include "ExecutionEngine.h"
#include "Utils.h"

namespace dev
{
namespace eth
{
namespace jit
{

namespace
{
	llvm::MemoryBuffer* g_lastObject;
	ExecutionEngineListener* g_listener;
}

ObjectCache* Cache::getObjectCache(ExecutionEngineListener* _listener)
{
	static ObjectCache objectCache;
	g_listener = _listener;
	return &objectCache;
}

std::unique_ptr<llvm::Module> Cache::getObject(std::string const& id)
{
	if (g_listener)
		g_listener->stateChanged(ExecState::CacheLoad);

	DLOG(cache) << id << ": search\n";
	if (!CHECK(!g_lastObject))
		g_lastObject = nullptr;

	llvm::SmallString<256> cachePath;
	llvm::sys::path::system_temp_directory(false, cachePath);
	llvm::sys::path::append(cachePath, "evm_objs", id);

#if defined(__GNUC__) && !defined(NDEBUG)
	llvm::sys::fs::file_status st;
	auto err = llvm::sys::fs::status(cachePath.str(), st);
	if (err)
		return nullptr;
	auto mtime = st.getLastModificationTime().toEpochTime();

	std::tm tm;
	strptime(__DATE__ __TIME__, " %b %d %Y %H:%M:%S", &tm);
	auto btime = (uint64_t)std::mktime(&tm);
	if (btime > mtime)
		return nullptr;
#endif

	if (auto r = llvm::MemoryBuffer::getFile(cachePath.str(), -1, false))
		g_lastObject = llvm::MemoryBuffer::getMemBufferCopy(r.get()->getBuffer());
	else if (r.getError() != std::make_error_code(std::errc::no_such_file_or_directory))
		std::cerr << r.getError().message(); // TODO: Add log

	if (g_lastObject)  // if object found create fake module
	{
		DLOG(cache) << id << ": found\n";
		auto&& context = llvm::getGlobalContext();
		auto module = std::unique_ptr<llvm::Module>(new llvm::Module(id, context));
		auto mainFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {}, false);
		auto mainFunc = llvm::Function::Create(mainFuncType, llvm::Function::ExternalLinkage, id, module.get());
		auto bb = llvm::BasicBlock::Create(context, {}, mainFunc);
		bb->getInstList().push_back(new llvm::UnreachableInst{context});
		return module;
	}
	DLOG(cache) << id << ": not found\n";
	return nullptr;
}


void ObjectCache::notifyObjectCompiled(llvm::Module const* _module, llvm::MemoryBuffer const* _object)
{
	if (g_listener)
		g_listener->stateChanged(ExecState::CacheWrite);

	auto&& id = _module->getModuleIdentifier();
	llvm::SmallString<256> cachePath;
	llvm::sys::path::system_temp_directory(false, cachePath);
	llvm::sys::path::append(cachePath, "evm_objs");

	if (llvm::sys::fs::create_directory(cachePath.str()))
		return; // TODO: Add log

	llvm::sys::path::append(cachePath, id);

	DLOG(cache) << id << ": write\n";
	std::string error;
	llvm::raw_fd_ostream cacheFile(cachePath.c_str(), error, llvm::sys::fs::F_None);
	cacheFile << _object->getBuffer();
}

llvm::MemoryBuffer* ObjectCache::getObject(llvm::Module const* _module)
{
	DLOG(cache) << _module->getModuleIdentifier() << ": use\n";
	auto o = g_lastObject;
	g_lastObject = nullptr;
	return o;
}

}
}
}
