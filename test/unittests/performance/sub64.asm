{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		0x51022b6317003a9d
		0xa20456c62e00753a
		0x51022b6317003a9d

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
	case 0x51022b6317003a9d {
		stop
	}
	default {
		0
		0
		revert
	}
}
