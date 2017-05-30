// Do not optimize.128
pragma solidity ^0.4.0;

contract sub128 {
	function sub128() {
		uint r;
 		for (uint i = 0; i < 2000000; ++i) {  // SUB by half gives same subtrahend
			assembly {
				0x4dde30faaacdc14d00327aac314e915d
				0x9bbc61f5559b829a0064f558629d22ba
				0x4dde30faaacdc14d00327aac314e915d
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
		if (r != 0x4dde30faaacdc14d00327aac314e915d)
			throw;
	}
}
