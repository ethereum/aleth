// Do not optimize.
pragma solidity ^0.4.0;

contract sub64 {
	function sub64() {
		uint r;
 		for (uint i = 0; i < 2000000; ++i) {  // SUB by half gives same subtrahend
			assembly {
				0x51022b6317003a9d
				0xa20456c62e00753a
				0x51022b6317003a9d
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				pop
				dup2
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				pop
				dup2
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				pop
				dup2
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				pop
				dup2
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				pop
				dup2
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				pop
				dup2
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				pop
				dup2
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				pop
				dup2
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				pop
				dup2
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
				=: r
				pop
				pop
			}
		}
		if (r != 0x51022b6317003a9d)
			throw;
	}
}
