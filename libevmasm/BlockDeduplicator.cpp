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
 * @file BlockDeduplicator.cpp
 * @author Christian <c@ethdev.com>
 * @date 2015
 * Unifies basic blocks that share content.
 */

#include <libevmasm/BlockDeduplicator.h>
#include <functional>
#include <libevmasm/AssemblyItem.h>
#include <libevmasm/SemanticInformation.h>

using namespace std;
using namespace dev;
using namespace dev::eth;


bool BlockDeduplicator::deduplicate()
{
	// Compares indices based on the suffix that starts there, ignoring tags and stopping at
	// opcodes that stop the control flow.

	// Virtual tag that signifies "the current block" and which is used to optimise loops.
	// We abort if this virtual tag actually exists.
	AssemblyItem pushSelf(PushTag, u256(-4));
	if (
		std::count(m_items.cbegin(), m_items.cend(), pushSelf.tag()) ||
		std::count(m_items.cbegin(), m_items.cend(), pushSelf.pushTag())
	)
		return false;

	function<bool(size_t, size_t)> comparator = [&](size_t _i, size_t _j)
	{
		if (_i == _j)
			return false;

		// To compare recursive loops, we have to already unify PushTag opcodes of the
		// block's own tag.
		AssemblyItem pushFirstTag(pushSelf);
		AssemblyItem pushSecondTag(pushSelf);

		if (_i < m_items.size() && m_items.at(_i).type() == Tag)
			pushFirstTag = m_items.at(_i).pushTag();
		if (_j < m_items.size() && m_items.at(_j).type() == Tag)
			pushSecondTag = m_items.at(_j).pushTag();

		BlockIterator first(m_items.begin() + _i, m_items.end(), &pushFirstTag, &pushSelf);
		BlockIterator second(m_items.begin() + _j, m_items.end(), &pushSecondTag, &pushSelf);
		BlockIterator end(m_items.end(), m_items.end());

		if (first != end && (*first).type() == Tag)
			++first;
		if (second != end && (*second).type() == Tag)
			++second;

		return std::lexicographical_compare(first, end, second, end);
	};

	size_t iterations = 0;
	for (; ; ++iterations)
	{
		//@todo this should probably be optimized.
		set<size_t, function<bool(size_t, size_t)>> blocksSeen(comparator);
		map<u256, u256> tagReplacement;
		for (size_t i = 0; i < m_items.size(); ++i)
		{
			if (m_items.at(i).type() != Tag)
				continue;
			auto it = blocksSeen.find(i);
			if (it == blocksSeen.end())
				blocksSeen.insert(i);
			else
				tagReplacement[m_items.at(i).data()] = m_items.at(*it).data();
		}

		bool changed = false;
		for (AssemblyItem& item: m_items)
			if (item.type() == PushTag && tagReplacement.count(item.data()))
			{
				changed = true;
				item.setData(tagReplacement.at(item.data()));
			}
		if (!changed)
			break;
	}
	return iterations > 0;
}

BlockDeduplicator::BlockIterator& BlockDeduplicator::BlockIterator::operator++()
{
	if (it == end)
		return *this;
	if (SemanticInformation::altersControlFlow(*it) && *it != AssemblyItem(eth::Instruction::JUMPI))
		it = end;
	else
	{
		++it;
		while (it != end && it->type() == Tag)
			++it;
	}
	return *this;
}

AssemblyItem const& BlockDeduplicator::BlockIterator::operator*() const
{
	if (replaceItem && replaceWith && *it == *replaceItem)
		return *replaceWith;
	else
		return *it;
}
