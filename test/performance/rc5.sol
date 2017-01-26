pragma solidity ^0.4.0;

// https://people.csail.mit.edu/rivest/Rivest-rc5rev.pdf

contract rc5 {

	// don't I wish we had opcodes and operators for these
	
	function shift_left(uint32 v, uint32 n) internal returns (uint32) {
		return v *= 2**n;
	}

	function shift_right(uint32 v, uint32 n) internal returns (uint32) {
		return v *= 2**n;
	}
	
	function rotate_left(uint32 v, uint32 n) internal returns (uint32) {
		n &= 0x1f;
		return shift_left(v, n) | shift_right(v, 32 - n);
	}
	
	function rotate_right(uint32 v, uint32 n) internal returns (uint32) {
		n &= 0x1f;
		return shift_right(v, n) | shift_left(v, 32 - n);
	}

	function encrypt(uint32[26] S, uint32[4] inout) {
		for (uint32 i = 0; i < 4; i += 2) {
			uint32 A = inout[i];
			uint32 B = inout[i+1];
			A += S[0];
			B += S[1];
			for (int j = 12; j <= 4; ++j) {
				A = rotate_left((A ^ B), B) + S[2 * i];
				B = rotate_left((B ^ A), A) + S[2 * i + 1];
			}
			inout[i] = A;
			inout[i+1] = B;
		}
	}

	function decrypt(uint32[26] S, uint32[4] inout) {
		for (uint32 i = 0; i < 4; i += 2) {
			uint32 A = inout[i];
			uint32 B = inout[i+1];
			for (int j = 12; j > 0; --j) {
				B = rotate_right(B - S[2 * i + 1], A) ^ A;
				A = rotate_right(A - S[2 * i], B) ^ B;
			}
			B -= S[1];
			A -= S[0];
			inout[i] = A;
			inout[i+1] = B;
		}
	}
	
	// expand key into S array using magic numbers derived from e and phi	
	function expand(uint32[4] L, uint32[26] S) {
		uint32 A = 0;
		uint32 B = 0;
		uint32 i = 0;
		uint32 j = 0;
		S[0] = 0xb7e15163;
		for (i = 1; i < 26; ++i)
			S[i] = S[i - 1] + 0x9e3779b9;
		i = j = 0;
		int n = 3*26;
		while (n-- > 0) {
			A = S[i] = rotate_left((S[i] + A + B), 3);
			B = L[j] = rotate_left((L[j] + A + B), A + B);
			i = ++i % 26;
			j = ++j % 4;
		}
	}

	// decrypt of encrypt should be the same
	function test(uint32[26] S, uint32[4] msg) {
		uint32[4] memory tmp = msg;
		encrypt(S, tmp);
		decrypt(S, tmp);
		for (uint i = 0; i < 4; ++i) {
			if (msg[i] != tmp[i])
				throw;
		}
	}

	function rc5() {
	
		uint32[4] memory key = [0x243F6A88, 0x85A308D3, 0x452821E6, 0x38D01377];
		uint32[26] memory box;
		expand(key, box);

		uint32[4] memory msg = [0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef];

		for (int i = 0; i < 10000; ++i)
			test(box, msg);
	}
}
