{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		0xfdedc7f10142ff97
		0xfbdfda0e2ce356173d1993d5f70a2b11
		0xfdedc7f10142ff97

		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		pop
		dup2
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		pop
		dup2
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		pop
		dup2
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		pop
		dup2
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		pop
		dup2
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		pop
		dup2
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		pop
		dup2
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div
		dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div dup2 div

		=: r
		pop
		pop
	}
	switch r
	case 0xfdedc7f10142ff97 {
		stop
	}
	default {
		0
		0
		revert
	}
}

