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
 * @author Christian <c@ethdev.com>
 * @date 2014
 * Tests for the Solidity optimizer.
 */

#include <string>
#include <tuple>
#include <memory>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <test/libsolidity/solidityExecutionFramework.h>
#include <libevmasm/CommonSubexpressionEliminator.h>
#include <libevmasm/ControlFlowGraph.h>
#include <libevmasm/Assembly.h>
#include <libevmasm/BlockDeduplicator.h>

using namespace std;
using namespace dev::eth;

namespace dev
{
namespace solidity
{
namespace test
{

class OptimizerTestFramework: public ExecutionFramework
{
public:
	OptimizerTestFramework() { }
	/// Compiles the source code with and without optimizing.
	void compileBothVersions(
		std::string const& _sourceCode,
		u256 const& _value = 0,
		std::string const& _contractName = ""
	)
	{
		m_optimize = false;
		bytes nonOptimizedBytecode = compileAndRun(_sourceCode, _value, _contractName);
		m_nonOptimizedContract = m_contractAddress;
		m_optimize = true;
		bytes optimizedBytecode = compileAndRun(_sourceCode, _value, _contractName);
		size_t nonOptimizedSize = 0;
		eth::eachInstruction(nonOptimizedBytecode, [&](Instruction, u256 const&) {
			nonOptimizedSize++;
		});
		size_t optimizedSize = 0;
		eth::eachInstruction(optimizedBytecode, [&](Instruction, u256 const&) {
			optimizedSize++;
		});
		BOOST_CHECK_MESSAGE(
			nonOptimizedSize > optimizedSize,
			"Optimizer did not reduce bytecode size."
		);
		m_optimizedContract = m_contractAddress;
	}

	template <class... Args>
	void compareVersions(std::string _sig, Args const&... _arguments)
	{
		m_contractAddress = m_nonOptimizedContract;
		bytes nonOptimizedOutput = callContractFunction(_sig, _arguments...);
		m_contractAddress = m_optimizedContract;
		bytes optimizedOutput = callContractFunction(_sig, _arguments...);
		BOOST_CHECK_MESSAGE(nonOptimizedOutput == optimizedOutput, "Computed values do not match."
							"\nNon-Optimized: " + toHex(nonOptimizedOutput) +
							"\nOptimized:     " + toHex(optimizedOutput));
	}

	AssemblyItems addDummyLocations(AssemblyItems const& _input)
	{
		// add dummy locations to each item so that we can check that they are not deleted
		AssemblyItems input = _input;
		for (AssemblyItem& item: input)
			item.setLocation(SourceLocation(1, 3, make_shared<string>("")));
		return input;
	}

	eth::KnownState createInitialState(AssemblyItems const& _input)
	{
		eth::KnownState state;
		for (auto const& item: addDummyLocations(_input))
			state.feedItem(item, true);
		return state;
	}

	AssemblyItems getCSE(AssemblyItems const& _input, eth::KnownState const& _state = eth::KnownState())
	{
		AssemblyItems input = addDummyLocations(_input);

		eth::CommonSubexpressionEliminator cse(_state);
		BOOST_REQUIRE(cse.feedItems(input.begin(), input.end()) == input.end());
		AssemblyItems output = cse.getOptimizedItems();

		for (AssemblyItem const& item: output)
		{
			BOOST_CHECK(item == Instruction::POP || !item.getLocation().isEmpty());
		}
		return output;
	}

	void checkCSE(
		AssemblyItems const& _input,
		AssemblyItems const& _expectation,
		KnownState const& _state = eth::KnownState()
	)
	{
		AssemblyItems output = getCSE(_input, _state);
		BOOST_CHECK_EQUAL_COLLECTIONS(_expectation.begin(), _expectation.end(), output.begin(), output.end());
	}

	AssemblyItems getCFG(AssemblyItems const& _input)
	{
		AssemblyItems output = _input;
		// Running it four times should be enough for these tests.
		for (unsigned i = 0; i < 4; ++i)
		{
			ControlFlowGraph cfg(output);
			AssemblyItems optItems;
			for (BasicBlock const& block: cfg.optimisedBlocks())
				copy(output.begin() + block.begin, output.begin() + block.end,
					 back_inserter(optItems));
			output = move(optItems);
		}
		return output;
	}

