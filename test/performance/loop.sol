contract loop {
	int N;
	function loop(){
		for (int i = 0; i < 4000000; ++i) {
		}
		N = i;
	}
}
