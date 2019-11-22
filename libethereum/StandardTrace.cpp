// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "StandardTrace.h"
#include "ExtVM.h"
#include <libevm/LegacyVM.h>

namespace dev
{
namespace eth
{
namespace
{
bool changesStorage(Instruction _inst)
{
    return _inst == Instruction::SSTORE;
}

}  // namespace

void StandardTrace::operator()(uint64_t _steps, uint64_t PC, Instruction inst, bigint newMemSize,
    bigint gasCost, bigint gas, VMFace const* _vm, ExtVMFace const* voidExt)
{
    (void)_steps;

    ExtVM const& ext = dynamic_cast<ExtVM const&>(*voidExt);
    auto vm = dynamic_cast<LegacyVM const*>(_vm);

    Json::Value r(Json::objectValue);

    Json::Value stack(Json::arrayValue);
    if (vm && !m_options.disableStack)
    {
        // Try extracting information about the stack from the VM is supported.
        for (auto const& i : vm->stack())
            stack.append(toCompactHexPrefixed(i, 1));
        r["stack"] = stack;
    }

    bool newContext = false;
    Instruction lastInst = Instruction::STOP;

    if (m_lastInst.size() == ext.depth)
    {
        // starting a new context
        assert(m_lastInst.size() == ext.depth);
        m_lastInst.push_back(inst);
        newContext = true;
    }
    else if (m_lastInst.size() == ext.depth + 2)
    {
        m_lastInst.pop_back();
        lastInst = m_lastInst.back();
    }
    else if (m_lastInst.size() == ext.depth + 1)
    {
        // continuing in previous context
        lastInst = m_lastInst.back();
        m_lastInst.back() = inst;
    }
    else
    {
        cwarn << "GAA!!! Tracing VM and more than one new/deleted stack frame between steps!";
        cwarn << "Attmepting naive recovery...";
        m_lastInst.resize(ext.depth + 1);
    }

    if (vm)
    {
        bytes const& memory = vm->memory();

        Json::Value memJson(Json::arrayValue);
        if (!m_options.disableMemory)
        {
            for (unsigned i = 0; i < memory.size(); i += 32)
            {
                bytesConstRef memRef(memory.data() + i, 32);
                memJson.append(toHex(memRef));
            }
            r["memory"] = memJson;
        }
        r["memSize"] = static_cast<uint64_t>(memory.size());
    }

    if (!m_options.disableStorage &&
        (m_options.fullStorage || changesStorage(lastInst) || newContext))
    {
        Json::Value storage(Json::objectValue);
        for (auto const& i : ext.state().storage(ext.myAddress))
            storage[toCompactHexPrefixed(i.second.first, 1)] =
                toCompactHexPrefixed(i.second.second, 1);
        r["storage"] = storage;
    }

    r["op"] = static_cast<uint8_t>(inst);
    if (m_showMnemonics)
        r["opName"] = instructionInfo(inst).name;
    r["pc"] = PC;
    r["gas"] = toString(gas);
    r["gasCost"] = toString(gasCost);
    r["depth"] = ext.depth + 1;  // depth in standard trace is 1-based
    if (!!newMemSize)
        r["memexpand"] = toString(newMemSize);

    if (m_outValue)
        m_outValue->append(r);
    else
        *m_outStream << m_fastWriter.write(r) << std::flush;
}
}  // namespace eth
}  // namespace dev
