{
	let r := 0
	for { let i := 0 } lt(i, 8192) { i := add(i, 1) } {

		0xc27b83c7d9d312389735f7b3b3b85eb630608f906992f889b6c8814e31b8eb3b
		0xdd1620ca66b8877ff381a3475f3892560337791da698c011eaa92fb9626161c3
		0xc27b83c7d9d312389735f7b3b3b85eb630608f906992f889b6c8814e31b8eb3b

		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		pop
		dup2
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		pop
		dup2
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		pop
		dup2
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		pop
		dup2
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		pop
		dup2
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		pop
		dup2
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		pop
		dup2
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp dup2 exp
		=: r

		pop
		pop
	}
	switch r
	case 0x3b46f4bb35c94a2cb5f8c1e1e1f918b76a3fe3da9874d2c8abf29a85f90aa37b {
		stop
	}
	default {
		0
		0
		revert
	}
}