	void checkCFG(AssemblyItems const& _input, AssemblyItems const& _expectation)
	{
		AssemblyItems output = getCFG(_input);
		BOOST_CHECK_EQUAL_COLLECTIONS(_expectation.begin(), _expectation.end(), output.begin(), output.end());
	}

protected:
	Address m_optimizedContract;
	Address m_nonOptimizedContract;
};

BOOST_FIXTURE_TEST_SUITE(SolidityOptimizer, OptimizerTestFramework)

BOOST_AUTO_TEST_CASE(smoke_test)
{
	char const* sourceCode = R"(
		contract test {
			function f(uint a) returns (uint b) {
				return a;
			}
		})";
	compileBothVersions(sourceCode);
	compareVersions("f(uint256)", u256(7));
}

BOOST_AUTO_TEST_CASE(identities)
{
	char const* sourceCode = R"(
		contract test {
			function f(int a) returns (int b) {
				return int(0) | (int(1) * (int(0) ^ (0 + a)));
			}
		})";
	compileBothVersions(sourceCode);
	compareVersions("f(uint256)", u256(0x12334664));
}

BOOST_AUTO_TEST_CASE(unused_expressions)
{
	char const* sourceCode = R"(
		contract test {
			uint data;
			function f() returns (uint a, uint b) {
				10 + 20;
				data;
			}
		})";
	compileBothVersions(sourceCode);
	compareVersions("f()");
}

BOOST_AUTO_TEST_CASE(constant_folding_both_sides)
{
	// if constants involving the same associative and commutative operator are applied from both
	// sides, the operator should be applied only once, because the expression compiler pushes
	// literals as late as possible
	char const* sourceCode = R"(
		contract test {
			function f(uint x) returns (uint y) {
				return 98 ^ (7 * ((1 | (x | 1000)) * 40) ^ 102);
			}
		})";
	compileBothVersions(sourceCode);
	compareVersions("f(uint256)");
}

BOOST_AUTO_TEST_CASE(storage_access)
{
	char const* sourceCode = R"(
		contract test {
			uint8[40] data;
			function f(uint x) returns (uint y) {
				data[2] = data[7] = uint8(x);
				data[4] = data[2] * 10 + data[3];
			}
		}
	)";
	compileBothVersions(sourceCode);
	compareVersions("f(uint256)");
}

BOOST_AUTO_TEST_CASE(array_copy)
{
	char const* sourceCode = R"(
		contract test {
			bytes2[] data1;
			bytes5[] data2;
			function f(uint x) returns (uint l, uint y) {
				for (uint i = 0; i < msg.data.length; ++i)
					data1[i] = msg.data[i];
				data2 = data1;
				l = data2.length;
				y = uint(data2[x]);
			}
		}
	)";
	compileBothVersions(sourceCode);
	compareVersions("f(uint256)", 0);
	compareVersions("f(uint256)", 10);
	compareVersions("f(uint256)", 36);
}

BOOST_AUTO_TEST_CASE(function_calls)
{
	char const* sourceCode = R"(
		contract test {
			function f1(uint x) returns (uint) { return x*x; }
			function f(uint x) returns (uint) { return f1(7+x) - this.f1(x**9); }
		}
	)";
	compileBothVersions(sourceCode);
	compareVersions("f(uint256)", 0);
	compareVersions("f(uint256)", 10);
	compareVersions("f(uint256)", 36);
}

BOOST_AUTO_TEST_CASE(storage_write_in_loops)
{
	char const* sourceCode = R"(
		contract test {
			uint d;
			function f(uint a) returns (uint r) {
				var x = d;
				for (uint i = 1; i < a * a; i++) {
					r = d;
					d = i;
				}

			}
		}
	)";
	compileBothVersions(sourceCode);
	compareVersions("f(uint256)", 0);
	compareVersions("f(uint256)", 10);
	compareVersions("f(uint256)", 36);
}

BOOST_AUTO_TEST_CASE(retain_information_in_branches)
{
	// This tests that the optimizer knows that we already have "z == sha3(y)" inside both branches.
	char const* sourceCode = R"(
		contract c {
			bytes32 d;
			uint a;
			function f(uint x, bytes32 y) returns (uint r_a, bytes32 r_d) {
				bytes32 z = sha3(y);
				if (x > 8) {
					z = sha3(y);
					a = x;
				} else {
					z = sha3(y);
					a = x;
				}
				r_a = a;
				r_d = d;
			}
		}
	)";
	compileBothVersions(sourceCode);
	compareVersions("f(uint256,bytes32)", 0, "abc");
	compareVersions("f(uint256,bytes32)", 8, "def");
	compareVersions("f(uint256,bytes32)", 10, "ghi");

	m_optimize = true;
	bytes optimizedBytecode = compileAndRun(sourceCode, 0, "c");
	size_t numSHA3s = 0;
	eth::eachInstruction(optimizedBytecode, [&](Instruction _instr, u256 const&) {
		if (_instr == eth::Instruction::SHA3)
			numSHA3s++;
	});
	BOOST_CHECK_EQUAL(1, numSHA3s);
}

