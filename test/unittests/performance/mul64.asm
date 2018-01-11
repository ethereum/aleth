{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		0xd
		0xd
		0xd

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
	case 0x780c7372621bd74d {
		stop
	}
	default {
		0
		0
		revert
	}
}




