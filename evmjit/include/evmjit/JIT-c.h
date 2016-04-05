
#include <stdint.h>

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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct evmjit_i256
{
	uint64_t words[4];
} evmjit_i256;

typedef struct evmjit_runtime_data
{
	int64_t 	gas;
	int64_t 	gasPrice;
	char const* callData;
	uint64_t 	callDataSize;
	evmjit_i256 address;
	evmjit_i256 caller;
	evmjit_i256 origin;
	evmjit_i256 transferredValue;
	evmjit_i256 apparentValue;
	evmjit_i256 coinBase;
	evmjit_i256 difficulty;
	evmjit_i256 gasLimit;
	uint64_t 	number;
	int64_t 	timestamp;
	char const* code;
	uint64_t 	codeSize;
	evmjit_i256	codeHash;
} evmjit_runtime_data;

typedef enum evmjit_return_code
{
	// Success codes
	Stop    = 0,
	Return  = 1,
	Suicide = 2,

	// Standard error codes
	OutOfGas           = -1,

	// Internal error codes
	LLVMError           = -101,
	UnexpectedException = -111
} evmjit_return_code;

typedef struct evmjit_context evmjit_context;

EVMJIT_API evmjit_context* evmjit_create(evmjit_runtime_data* _data, void* _env);

EVMJIT_API evmjit_return_code evmjit_exec(evmjit_context* _context, void* _schedule);

EVMJIT_API void evmjit_destroy(evmjit_context* _context);


inline char const* evmjit_get_output(evmjit_runtime_data* _data) { return _data->callData; }
inline uint64_t evmjit_get_output_size(evmjit_runtime_data* _data) { return _data->callDataSize; }

#ifdef __cplusplus
}
#endif
