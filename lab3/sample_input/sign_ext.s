	.data
	.text
main:
	addiu	$1, $0, -1
	addiu	$2, $0, -1
	andi	$2, $2, 0xffff
	ori	$3, $0, 0xffff
	lui	$4, 1
	sltiu	$4, $4, -1
