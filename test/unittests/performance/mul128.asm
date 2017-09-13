{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		0xb5
		0xb5
		0xb5

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
	case 0xb4b0df2687b7c1a6fb5d5b1943a57675 {
		stop
	}
	default {
		0
		0
		revert
	}
}

