{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		0xffffffffffffffff
		0xf5470b43c6549b016288e9a65629687
		0xffffffffffffffff
		
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
	case 0xf5470b43c6549b026288e9a65629686f {
		stop
	}
	default {
		0
		0
		revert
	}
}
