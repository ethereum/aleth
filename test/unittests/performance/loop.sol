pragma solidity ^0.4.0;

contract loop {
	int N;
	function loop(){
		for (int i = 0; i < 1048576; ++i) {
		}
		N = i;
	}
}
