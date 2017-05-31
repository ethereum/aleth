// do not optimize
pragma solidity ^0.4.0;

contract pop {
	function pop() {
		uint r;
		for (uint i = 0; i < 2000000; ++i) {
			assembly {
				1
				2
				1
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
		if (r != 1)
			throw;
	}
}
