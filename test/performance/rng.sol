contract rng {

    // not a good RNG, just enough 256-bit operations to dominate loop overhead
    uint256 rand;
    function rng(){
        uint256 rand1 = 3;
        uint256 rand2 = 5;
        uint256 rand3 = 7;
        uint256 rand4 = 11;
        for (int i = 0; i < 1000000; ++i) {
           rand1 = 0xffe7649d5eca8417 * rand1 + 0xf47ed85c4b9a6379;
           rand2 = 0xf8e5dd9a5c994bba * rand2 + 0xf91d87e4b8b74e55;
           rand3 = 0xff97f6f3b29cda52 * rand3 + 0xf393ada8dd75c938;
           rand4 = 0xfe8d437c45bb3735 * rand3 + 0xf47d9a7b5428ffec;
        }
        rand = rand1 ^ rand2 ^ rand3 ^ rand4;
    }
}
