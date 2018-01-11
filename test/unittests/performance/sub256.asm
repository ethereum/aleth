{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

			0x4bfcd8bb2ac462735b48a17580690283980aa2d679f091c64364594df113ea37
			0x97f9b1765588c4e6b69142eb00d20507301545acf3e1238c86c8b29be227d46e
			0x4bfcd8bb2ac462735b48a17580690283980aa2d679f091c64364594df113ea37

			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			pop
			dup2
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			pop
			dup2
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			pop
			dup2
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			pop
			dup2
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			pop
			dup2
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			pop
			dup2
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			pop
			dup2
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub
			dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub dup2 sub

		=: r
		pop
		pop
	}
	switch r
	case 0x4bfcd8bb2ac462735b48a17580690283980aa2d679f091c64364594df113ea37 {
		stop
	}
	default {
		0
		0
		revert
	}
}
