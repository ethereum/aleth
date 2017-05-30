// do not optimize
pragma solidity ^0.4.0;

contract mop {
	function mop() {
		uint r;
		for (uint i = 0; i < 2000000; ++i) {
			assembly {
				0xc1516ebae141a8d5b81c5814d895c3dd
				0x4841f4a7dfb6331a8964899d18e4b715
				0xc1516ebae141a8d5b81c5814d895c3dd
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				pop
				dup2
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				pop
				dup2
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				pop
				dup2
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				pop
				dup2
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				pop
				dup2
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				pop
				dup2
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				pop
				dup2
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
				=: r
				pop
				pop
			}
		}
		if (r != 0xc1516ebae141a8d5b81c5814d895c3dd)
			throw;
	}
}
