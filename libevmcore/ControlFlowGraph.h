/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * @file ControlFlowGraph.h
 * @author Christian <c@ethdev.com>
 * @date 2015
 * Control flow analysis for the optimizer.
 */

#pragma once

#include <vector>
#include <libdevcore/Common.h>
#include <libdevcore/Assertions.h>

namespace dev
{
namespace eth
{

class AssemblyItem;
using AssemblyItems = std::vector<AssemblyItem>;

/**
 * Identifier for a block, coincides with the tag number of an AssemblyItem but adds a special
 * ID for the inital block.
 */
class BlockId
{
public:
	BlockId() { *this = invalid(); }
	explicit BlockId(unsigned _id): m_id(_id) {}
	explicit BlockId(u256 const& _id);
	static BlockId initial() { return BlockId(-2); }
	static BlockId invalid() { return BlockId(-1); }

	bool operator==(BlockId const& _other) const { return m_id == _other.m_id; }
	bool operator!=(BlockId const& _other) const { return m_id != _other.m_id; }
	bool operator<(BlockId const& _other) const { return m_id < _other.m_id; }
	explicit operator bool() const { return *this != invalid(); }

private:
	unsigned m_id;
};

/**
 * Control flow block inside which instruction counter is always incremented by one
 * (except for possibly the last instruction).
 */
struct BasicBlock
{
	/// Start index into assembly item list.
	unsigned begin = 0;
	/// End index (excluded) inte assembly item list.
	unsigned end = 0;
	/// Tags pushed inside this block, with multiplicity.
	std::vector<BlockId> pushedTags;
	/// ID of the block that always follows this one (either JUMP or flow into new block),
	/// or BlockId::invalid() otherwise
	BlockId next = BlockId::invalid();
	/// ID of the block that has to precede this one.
	BlockId prev = BlockId::invalid();

	enum class EndType { JUMP, JUMPI, STOP, HANDOVER };
	EndType endType = EndType::HANDOVER;
};

class ControlFlowGraph
{
public:
	/// Initializes the control flow graph.
	/// @a _items has to persist across the usage of this class.
	ControlFlowGraph(AssemblyItems const& _items): m_items(_items) {}
	/// @returns the collection of optimised items, should be called only once.
	AssemblyItems optimisedItems();

private:
	void findLargestTag();
	void splitBlocks();
	void resolveNextLinks();
	void removeUnusedBlocks();
	void setPrevLinks();
	AssemblyItems rebuildCode();

	BlockId generateNewId();

	unsigned m_lastUsedId = 0;
	AssemblyItems const& m_items;
	std::map<BlockId, BasicBlock> m_blocks;
};


}
}
