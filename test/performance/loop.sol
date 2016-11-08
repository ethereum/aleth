contract loop {
	int N;
	function loop(){
		for (int i = 0; i < 10000000; ++i) {
		}
		N = i;
	}
}
