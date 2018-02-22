{
	let r := 0
	0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
	256
	shl
	=: r
	switch r
	case 0 {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	2
	1
	shl
	=: r
	switch r
	case 4 {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
	1
	shl
	=: r
	switch r
	case 0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	0x8000000000000000000000000000000000000000000000000000000000000000
	1
	shl
	=: r
	switch r
	case 0 {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	3
	255
	shl
	=: r
	switch r
	case 0x8000000000000000000000000000000000000000000000000000000000000000 {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	0xffffffff
	224
	shl
	=: r
	switch r
	case 0xffffffff00000000000000000000000000000000000000000000000000000000 {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	2
	1
	shr
	=: r
	switch r
	case 1 {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
	256
	shr
	=: r
	switch r
	case 0 {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
	1
	shr
	=: r
	switch r
	case 0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
	1
	shr
	=: r
	switch r
	case 0x3fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	0x8000000000000000000000000000000000000000000000000000000000000000
	1
	shr
	=: r
	switch r
	case 0x4000000000000000000000000000000000000000000000000000000000000000 {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
	256
	sar
	=: r
	switch r
	case 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff {
	}
	default {
		0
		0
		revert
	}

	///
	0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
	256
	sar
	=: r
	switch r
	case 0 {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
	1
	sar
	=: r
	switch r
	case 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff {
	}
	default {
		0
		0
		revert
	}

	///
	r := 0
	0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
	1
	sar
	=: r
	switch r
	case 0x3fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff {
		stop
	}
	default {
		0
		0
		revert
	}
}
