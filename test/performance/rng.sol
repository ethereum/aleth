pragma solidity ^0.4.0;

contract rng {

	 // not a good RNG, just enough 256-bit operations to dominate loop overhead
	uint rand;
	function test() returns (uint) {
		 uint rand1 = 3;
		 uint rand2 = 5;
		 uint rand3 = 7;
		 uint rand4 = 11;
		 for (int i = 0; i < 1000000; ++i) {
			 rand1 = 0x243F6A8885A308D3 * rand1 + 0x13198A2E03707344;
			 rand2 = 0xA4093822299F31D0 * rand2 + 0x082EFA98EC4E6C89;
			 rand3 = 0x452821E638D01377 * rand3 + 0xBE5466CF34E90C6C;
			 rand4 = 0xC0AC29B7C97C50DD * rand3 + 0x3F84D5B5B5470917;
		 }
		 return rand1 ^ rand2 ^ rand3 ^ rand4;
		 return rand;
	}
	function rng() {
		rand = test();
	}
}
