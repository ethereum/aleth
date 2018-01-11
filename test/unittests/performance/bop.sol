// do not optimize
pragma solidity ^0.4.0;

contract bop {
	function bop() {
		uint r;
		assembly {
			2
			1
			dup2
			pop
			pop
			=: r
		}
		if (r != 2)
			throw;
	}
}
