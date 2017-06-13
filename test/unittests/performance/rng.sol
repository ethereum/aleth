pragma solidity ^0.4.0;

contract rng {

	// not a good RNG, just enough 256-bit operations to dominate loop overhead
	uint rand;
	function test() returns (uint) {
		
		uint rand1 = 0x13bc89fed82f60a73;
		uint rand2 = 0x18f36bbc46ac3dc13cfd37825;
		uint rand3 = 0x15681eefd0b362a892bed8d6f605b1997;
		uint rand4 = 0x1e96f1ffe206d52abac9fcdac9bb17814cb748a6b5;
		for (int i = 0; i < 5000000; ++i) {
			rand1 *= 0x16b8ce501af6621e7e3f4e366d6c967a6667d65c329d9ac2a7e35db2f092073;
			rand1 += 0x1b036d1e2832839b693e9c83d786056b977602ef8a354592b;
			rand2 *= 0xfd4d51ba81bcb0cb635868f6e3dbffb10bd4734bdc4160bcd2ff9665868ad6cf;
			rand2 += 0x13d31725f8e66d8f22ad1c138ded01b17a7bb376a52d7606b4d1e60f9;
			rand3 *= 0xfd4d51ba81bcb0cb635868f6e3dbffb10bd4734bdc4160bcd2ff9665868ad6cf;
			rand3 += 0x91d73316855198e20240e120e592ce37d8642ebc4d8f6835a70561fcc3dd;
			rand4 *= 0x85b97faf7f8511f3173347ea124749fb69335f577897cc4550e16888749aa7;
			rand4 += 0xb19bb307991f810548aac2ebe127f0d19e8b2fc09b0bc88c4773b2dce8bbbc91;
		}
		return rand1 ^ rand2 ^ rand3 ^ rand4;
	}
	function rng() {
		rand = test();
	}
}
