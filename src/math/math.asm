[extern pow]
pow:
	mov eax, [esp+0x1c]
	fld TWORD [esp+0x4]
	fld TWORD [esp+0x10]
	fyl2x
	fld1
	fld st1
	fprem
	f2xm1
	fadd
	fscale
	fxch st1
	fstp st0
	fstp TWORD [eax]
	ret