BOOST_AUTO_TEST_CASE(store_tags_as_unions)
{
	// This calls the same function from two sources and both calls have a certain sha3 on
	// the stack at the same position.
	// Without storing tags as unions, the return from the shared function would not know where to
	// jump and thus all jumpdests are forced to clear their state and we do not know about the
	// sha3 anymore.
	// Note that, for now, this only works if the functions have the same number of return
	// parameters since otherwise, the return jump addresses are at different stack positions
	// which triggers the "unknown jump target" situation.
	char const* sourceCode = R"(
		contract test {
			bytes32 data;
			function f(uint x, bytes32 y) external returns (uint r_a, bytes32 r_d) {
				r_d = sha3(y);
				shared(y);
				r_d = sha3(y);
				r_a = 5;
			}
			function g(uint x, bytes32 y) external returns (uint r_a, bytes32 r_d) {
				r_d = sha3(y);
				shared(y);
				r_d = bytes32(uint(sha3(y)) + 2);
				r_a = 7;
			}
			function shared(bytes32 y) internal {
				data = sha3(y);
			}
		}
	)";
	compileBothVersions(sourceCode);
	compareVersions("f()", 7, "abc");

	m_optimize = true;
	bytes optimizedBytecode = compileAndRun(sourceCode, 0, "test");
	size_t numSHA3s = 0;
	eth::eachInstruction(optimizedBytecode, [&](Instruction _instr, u256 const&) {
		if (_instr == eth::Instruction::SHA3)
			numSHA3s++;
	});
// TEST DISABLED UNTIL 93693404 IS IMPLEMENTED
//	BOOST_CHECK_EQUAL(2, numSHA3s);
}

BOOST_AUTO_TEST_CASE(cse_intermediate_swap)
{
	eth::KnownState state;
	eth::CommonSubexpressionEliminator cse(state);
	AssemblyItems input{
		Instruction::SWAP1, Instruction::POP, Instruction::ADD, u256(0), Instruction::SWAP1,
		Instruction::SLOAD, Instruction::SWAP1, u256(100), Instruction::EXP, Instruction::SWAP1,
		Instruction::DIV, u256(0xff), Instruction::AND
	};
	BOOST_REQUIRE(cse.feedItems(input.begin(), input.end()) == input.end());
	AssemblyItems output = cse.getOptimizedItems();
	BOOST_CHECK(!output.empty());
}

BOOST_AUTO_TEST_CASE(cse_negative_stack_access)
{
	AssemblyItems input{Instruction::DUP2, u256(0)};
	checkCSE(input, input);
}

BOOST_AUTO_TEST_CASE(cse_negative_stack_end)
{
	AssemblyItems input{Instruction::ADD};
	checkCSE(input, input);
}

BOOST_AUTO_TEST_CASE(cse_intermediate_negative_stack)
{
	AssemblyItems input{Instruction::ADD, u256(1), Instruction::DUP1};
	checkCSE(input, input);
}

BOOST_AUTO_TEST_CASE(cse_pop)
{
	checkCSE({Instruction::POP}, {Instruction::POP});
}

BOOST_AUTO_TEST_CASE(cse_unneeded_items)
{
	AssemblyItems input{
		Instruction::ADD,
		Instruction::SWAP1,
		Instruction::POP,
		u256(7),
		u256(8),
	};
	checkCSE(input, input);
}

BOOST_AUTO_TEST_CASE(cse_constant_addition)
{
	AssemblyItems input{u256(7), u256(8), Instruction::ADD};
	checkCSE(input, {u256(7 + 8)});
}

BOOST_AUTO_TEST_CASE(cse_invariants)
{
	AssemblyItems input{
		Instruction::DUP1,
		Instruction::DUP1,
		u256(0),
		Instruction::OR,
		Instruction::OR
	};
	checkCSE(input, {Instruction::DUP1});
}

BOOST_AUTO_TEST_CASE(cse_subself)
{
	checkCSE({Instruction::DUP1, Instruction::SUB}, {Instruction::POP, u256(0)});
}

