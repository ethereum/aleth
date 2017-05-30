// Do not optimize.
pragma solidity ^0.4.0;

contract add64 {
	function add64() {
		uint r;
		for (uint i = 0; i < 2000000; ++i) {  // 64 ADDs to 64-bit value
			assembly {
				0xffffffff
				0xfd37f3e2bba2c4f
				0xffffffff
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
		if (r != 0xfd37f3e3bba2c4ef)
			throw;
	}
}