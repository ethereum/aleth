pragma solidity ^0.4.0;

contract mix {

	// Mixing Function G from BLAKE2B
	function G(uint64[16] v, uint a, uint b, uint c, uint d, uint64 x, uint64 y) constant internal {

		// Dereference to decrease memory reads
		uint64 va = v[a];
		uint64 vb = v[b];
		uint64 vc = v[c];
		uint64 vd = v[d];
		//Optimised mixing function
		assembly{
			// v[a] := (v[a] + v[b] + x) mod 2**64
			va := addmod(add(va,vb),x, 0x10000000000000000)
			//v[d] := (v[d] ^ v[a]) >>> 32
			vd := xor(div(xor(vd,va), 0x100000000), mulmod(xor(vd, va),0x100000000, 0x10000000000000000))
			//v[c] := (v[c] + v[d])     mod 2**64
			vc := addmod(vc,vd, 0x10000000000000000)
			//v[b] := (v[b] ^ v[c]) >>> 24
			vb := xor(div(xor(vb,vc), 0x1000000), mulmod(xor(vb, vc),0x10000000000, 0x10000000000000000))
			// v[a] := (v[a] + v[b] + y) mod 2**64
			va := addmod(add(va,vb),y, 0x10000000000000000)
			//v[d] := (v[d] ^ v[a]) >>> 16
			vd := xor(div(xor(vd,va), 0x10000), mulmod(xor(vd, va),0x1000000000000, 0x10000000000000000))
			//v[c] := (v[c] + v[d])     mod 2**64
			vc := addmod(vc,vd, 0x10000000000000000)
			// v[b] := (v[b] ^ v[c]) >>> 63
			vb := xor(div(xor(vb,vc), 0x8000000000000000), mulmod(xor(vb, vc),0x2, 0x10000000000000000))
		}
		
		v[a] = va;
		v[b] = vb;
		v[c] = vc;
		v[d] = vd;
	}

	function mix() {

		uint64[16] memory v = [
		   0xffffffffffffffff,
		   0xffffffffffff,
		   0xffffffff,
		   0xffff,
		   0xffffffffffffffff,
		   0xffffffffffff,
		   0xffffffff,
		   0xffff,
		   0xffffffffffffffff,
		   0xffffffffffff,
		   0xffffffff,
		   0xffff,
		   0xffffffffffffffff,
		   0xffffffffffff,
		   0xffffffff,
		   0xffff
		];

		uint64[16] memory m = [
		   0xffff,
		   0xffffffff,
		   0xffffffffffff,
		   0xffffffffffffffff,
		   0xffff,
		   0xffffffff,
		   0xffffffffffff,
		   0xffffffffffffffff,
		   0xffff,
		   0xffffffff,
		   0xffffffffffff,
		   0xffffffffffffffff,
		   0xffff,
		   0xffffffff,
		   0xffffffffffff,
		   0xffffffffffffffff
		];

		for(uint i=0; i < 2500; ++i) {

			G( v, 0, 4, 8, 12, m[0], m[1]);
			G( v, 1, 5, 9, 13, m[2], m[3]);
			G( v, 2, 6, 10, 14, m[4], m[5]);
			G( v, 3, 7, 11, 15, m[6], m[7]);
			G( v, 0, 5, 10, 15, m[8], m[9]);
			G( v, 1, 6, 11, 12, m[10], m[11]);
			G( v, 2, 7, 8, 13, m[12], m[13]);
			G( v, 3, 4, 9, 14, m[14], m[15]);

			G( v, 0, 4, 8, 12, m[14], m[10]);
			G( v, 1, 5, 9, 13, m[4], m[8]);
			G( v, 2, 6, 10, 14, m[9], m[15]);
			G( v, 3, 7, 11, 15, m[13], m[6]);
			G( v, 0, 5, 10, 15, m[1], m[12]);
			G( v, 1, 6, 11, 12, m[0], m[2]);
			G( v, 2, 7, 8, 13, m[11], m[7]);
			G( v, 3, 4, 9, 14, m[5], m[3]);

			G( v, 0, 4, 8, 12, m[11], m[8]);
			G( v, 1, 5, 9, 13, m[12], m[0]);
			G( v, 2, 6, 10, 14, m[5], m[2]);
			G( v, 3, 7, 11, 15, m[15], m[13]);
			G( v, 0, 5, 10, 15, m[10], m[14]);
			G( v, 1, 6, 11, 12, m[3], m[6]);
			G( v, 2, 7, 8, 13, m[7], m[1]);
			G( v, 3, 4, 9, 14, m[9], m[4]);

			G( v, 0, 4, 8, 12, m[7], m[9]);
			G( v, 1, 5, 9, 13, m[3], m[1]);
			G( v, 2, 6, 10, 14, m[13], m[12]);
			G( v, 3, 7, 11, 15, m[11], m[14]);
			G( v, 0, 5, 10, 15, m[2], m[6]);
			G( v, 1, 6, 11, 12, m[5], m[10]);
			G( v, 2, 7, 8, 13, m[4], m[0]);
			G( v, 3, 4, 9, 14, m[15], m[8]);

			G( v, 0, 4, 8, 12, m[9], m[0]);
			G( v, 1, 5, 9, 13, m[5], m[7]);
			G( v, 2, 6, 10, 14, m[2], m[4]);
			G( v, 3, 7, 11, 15, m[10], m[15]);
			G( v, 0, 5, 10, 15, m[14], m[1]);
			G( v, 1, 6, 11, 12, m[11], m[12]);
			G( v, 2, 7, 8, 13, m[6], m[8]);
			G( v, 3, 4, 9, 14, m[3], m[13]);

			G( v, 0, 4, 8, 12, m[2], m[12]);
			G( v, 1, 5, 9, 13, m[6], m[10]);
			G( v, 2, 6, 10, 14, m[0], m[11]);
			G( v, 3, 7, 11, 15, m[8], m[3]);
			G( v, 0, 5, 10, 15, m[4], m[13]);
			G( v, 1, 6, 11, 12, m[7], m[5]);
			G( v, 2, 7, 8, 13, m[15], m[14]);
			G( v, 3, 4, 9, 14, m[1], m[9]);

			G( v, 0, 4, 8, 12, m[12], m[5]);
			G( v, 1, 5, 9, 13, m[1], m[15]);
			G( v, 2, 6, 10, 14, m[14], m[13]);
			G( v, 3, 7, 11, 15, m[4], m[10]);
			G( v, 0, 5, 10, 15, m[0], m[7]);
			G( v, 1, 6, 11, 12, m[6], m[3]);
			G( v, 2, 7, 8, 13, m[9], m[2]);
			G( v, 3, 4, 9, 14, m[8], m[11]);

			G( v, 0, 4, 8, 12, m[13], m[11]);
			G( v, 1, 5, 9, 13, m[7], m[14]);
			G( v, 2, 6, 10, 14, m[12], m[1]);
			G( v, 3, 7, 11, 15, m[3], m[9]);
			G( v, 0, 5, 10, 15, m[5], m[0]);
			G( v, 1, 6, 11, 12, m[15], m[4]);
			G( v, 2, 7, 8, 13, m[8], m[6]);
			G( v, 3, 4, 9, 14, m[2], m[10]);

			G( v, 0, 4, 8, 12, m[6], m[15]);
			G( v, 1, 5, 9, 13, m[14], m[9]);
			G( v, 2, 6, 10, 14, m[11], m[3]);
			G( v, 3, 7, 11, 15, m[0], m[8]);
			G( v, 0, 5, 10, 15, m[12], m[2]);
			G( v, 1, 6, 11, 12, m[13], m[7]);
			G( v, 2, 7, 8, 13, m[1], m[4]);
			G( v, 3, 4, 9, 14, m[10], m[5]);

			G( v, 0, 4, 8, 12, m[10], m[2]);
			G( v, 1, 5, 9, 13, m[8], m[4]);
			G( v, 2, 6, 10, 14, m[7], m[6]);
			G( v, 3, 7, 11, 15, m[1], m[5]);
			G( v, 0, 5, 10, 15, m[15], m[11]);
			G( v, 1, 6, 11, 12, m[9], m[14]);
			G( v, 2, 7, 8, 13, m[3], m[12]);
			G( v, 3, 4, 9, 14, m[13], m[0]);

			G( v, 0, 4, 8, 12, m[0], m[1]);
			G( v, 1, 5, 9, 13, m[2], m[3]);
			G( v, 2, 6, 10, 14, m[4], m[5]);
			G( v, 3, 7, 11, 15, m[6], m[7]);
			G( v, 0, 5, 10, 15, m[8], m[9]);
			G( v, 1, 6, 11, 12, m[10], m[11]);
			G( v, 2, 7, 8, 13, m[12], m[13]);
			G( v, 3, 4, 9, 14, m[14], m[15]);

			G( v, 0, 4, 8, 12, m[14], m[10]);
			G( v, 1, 5, 9, 13, m[4], m[8]);
			G( v, 2, 6, 10, 14, m[9], m[15]);
			G( v, 3, 7, 11, 15, m[13], m[6]);
			G( v, 0, 5, 10, 15, m[1], m[12]);
			G( v, 1, 6, 11, 12, m[0], m[2]);
			G( v, 2, 7, 8, 13, m[11], m[7]);
			G( v, 3, 4, 9, 14, m[5], m[3]);
		}
	}
}