BOOST_AUTO_TEST_CASE(cse_subother)
{
	checkCSE({Instruction::SUB}, {Instruction::SUB});
}

BOOST_AUTO_TEST_CASE(cse_double_negation)
{
	checkCSE({Instruction::DUP5, Instruction::NOT, Instruction::NOT}, {Instruction::DUP5});
}

BOOST_AUTO_TEST_CASE(cse_double_iszero)
{
	checkCSE({Instruction::GT, Instruction::ISZERO, Instruction::ISZERO}, {Instruction::GT});
	checkCSE({Instruction::GT, Instruction::ISZERO}, {Instruction::GT, Instruction::ISZERO});
	checkCSE(
		{Instruction::ISZERO, Instruction::ISZERO, Instruction::ISZERO},
		{Instruction::ISZERO}
	);
}

BOOST_AUTO_TEST_CASE(cse_associativity)
{
	AssemblyItems input{
		Instruction::DUP1,
		Instruction::DUP1,
		u256(0),
		Instruction::OR,
		Instruction::OR
	};
	checkCSE(input, {Instruction::DUP1});
}

BOOST_AUTO_TEST_CASE(cse_associativity2)
{
	AssemblyItems input{
		u256(0),
		Instruction::DUP2,
		u256(2),
		u256(1),
		Instruction::DUP6,
		Instruction::ADD,
		u256(2),
		Instruction::ADD,
		Instruction::ADD,
		Instruction::ADD,
		Instruction::ADD
	};
	checkCSE(input, {Instruction::DUP2, Instruction::DUP2, Instruction::ADD, u256(5), Instruction::ADD});
}

BOOST_AUTO_TEST_CASE(cse_storage)
{
	AssemblyItems input{
		u256(0),
		Instruction::SLOAD,
		u256(0),
		Instruction::SLOAD,
		Instruction::ADD,
		u256(0),
		Instruction::SSTORE
	};
	checkCSE(input, {
		u256(0),
		Instruction::DUP1,
		Instruction::SLOAD,
		Instruction::DUP1,
		Instruction::ADD,
		Instruction::SWAP1,
		Instruction::SSTORE
	});
}

BOOST_AUTO_TEST_CASE(cse_noninterleaved_storage)
{
	// two stores to the same location should be replaced by only one store, even if we
	// read in the meantime
	AssemblyItems input{
		u256(7),
		Instruction::DUP2,
		Instruction::SSTORE,
		Instruction::DUP1,
		Instruction::SLOAD,
		u256(8),
		Instruction::DUP3,
		Instruction::SSTORE
	};
	checkCSE(input, {
		u256(8),
		Instruction::DUP2,
		Instruction::SSTORE,
		u256(7)
	});
}

BOOST_AUTO_TEST_CASE(cse_interleaved_storage)
{
	// stores and reads to/from two unknown locations, should not optimize away the first store
	AssemblyItems input{
		u256(7),
		Instruction::DUP2,
		Instruction::SSTORE, // store to "DUP1"
		Instruction::DUP2,
		Instruction::SLOAD, // read from "DUP2", might be equal to "DUP1"
		u256(0),
		Instruction::DUP3,
		Instruction::SSTORE // store different value to "DUP1"
	};
	checkCSE(input, input);
}

BOOST_AUTO_TEST_CASE(cse_interleaved_storage_same_value)
{
	// stores and reads to/from two unknown locations, should not optimize away the first store
	// but it should optimize away the second, since we already know the value will be the same
	AssemblyItems input{
		u256(7),
		Instruction::DUP2,
		Instruction::SSTORE, // store to "DUP1"
		Instruction::DUP2,
		Instruction::SLOAD, // read from "DUP2", might be equal to "DUP1"
		u256(6),
		u256(1),
		Instruction::ADD,
		Instruction::DUP3,
		Instruction::SSTORE // store same value to "DUP1"
	};
	checkCSE(input, {
		u256(7),
		Instruction::DUP2,
		Instruction::SSTORE,
		Instruction::DUP2,
		Instruction::SLOAD
	});
}

BOOST_AUTO_TEST_CASE(cse_interleaved_storage_at_known_location)
{
	// stores and reads to/from two known locations, should optimize away the first store,
	// because we know that the location is different
	AssemblyItems input{
		u256(0x70),
		u256(1),
		Instruction::SSTORE, // store to 1
		u256(2),
		Instruction::SLOAD, // read from 2, is different from 1
		u256(0x90),
		u256(1),
		Instruction::SSTORE // store different value at 1
	};
	checkCSE(input, {
		u256(2),
		Instruction::SLOAD,
		u256(0x90),
		u256(1),
		Instruction::SSTORE
	});
}

