#pragma once

#include <memory>

#include <llvm/ExecutionEngine/ObjectCache.h>

namespace dev
{
namespace eth
{
namespace jit
{
class ExecutionEngineListener;

enum class CacheMode
{
	on,
	off,
	read,
	write,
	clear
};

class ObjectCache : public llvm::ObjectCache
{
public:
	/// notifyObjectCompiled - Provides a pointer to compiled code for Module M.
	virtual void notifyObjectCompiled(llvm::Module const* _module, llvm::MemoryBuffer const* _object) final override;

	/// getObjectCopy - Returns a pointer to a newly allocated MemoryBuffer that
	/// contains the object which corresponds with Module M, or 0 if an object is
	/// not available. The caller owns both the MemoryBuffer returned by this
	/// and the memory it references.
	virtual llvm::MemoryBuffer* getObject(llvm::Module const* _module) final override;
};


class Cache
{
public:
	static ObjectCache* getObjectCache(CacheMode _mode, ExecutionEngineListener* _listener);
	static std::unique_ptr<llvm::Module> getObject(std::string const& id);

	/// Clears cache storage
	static void clear();
};

}
}
}
