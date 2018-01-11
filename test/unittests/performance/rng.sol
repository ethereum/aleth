pragma solidity ^0.4.0;

contract rng {

	// 2**20 tests
	function rng() {
	
		// magic seeds
		uint blum = 0x20fa78d1fb3d86d2b02d955ec04ed801
				  * 0x2fc84408f87e831d280a05613124dc35;
		uint[5] memory shub = [uint(0xa96d80548950c6e1fd4299395ee381a3),
									0xec36bfc9cfdfcbf201fd78ee4cdad4fb,
		                            0xdba29b85a759fb10848f01256f2b5c39,
		                            0xa9390182a3a79447c8394ae189edcfc1,
		                            0xd66ec8a137fb01fae6b7a83c83d0adf9];

		// 2**20 / 32
		for (uint i = 0; i < 32768; ++i) {
			uint[5] memory out = [uint256(0),0,0,0,0];

			// need 32 bytes of output for each log argument
			for (uint j = 1; j< 32; ++j) {
			
				// harvest low order bytes of 5 Blum Blum Shubs for 5-word log output
				for (uint k = 0; k < 5; ++k) {
					uint sqr = shub[k];
					sqr *= sqr;
					sqr %= blum;
					shub[k] = sqr;
					out[k] |= (sqr & 255) << j;
				}
			}
// parity chokes	log4(bytes32(out[0]), bytes32(out[1]), bytes32(out[2]), bytes32(out[3]), bytes32(out[4]));
		}
	}
}
