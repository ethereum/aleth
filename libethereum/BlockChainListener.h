#pragma once

#include "../libethcore/BlockInfo.h"
#include "State.h"

namespace eth

{

    class BlockChainListener
    {

        public:
            virtual void onBlockImport(State &_state, BlockInfo _block){};

    };
};