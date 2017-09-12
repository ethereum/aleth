{
	let r := 0
	for { let i := 0 } lt(i, 1048576) { i := add(i, 1) } {

		0xff3f9014f20db29ae04af2c2d265de17
		0xfe7fb0d1f59dfe9492ffbf73683fd1e870eec79504c60144cc7f5fc2bad1e611
		0xff3f9014f20db29ae04af2c2d265de17

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
	case 0xff3f9014f20db29ae04af2c2d265de17 {
		stop
	}
	default {
		0
		0
		revert
	}
}