BOOST_AUTO_TEST_CASE(cse_interleaved_storage_at_known_location_offset)
{
	// stores and reads to/from two locations which are known to be different,
	// should optimize away the first store, because we know that the location is different
	AssemblyItems input{
		u256(0x70),
		Instruction::DUP2,
		u256(1),
		Instruction::ADD,
		Instruction::SSTORE, // store to "DUP1"+1
		Instruction::DUP1,
		u256(2),
		Instruction::ADD,
		Instruction::SLOAD, // read from "DUP1"+2, is different from "DUP1"+1
		u256(0x90),
		Instruction::DUP3,
		u256(1),
		Instruction::ADD,
		Instruction::SSTORE // store different value at "DUP1"+1
	};
	checkCSE(input, {
		u256(2),
		Instruction::DUP2,
		Instruction::ADD,
		Instruction::SLOAD,
		u256(0x90),
		u256(1),
		Instruction::DUP4,
		Instruction::ADD,
		Instruction::SSTORE
	});
}

BOOST_AUTO_TEST_CASE(cse_interleaved_memory_at_known_location_offset)
{
	// stores and reads to/from two locations which are known to be different,
	// should not optimize away the first store, because the location overlaps with the load,
	// but it should optimize away the second, because we know that the location is different by 32
	AssemblyItems input{
		u256(0x50),
		Instruction::DUP2,
		u256(2),
		Instruction::ADD,
		Instruction::MSTORE, // ["DUP1"+2] = 0x50
		u256(0x60),
		Instruction::DUP2,
		u256(32),
		Instruction::ADD,
		Instruction::MSTORE, // ["DUP1"+32] = 0x60
		Instruction::DUP1,
		Instruction::MLOAD, // read from "DUP1"
		u256(0x70),
		Instruction::DUP3,
		u256(32),
		Instruction::ADD,
		Instruction::MSTORE, // ["DUP1"+32] = 0x70
		u256(0x80),
		Instruction::DUP3,
		u256(2),
		Instruction::ADD,
		Instruction::MSTORE, // ["DUP1"+2] = 0x80
	};
	// If the actual code changes too much, we could also simply check that the output contains
	// exactly 3 MSTORE and exactly 1 MLOAD instruction.
	checkCSE(input, {
		u256(0x50),
		u256(2),
		Instruction::DUP3,
		Instruction::ADD,
		Instruction::SWAP1,
		Instruction::DUP2,
		Instruction::MSTORE, // ["DUP1"+2] = 0x50
		Instruction::DUP2,
		Instruction::MLOAD, // read from "DUP1"
		u256(0x70),
		u256(32),
		Instruction::DUP5,
		Instruction::ADD,
		Instruction::MSTORE, // ["DUP1"+32] = 0x70
		u256(0x80),
		Instruction::SWAP1,
		Instruction::SWAP2,
		Instruction::MSTORE // ["DUP1"+2] = 0x80
	});
}

BOOST_AUTO_TEST_CASE(cse_deep_stack)
{
	AssemblyItems input{
		Instruction::ADD,
		Instruction::SWAP1,
		Instruction::POP,
		Instruction::SWAP8,
		Instruction::POP,
		Instruction::SWAP8,
		Instruction::POP,
		Instruction::SWAP8,
		Instruction::SWAP5,
		Instruction::POP,
		Instruction::POP,
		Instruction::POP,
		Instruction::POP,
		Instruction::POP,
	};
	checkCSE(input, {
		Instruction::SWAP4,
		Instruction::SWAP12,
		Instruction::SWAP3,
		Instruction::SWAP11,
		Instruction::POP,
		Instruction::SWAP1,
		Instruction::SWAP3,
		Instruction::ADD,
		Instruction::SWAP8,
		Instruction::POP,
		Instruction::SWAP6,
		Instruction::POP,
		Instruction::POP,
		Instruction::POP,
		Instruction::POP,
		Instruction::POP,
		Instruction::POP,
	});
}

BOOST_AUTO_TEST_CASE(cse_jumpi_no_jump)
{
	AssemblyItems input{
		u256(0),
		u256(1),
		Instruction::DUP2,
		AssemblyItem(PushTag, 1),
		Instruction::JUMPI
	};
	checkCSE(input, {
		u256(0),
		u256(1)
	});
}

