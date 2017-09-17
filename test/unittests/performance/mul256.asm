{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		0x802431afcbce1fc194c9eaa417b2fb67dc75a95db0bc7ec6b1c8af11df6a1da9
		0xa1f5aac137876480252e5dcac62c354ec0d42b76b0642b6181ed099849ea1d57
		0x802431afcbce1fc194c9eaa417b2fb67dc75a95db0bc7ec6b1c8af11df6a1da9

		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		pop
		dup2
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		pop
		dup2
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		pop
		dup2
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		pop
		dup2
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		pop
		dup2
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		pop
		dup2
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		pop
		dup2
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul
		dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul dup2 mul

		=: r
		pop
		pop
	}
	switch r
	case 0xf68cec53bdbebd0d8a2587f4716010120f43bc8873f6d3b26c697204beaf2a29 {
		stop
	}
	default {
		0
		0
		revert
	}
}

