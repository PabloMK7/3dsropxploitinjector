.macro relocate
	add_and_store 0xBABE0007, 0x4, MENU_OBJECT_LOC + appCode - object + 0x1700
	add_and_store 0xBABE0007, 0x13d0, MENU_OBJECT_LOC + appCode - object + 0x1708
	add_and_store 0xBABE0007, 0x8, MENU_OBJECT_LOC + appCode - object + 0x1710
	add_and_store 0xBABE0007, 0x17d0, MENU_OBJECT_LOC + appCode - object + 0x1720
	add_and_store 0xBABE0007, 0x17d6, MENU_OBJECT_LOC + appCode - object + 0x1724
	add_and_store 0xBABE0007, 0x17dc, MENU_OBJECT_LOC + appCode - object + 0x1728
	add_and_store 0xBABE0007, 0x6a1, MENU_OBJECT_LOC + appCode - object + 0x98
.endmacro