BOOST_AUTO_TEST_CASE(cse_jumpi_jump)
{
	AssemblyItems input{
		u256(1),
		u256(1),
		Instruction::DUP2,
		AssemblyItem(PushTag, 1),
		Instruction::JUMPI
	};
	checkCSE(input, {
		u256(1),
		Instruction::DUP1,
		AssemblyItem(PushTag, 1),
		Instruction::JUMP
	});
}

BOOST_AUTO_TEST_CASE(cse_empty_sha3)
{
	AssemblyItems input{
		u256(0),
		Instruction::DUP2,
		Instruction::SHA3
	};
	checkCSE(input, {
		u256(sha3(bytesConstRef()))
	});
}

BOOST_AUTO_TEST_CASE(cse_partial_sha3)
{
	AssemblyItems input{
		u256(0xabcd) << (256 - 16),
		u256(0),
		Instruction::MSTORE,
		u256(2),
		u256(0),
		Instruction::SHA3
	};
	checkCSE(input, {
		u256(0xabcd) << (256 - 16),
		u256(0),
		Instruction::MSTORE,
		u256(sha3(bytes{0xab, 0xcd}))
	});
}

BOOST_AUTO_TEST_CASE(cse_sha3_twice_same_location)
{
	// sha3 twice from same dynamic location
	AssemblyItems input{
		Instruction::DUP2,
		Instruction::DUP1,
		Instruction::MSTORE,
		u256(64),
		Instruction::DUP2,
		Instruction::SHA3,
		u256(64),
		Instruction::DUP3,
		Instruction::SHA3
	};
	checkCSE(input, {
		Instruction::DUP2,
		Instruction::DUP1,
		Instruction::MSTORE,
		u256(64),
		Instruction::DUP2,
		Instruction::SHA3,
		Instruction::DUP1
	});
}

BOOST_AUTO_TEST_CASE(cse_sha3_twice_same_content)
{
	// sha3 twice from different dynamic location but with same content
	AssemblyItems input{
		Instruction::DUP1,
		u256(0x80),
		Instruction::MSTORE, // m[128] = DUP1
		u256(0x20),
		u256(0x80),
		Instruction::SHA3, // sha3(m[128..(128+32)])
		Instruction::DUP2,
		u256(12),
		Instruction::MSTORE, // m[12] = DUP1
		u256(0x20),
		u256(12),
		Instruction::SHA3 // sha3(m[12..(12+32)])
	};
	checkCSE(input, {
		u256(0x80),
		Instruction::DUP2,
		Instruction::DUP2,
		Instruction::MSTORE,
		u256(0x20),
		Instruction::SWAP1,
		Instruction::SHA3,
		u256(12),
		Instruction::DUP3,
		Instruction::SWAP1,
		Instruction::MSTORE,
		Instruction::DUP1
	});
}

BOOST_AUTO_TEST_CASE(cse_sha3_twice_same_content_dynamic_store_in_between)
{
	// sha3 twice from different dynamic location but with same content,
	// dynamic mstore in between, which forces us to re-calculate the sha3
	AssemblyItems input{
		u256(0x80),
		Instruction::DUP2,
		Instruction::DUP2,
		Instruction::MSTORE, // m[128] = DUP1
		u256(0x20),
		Instruction::DUP1,
		Instruction::DUP3,
		Instruction::SHA3, // sha3(m[128..(128+32)])
		u256(12),
		Instruction::DUP5,
		Instruction::DUP2,
		Instruction::MSTORE, // m[12] = DUP1
		Instruction::DUP12,
		Instruction::DUP14,
		Instruction::MSTORE, // destroys memory knowledge
		Instruction::SWAP2,
		Instruction::SWAP1,
		Instruction::SWAP2,
		Instruction::SHA3 // sha3(m[12..(12+32)])
	};
	checkCSE(input, input);
}

