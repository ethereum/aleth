contract loop {
    int N;
    function loop(){
        for (int i = 0; i < 1000000; ++i) {
        }
        N = i;
    }
}
