#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <type_traits>

#ifdef _MSC_VER
#ifdef evmjit_EXPORTS
	#define EVMJIT_API __declspec(dllexport)
#else
	#define EVMJIT_API __declspec(dllimport)
#endif

#define _ALLOW_KEYWORD_MACROS
#define noexcept throw()
#else
#define EVMJIT_API [[gnu::visibility("default")]]
#endif

namespace dev
{
namespace evmjit
{

using byte = uint8_t;
using bytes_ref = std::tuple<byte const*, size_t>;

/// Representation of 256-bit hash value
struct h256
{
	uint64_t words[4];
};

inline bool operator==(h256 const& _h1, h256 const& _h2)
{
	return 	_h1.words[0] == _h2.words[0] &&
			_h1.words[1] == _h2.words[1] &&
			_h1.words[2] == _h2.words[2] &&
			_h1.words[3] == _h2.words[3];
}

/// Representation of 256-bit value binary compatible with LLVM i256
struct i256
{
	uint64_t words[4];

	i256() = default;
	i256(h256 const& _h) { std::memcpy(this, &_h, sizeof(*this)); }
};

// TODO: Merge with ExecutionContext
struct RuntimeData
{
	enum Index
	{
		Gas,
		GasPrice,
		CallData,
		CallDataSize,
		Address,
		Caller,
		Origin,
		TransferredCallValue,	// actual ether amount transferred
		ApparentCallValue,		// value of msg.value - different during DELEGATECALL
		CoinBase,
		Difficulty,
		GasLimit,
		Number,
		Timestamp,
		Code,
		CodeSize,

		SuicideDestAddress = Address,		///< Suicide balance destination address
		ReturnData 		   = CallData,		///< Return data pointer (set only in case of RETURN)
		ReturnDataSize 	   = CallDataSize,	///< Return data size (set only in case of RETURN)
	};

	static size_t const numElements = CodeSize + 1;

	int64_t 	gas = 0;
	int64_t 	gasPrice = 0;
	byte const* callData = nullptr;
	uint64_t 	callDataSize = 0;
	i256 		address;
	i256 		caller;
	i256 		origin;
	i256 		transferredValue;
	i256 		apparentValue;
	i256 		coinBase;
	i256 		difficulty;
	i256 		gasLimit;
	uint64_t 	number = 0;
	int64_t 	timestamp = 0;
	byte const* code = nullptr;
	uint64_t 	codeSize = 0;
	h256		codeHash;
};

struct JITSchedule
{
	// Move to constexpr once all our target compilers support it.
	typedef std::integral_constant<uint64_t, 1024> stackLimit;
	typedef std::integral_constant<uint64_t, 0> stepGas0;
	typedef std::integral_constant<uint64_t, 2> stepGas1;
	typedef std::integral_constant<uint64_t, 3> stepGas2;
	typedef std::integral_constant<uint64_t, 5> stepGas3;
	typedef std::integral_constant<uint64_t, 8> stepGas4;
	typedef std::integral_constant<uint64_t, 10> stepGas5;
	typedef std::integral_constant<uint64_t, 20> stepGas6;
	typedef std::integral_constant<uint64_t, 0> stepGas7;
	typedef std::integral_constant<uint64_t, 10> expByteGas;
	typedef std::integral_constant<uint64_t, 30> sha3Gas;
	typedef std::integral_constant<uint64_t, 6> sha3WordGas;
	typedef std::integral_constant<uint64_t, 50> sloadGas;
	typedef std::integral_constant<uint64_t, 20000> sstoreSetGas;
	typedef std::integral_constant<uint64_t, 5000> sstoreResetGas;
	typedef std::integral_constant<uint64_t, 5000> sstoreClearGas;
	typedef std::integral_constant<uint64_t, 1> jumpdestGas;
	typedef std::integral_constant<uint64_t, 375> logGas;
	typedef std::integral_constant<uint64_t, 8> logDataGas;
	typedef std::integral_constant<uint64_t, 375> logTopicGas;
	typedef std::integral_constant<uint64_t, 32000> createGas;
	typedef std::integral_constant<uint64_t, 40> callGas;
	typedef std::integral_constant<uint64_t, 3> memoryGas;
	typedef std::integral_constant<uint64_t, 3> copyGas;
	bool haveDelegateCall = true;

	/// Computes a hash of the schedule.
	EVMJIT_API int64_t id() const;

	/// @returns an identifier for the code that is built from the code and the schedule data.
	EVMJIT_API std::string codeIdentifier(h256 const& _codeHash) const;
};

/// VM Environment (ExtVM) opaque type
struct Env;

enum class ReturnCode
{
	// Success codes
	Stop    = 0,
	Return  = 1,
	Suicide = 2,

	// Standard error codes
	OutOfGas           = -1,

	// Internal error codes
	LLVMError          = -101,

	UnexpectedException = -111,

	LinkerWorkaround = -299,
};

class ExecutionContext
{
public:
	ExecutionContext() = default;
	ExecutionContext(RuntimeData& _data, Env* _env) { init(_data, _env); }
	ExecutionContext(ExecutionContext const&) = delete;
	ExecutionContext& operator=(ExecutionContext const&) = delete;
	EVMJIT_API ~ExecutionContext() noexcept;

	void init(RuntimeData& _data, Env* _env) { m_data = &_data; m_env = _env; }

	byte const* code() const { return m_data->code; }
	uint64_t codeSize() const { return m_data->codeSize; }
	h256 const& codeHash() const { return m_data->codeHash; }

	bytes_ref getReturnData() const;

protected:
	RuntimeData* m_data = nullptr;	///< Pointer to data. Expected by compiled contract.
	Env* m_env = nullptr;			///< Pointer to environment proxy. Expected by compiled contract.
	byte* m_memData = nullptr;
	uint64_t m_memSize = 0;
	uint64_t m_memCap = 0;

public:
	/// Reference to returned data (RETURN opcode used)
	bytes_ref returnData;
};

class JIT
{
public:

	/// Ask JIT if the EVM code is ready for execution.
	/// Returns `true` if the EVM code has been compiled and loaded into memory.
	/// In this case the code can be executed without overhead.
	/// \param _codeIdentifier	the identifier of the code consisting of a hash of the code and the schedule.
	EVMJIT_API static bool isCodeReady(std::string const& _codeIdentifier);

	/// Compile the given EVM code to machine code and make available for execution.
	EVMJIT_API static void compile(
		byte const* _code,
		uint64_t _codeSize,
		std::string const& _codeIdentifier,
		JITSchedule const& _schedule
	);

	/// Execude the code given in @a _context and compile it if necessary.
	EVMJIT_API static ReturnCode exec(ExecutionContext& _context, JITSchedule const& _schedule);
};

}
}

namespace std
{
template<> struct hash<dev::evmjit::h256>
{
	size_t operator()(dev::evmjit::h256 const& _h) const
	{
		/// This implementation expects the argument to be a full 256-bit Keccak hash.
		/// It does nothing more than returning a slice of the input hash.
		return static_cast<size_t>(_h.words[0]);
	};
};
}