BOOST_AUTO_TEST_CASE(cse_sha3_twice_same_content_noninterfering_store_in_between)
{
	// sha3 twice from different dynamic location but with same content,
	// dynamic mstore in between, but does not force us to re-calculate the sha3
	AssemblyItems input{
		u256(0x80),
		Instruction::DUP2,
		Instruction::DUP2,
		Instruction::MSTORE, // m[128] = DUP1
		u256(0x20),
		Instruction::DUP1,
		Instruction::DUP3,
		Instruction::SHA3, // sha3(m[128..(128+32)])
		u256(12),
		Instruction::DUP5,
		Instruction::DUP2,
		Instruction::MSTORE, // m[12] = DUP1
		Instruction::DUP12,
		u256(12 + 32),
		Instruction::MSTORE, // does not destoy memory knowledge
		Instruction::DUP13,
		u256(128 - 32),
		Instruction::MSTORE, // does not destoy memory knowledge
		u256(0x20),
		u256(12),
		Instruction::SHA3 // sha3(m[12..(12+32)])
	};
	// if this changes too often, only count the number of SHA3 and MSTORE instructions
	AssemblyItems output = getCSE(input);
	BOOST_CHECK_EQUAL(4, count(output.begin(), output.end(), AssemblyItem(Instruction::MSTORE)));
	BOOST_CHECK_EQUAL(1, count(output.begin(), output.end(), AssemblyItem(Instruction::SHA3)));
}

BOOST_AUTO_TEST_CASE(cse_with_initially_known_stack)
{
	eth::KnownState state = createInitialState(AssemblyItems{
		u256(0x12),
		u256(0x20),
		Instruction::ADD
	});
	AssemblyItems input{
		u256(0x12 + 0x20)
	};
	checkCSE(input, AssemblyItems{Instruction::DUP1}, state);
}

BOOST_AUTO_TEST_CASE(cse_equality_on_initially_known_stack)
{
	eth::KnownState state = createInitialState(AssemblyItems{Instruction::DUP1});
	AssemblyItems input{
		Instruction::EQ
	};
	AssemblyItems output = getCSE(input, state);
	// check that it directly pushes 1 (true)
	BOOST_CHECK(find(output.begin(), output.end(), AssemblyItem(u256(1))) != output.end());
}

BOOST_AUTO_TEST_CASE(cse_access_previous_sequence)
{
	// Tests that the code generator detects whether it tries to access SLOAD instructions
	// from a sequenced expression which is not in its scope.
	eth::KnownState state = createInitialState(AssemblyItems{
		u256(0),
		Instruction::SLOAD,
		u256(1),
		Instruction::ADD,
		u256(0),
		Instruction::SSTORE
	});
	// now stored: val_1 + 1 (value at sequence 1)
	// if in the following instructions, the SLOAD cresolves to "val_1 + 1",
	// this cannot be generated because we cannot load from sequence 1 anymore.
	AssemblyItems input{
		u256(0),
		Instruction::SLOAD,
	};
	BOOST_CHECK_THROW(getCSE(input, state), StackTooDeepException);
	// @todo for now, this throws an exception, but it should recover to the following
	// (or an even better version) at some point:
	// 0, SLOAD, 1, ADD, SSTORE, 0 SLOAD
}

BOOST_AUTO_TEST_CASE(cse_optimise_return)
{
	checkCSE(
		AssemblyItems{u256(0), u256(7), Instruction::RETURN},
		AssemblyItems{Instruction::STOP}
	);
}

BOOST_AUTO_TEST_CASE(control_flow_graph_remove_unused)
{
	// remove parts of the code that are unused
	AssemblyItems input{
		AssemblyItem(PushTag, 1),
		Instruction::JUMP,
		u256(7),
		AssemblyItem(Tag, 1),
	};
	checkCFG(input, {});
}

BOOST_AUTO_TEST_CASE(control_flow_graph_remove_unused_loop)
{
	AssemblyItems input{
		AssemblyItem(PushTag, 3),
		Instruction::JUMP,
		AssemblyItem(Tag, 1),
		u256(7),
		AssemblyItem(PushTag, 2),
		Instruction::JUMP,
		AssemblyItem(Tag, 2),
		u256(8),
		AssemblyItem(PushTag, 1),
		Instruction::JUMP,
		AssemblyItem(Tag, 3),
		u256(11)
	};
	checkCFG(input, {u256(11)});
}

BOOST_AUTO_TEST_CASE(control_flow_graph_reconnect_single_jump_source)
{
	// move code that has only one unconditional jump source
	AssemblyItems input{
		u256(1),
		AssemblyItem(PushTag, 1),
		Instruction::JUMP,
		AssemblyItem(Tag, 2),
		u256(2),
		AssemblyItem(PushTag, 3),
		Instruction::JUMP,
		AssemblyItem(Tag, 1),
		u256(3),
		AssemblyItem(PushTag, 2),
		Instruction::JUMP,
		AssemblyItem(Tag, 3),
		u256(4),
	};
	checkCFG(input, {u256(1), u256(3), u256(2), u256(4)});
}

