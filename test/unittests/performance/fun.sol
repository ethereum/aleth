pragma solidity ^0.4.0;

contract base {

	function nada(uint u) internal returns (uint) {
		return u;
	}
}

contract derived is base {
	function nada(uint u) internal returns (uint) {
		return base.nada(u);
	}
}

contract fun is derived {

	function useless(uint u) returns (uint) {
		return nada(u);
	}
	
	function test() {
		uint u = 8;
		for (int i = 0; i < 13421773; ++i)
			u = nada(useless(u));
		assert(u == 8);
	}

	function fun() {
		test();
	}
}
