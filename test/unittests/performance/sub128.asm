{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		0x4dde30faaacdc14d00327aac314e915d
		0x9bbc61f5559b829a0064f558629d22ba
		0x4dde30faaacdc14d00327aac314e915d

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
	case 0x4dde30faaacdc14d00327aac314e915d {
		stop
	}
	default {
		0
		0
		revert
	}
}
