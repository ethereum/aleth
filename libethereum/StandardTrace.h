// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <json/json.h>
#include <libdevcore/Common.h>
#include <libevm/Instruction.h>
#include <libevm/VMFace.h>
#include <cstdint>

namespace Json
{
class Value;
}

namespace dev
{
namespace eth
{
class StandardTrace
{
public:
    struct DebugOptions
    {
        bool disableStorage = false;
        bool disableMemory = false;
        bool disableStack = false;
        bool fullStorage = false;
    };

    // Output json trace to stream, one line per op
    explicit StandardTrace(std::ostream& _outStream) noexcept : m_outStream{&_outStream} {}
    // Append json trace to given (array) value
    explicit StandardTrace(Json::Value& _outValue) noexcept : m_outValue{&_outValue} {}

    void operator()(uint64_t _steps, uint64_t _PC, Instruction _inst, bigint _newMemSize,
        bigint _gasCost, bigint _gas, VMFace const* _vm, ExtVMFace const* _extVM);

    void setShowMnemonics() { m_showMnemonics = true; }
    void setOptions(DebugOptions _options) { m_options = _options; }

    OnOpFunc onOp()
    {
        return [=](uint64_t _steps, uint64_t _PC, Instruction _inst, bigint _newMemSize,
                   bigint _gasCost, bigint _gas, VMFace const* _vm, ExtVMFace const* _extVM) {
            (*this)(_steps, _PC, _inst, _newMemSize, _gasCost, _gas, _vm, _extVM);
        };
    }

private:
    bool m_showMnemonics = false;
    std::vector<Instruction> m_lastInst;
    std::ostream* m_outStream = nullptr;
    Json::Value* m_outValue = nullptr;
    Json::FastWriter m_fastWriter;
    DebugOptions m_options;
};
}  // namespace eth
}  // namespace dev
