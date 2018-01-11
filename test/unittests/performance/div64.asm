{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		0xfcb34eb3
		0xf97180878e839129
		0xfcb34eb3
	
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
	case 0xfcb34eb3 {
		stop
	}
	default {
		0
		0
		revert
	}
}
