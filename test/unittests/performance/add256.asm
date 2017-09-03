{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		0x802431afcbce1fc194c9eaa417b2fb67dc75a95db0bc7ec6b1c8af11df6a1da9
		0xa1f5aac137876480252e5dcac62c354ec0d42b76b0642b6181ed099849ea1d57
		0x802431afcbce1fc194c9eaa417b2fb67dc75a95db0bc7ec6b1c8af11df6a1da9
		
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
	case 0x9f7eddc3444467c3e7afc7507a765053e9b860c8b6ff34ded09948967e0bf319 {
		stop
	}
	default {
		0
		0
		revert
	}
}
