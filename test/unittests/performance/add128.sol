// Do not optimize.
pragma solidity ^0.4.0;

contract add128 {
	function add128() {
		uint r;
		for (uint i = 0; i < 2000000; ++i) {  // 64 ADDs to 128-bit value
			assembly {
				0xffffffffffffffff
				0xf5470b43c6549b016288e9a65629687
				0xffffffffffffffff
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				pop
				dup2
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				pop
				dup2
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				pop
				dup2
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				pop
				dup2
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				pop
				dup2
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				pop
				dup2
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				pop
				dup2
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				pop
				dup2
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				pop
				dup2
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
				=: r
				pop
				pop
			}
		}
		if (r != 0xf5470b43c6549b026288e9a65629686f)
			throw;
	}
}