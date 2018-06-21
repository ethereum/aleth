/* Aleth: Ethereum C++ client, tools and libraries.
 * Copyright 2018 Aleth Autors.
 * Licensed under the GNU General Public License, Version 3. See the LICENSE file.
 */

#pragma once

#include <evmc/evmc.h>

#if __cplusplus
extern "C" {
/* TODO: Move NOEXCEPT to evmc.h */
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

struct evmc_instance* evmc_create_interpreter() NOEXCEPT;

#if __cplusplus
}
#endif
