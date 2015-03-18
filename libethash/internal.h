#pragma once
#include "compiler.h"
#include "endian.h"
#include "ethash.h"

#define ENABLE_SSE 1

#if defined(_M_X64) && ENABLE_SSE
#include <smmintrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// compile time settings
#define NODE_WORDS (64/4)
#define MIX_WORDS (MIX_BYTES/4)
#define MIX_NODES (MIX_WORDS / NODE_WORDS)
#include <stdint.h>

typedef union node {
    uint8_t bytes[NODE_WORDS * 4];
    uint32_t words[NODE_WORDS];
    uint64_t double_words[NODE_WORDS / 2];

#if defined(_M_X64) && ENABLE_SSE
	__m128i xmm[NODE_WORDS/4];
#endif

} node;

void ethash_calculate_dag_item(
        node *const ret,
        const unsigned node_index,
        ethash_params const *params,
        ethash_cache const *cache
);

void ethash_quick_hash(
        uint8_t return_hash[32],
        const uint8_t header_hash[32],
        const uint64_t nonce,
        const uint8_t mix_hash[32]);

#ifdef __cplusplus
}
#endif