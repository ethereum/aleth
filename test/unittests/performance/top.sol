// do not optimize
pragma solidity ^0.4.0;

contract top {
	function top() {
		uint r;
		assembly {
			0
			2
			div
			=: r
		}
		if (r != 0)
			throw;
	}
}
