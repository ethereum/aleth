// https://people.csail.mit.edu/rivest/Rivest-rc5rev.pdf

contract rc5 {

   // don't I wish we had opcodes and operators for these
   
   function shift_left(uint v, uint n) returns (uint v) {
      v *= 2**n;
   }

   function shift_right(uint v, uint n) returns (uint v) {
      v /= 2**n;
   }
   
   function rotate_left(uint v, uint n) returns (uint) {
      return shift_left(v, n) | shift_right(v, 256 - n);
   }
   
   function rotate_right(uint v, uint n) returns (uint) {
      return shift_right(v, n) | shift_left(v, 256 - n);
   }

   // expand key L into S array using magic numbers P and Q

   uint[512] S;

   uint P = 3; // uint(.71828182845904523536028747135266249775724709369995957496696762772407663035354759 * 2**256) | 1;
   uint Q = 5; // uint(.61803398874989484820458683436563811772030917980576286213544862270526046281890244 * 2**256) | 1;

   function expand(uint[] L) {
      uint A = 0;
      uint B = 0;
      uint i;
      uint j;
      S[0] = P;
      uint n = L.length > 512 ? L.length : 512;
      n *= 3;
      for (i = 1; i < 512; ++i)
         S[i] = S[i - 1] + Q;
      i = j = 0;
      while (n-- > 0) {
         A = S[i] = rotate_left((S[i] + A + B), 3);
         B = L[j] = rotate_left((L[j] + A + B), A + B);
         i = ++i % 512;
         j = ++j % L.length;
      }
   }

   function encrypt(uint[] inout) {
      for (uint i = 0; i < inout.length; i += 2) {
         uint A = inout[i];
         uint B = inout[i+1];
         A += S[0];
         B += S[1];
         for (uint j = 1; j < 16; ++j) {
            A = rotate_left((A ^ B), B) + S[2 * j];
            B = rotate_left((B ^ A), A) + S[2 * j + 1];
         }
         inout[i] = A;
         inout[i+1] = B;
      }
   }

   function decrypt(uint[] inout) {
      for (uint i = 0; i < inout.length; i += 2) {
         uint A = inout[i];
         uint B = inout[i+1];
         for (uint j = 15; j >= 0; --j) {
            B = rotate_right(B - S[2 * i + 1], A) ^ A;
            A = rotate_right(A - S[2 * i], B) ^ B;
         }
         B -= S[1];
         A -= S[0];
         inout[i] = A;
         inout[i+1] = B;
      }
   }
   
   uint[] key;
   uint[] msg;
      
   function rc5() {         
      
      for (uint i = 0; i < 10000; ++i) {

         // simple, growing test keys and messages for encryption and decryption
         key.push(i);
         expand(key);
         msg.push(i);
         msg.push(i);
         uint[] tmp = msg;
         encrypt(tmp);
         decrypt(tmp);
         
         // decrypt of encrypt should be the same
         for (uint j = 0; j < msg.length; ++j)
            if (msg[j] != tmp[j])
               throw;
      }
   }
}
