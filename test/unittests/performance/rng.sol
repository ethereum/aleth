pragma solidity ^0.4.0;

contract rng {

	// not a good RNG, just enough 256-bit operations to dominate loop overhead
	uint rand;
	function test() returns (uint) {
		
		uint rand1 = 134217728;
		for (int i = 0; i < 1048576; ++i) {
			rand1 *= 0x16b8ce501af6621e7e3f4e366d6c967a6667d65c329d9ac2a7e35db2f092073;
			rand1 += 0x1b036d1e2832839b693e9c83d786056b977602ef8a354592b;
		}
		return rand1;
	}
	function rng() {
		rand = test();
	}
}
