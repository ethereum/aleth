pragma solidity ^0.4.0;

contract rng {

	 // not a good RNG, just enough 256-bit operations to dominate loop overhead
	uint rand;
	function test() returns (uint) {
		 uint rand1 = 3;
		 uint rand2 = 5;
		 uint rand3 = 7;
		 uint rand4 = 11;
		 for (int i = 0; i < 5000000; ++i) {
			 rand1 = 0x9924b2a26908c1b1 * rand1 + 0xf5511c50ab888555;
			 rand2 = 0xeeba8a600791d63f * rand2 + 0x973ee10a1ef0ae9f;
			 rand3 = 0xd22a58b2dc9afccd * rand3 + 0xd489101af4629b47;
			 rand4 = 0xf9491f202b4ccca3 * rand4 + 0xb39041b8e35cac13;
			 rand1 = 0x9924b2a26908c1b1 * rand1 + 0xf5511c50ab888555;
			 rand2 = 0xeeba8a600791d63f * rand2 + 0x973ee10a1ef0ae9f;
			 rand3 = 0xd22a58b2dc9afccd * rand3 + 0xd489101af4629b47;
			 rand4 = 0xf9491f202b4ccca3 * rand4 + 0xb39041b8e35cac13;
			 rand1 = 0x9924b2a26908c1b1 * rand1 + 0xf5511c50ab888555;
			 rand2 = 0xeeba8a600791d63f * rand2 + 0x973ee10a1ef0ae9f;
			 rand3 = 0xd22a58b2dc9afccd * rand3 + 0xd489101af4629b47;
			 rand4 = 0xf9491f202b4ccca3 * rand4 + 0xb39041b8e35cac13;
			 rand1 = 0x9924b2a26908c1b1 * rand1 + 0xf5511c50ab888555;
			 rand2 = 0xeeba8a600791d63f * rand2 + 0x973ee10a1ef0ae9f;
			 rand3 = 0xd22a58b2dc9afccd * rand3 + 0xd489101af4629b47;
			 rand4 = 0xf9491f202b4ccca3 * rand4 + 0xb39041b8e35cac13;
		 }
		 return rand1 ^ rand2 ^ rand3 ^ rand4;
	}
	function rng() {
		rand = test();
	}
}