BOOST_AUTO_TEST_CASE(control_flow_graph_do_not_remove_returned_to)
{
	// do not remove parts that are "returned to"
	AssemblyItems input{
		AssemblyItem(PushTag, 1),
		AssemblyItem(PushTag, 2),
		Instruction::JUMP,
		AssemblyItem(Tag, 2),
		Instruction::JUMP,
		AssemblyItem(Tag, 1),
		u256(2)
	};
	checkCFG(input, {u256(2)});
}

BOOST_AUTO_TEST_CASE(block_deduplicator)
{
	AssemblyItems input{
		AssemblyItem(PushTag, 2),
		AssemblyItem(PushTag, 1),
		AssemblyItem(PushTag, 3),
		u256(6),
		eth::Instruction::SWAP3,
		eth::Instruction::JUMP,
		AssemblyItem(Tag, 1),
		u256(6),
		eth::Instruction::SWAP3,
		eth::Instruction::JUMP,
		AssemblyItem(Tag, 2),
		u256(6),
		eth::Instruction::SWAP3,
		eth::Instruction::JUMP,
		AssemblyItem(Tag, 3)
	};
	BlockDeduplicator dedup(input);
	dedup.deduplicate();

	set<u256> pushTags;
	for (AssemblyItem const& item: input)
		if (item.type() == PushTag)
			pushTags.insert(item.data());
	BOOST_CHECK_EQUAL(pushTags.size(), 2);
}

BOOST_AUTO_TEST_CASE(block_deduplicator_loops)
{
	AssemblyItems input{
		u256(0),
		eth::Instruction::SLOAD,
		AssemblyItem(PushTag, 1),
		AssemblyItem(PushTag, 2),
		eth::Instruction::JUMPI,
		eth::Instruction::JUMP,
		AssemblyItem(Tag, 1),
		u256(5),
		u256(6),
		eth::Instruction::SSTORE,
		AssemblyItem(PushTag, 1),
		eth::Instruction::JUMP,
		AssemblyItem(Tag, 2),
		u256(5),
		u256(6),
		eth::Instruction::SSTORE,
		AssemblyItem(PushTag, 2),
		eth::Instruction::JUMP,
	};
	BlockDeduplicator dedup(input);
	dedup.deduplicate();

	set<u256> pushTags;
	for (AssemblyItem const& item: input)
		if (item.type() == PushTag)
			pushTags.insert(item.data());
	BOOST_CHECK_EQUAL(pushTags.size(), 1);
}

BOOST_AUTO_TEST_CASE(computing_constants)
{
	char const* sourceCode = R"(
		contract c {
			uint a;
			uint b;
			uint c;
			function set() returns (uint a, uint b, uint c) {
				a = 0x77abc0000000000000000000000000000000000000000000000000000000001;
				b = 0x817416927846239487123469187231298734162934871263941234127518276;
				g();
			}
			function g() {
				b = 0x817416927846239487123469187231298734162934871263941234127518276;
				c = 0x817416927846239487123469187231298734162934871263941234127518276;
			}
			function get() returns (uint ra, uint rb, uint rc) {
				ra = a;
				rb = b;
				rc = c ;
			}
		}
	)";
	compileBothVersions(sourceCode);
	compareVersions("set()");
	compareVersions("get()");

	m_optimize = true;
	m_optimizeRuns = 1;
	bytes optimizedBytecode = compileAndRun(sourceCode, 0, "c");
	bytes complicatedConstant = toBigEndian(u256("0x817416927846239487123469187231298734162934871263941234127518276"));
	unsigned occurrences = 0;
	for (auto iter = optimizedBytecode.cbegin(); iter < optimizedBytecode.cend(); ++occurrences)
		iter = search(iter, optimizedBytecode.cend(), complicatedConstant.cbegin(), complicatedConstant.cend()) + 1;
	BOOST_CHECK_EQUAL(2, occurrences);

	bytes constantWithZeros = toBigEndian(u256("0x77abc0000000000000000000000000000000000000000000000000000000001"));
	BOOST_CHECK(search(
		optimizedBytecode.cbegin(),
		optimizedBytecode.cend(),
		constantWithZeros.cbegin(),
		constantWithZeros.cend()
	) == optimizedBytecode.cend());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
} // end namespaces
