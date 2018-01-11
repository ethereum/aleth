{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		1
		2
		1
		
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		pop
		dup2
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		pop
		dup2
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		pop
		dup2
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		pop
		dup2
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		pop
		dup2
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		pop
		dup2
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		pop
		dup2
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop
		dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop dup2 pop

		=: r
		pop
		pop
	}
	switch r
	case 1 {
		stop
	}
	default {
		0
		0
		revert
	}
}
