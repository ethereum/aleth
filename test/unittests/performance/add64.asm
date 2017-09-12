{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		0xffffffff
		0xfd37f3e2bba2c4f
		0xffffffff
		
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		pop
		dup2
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		pop
		dup2
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		pop
		dup2
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		pop
		dup2
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		pop
		dup2
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		pop
		dup2
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		pop
		dup2
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add
		dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add dup2 add

		=: r
		pop
		pop
	}
	switch r
	case 0xfd37f3e3bba2c4ef {
		stop
	}
	default {
		0
		0
		revert
	}
}