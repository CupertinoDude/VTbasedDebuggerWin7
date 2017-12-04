/*
Code By Tesla.Angela
*/
//pSourceAddress��Ҫ��ȡ���ں˵�ַ
#include "ntddk.h"
typedef int(*LDE_DISASM)(void *p, int dw);
 LDE_DISASM LDE;
 ULONG64 FindFreeSpace(ULONG64 StartAddress, ULONG64 Length);
 void GetKernelModuleBase(char* lpModuleName, ULONG64 *ByRefBase, ULONG *ByRefSize);
 void SafeMemcpy(PVOID dst, PVOID src, ULONG32 length);
#define kmalloc(_s) ExAllocatePoolWithTag(NonPagedPool, _s, 'SYSQ')
#define kfree(_p) ExFreePool(_p)
 typedef struct _HOOK_64BIT{
	 PVOID orgcode;

	 PVOID orgcodenumber;
	 PVOID FreeSpacebyte;

 }HOOK_64BIT, *PHOOK_64BIT;
BOOLEAN SafeRKM(PVOID pDestination, PVOID pSourceAddress, SIZE_T SizeOfCopy)
{
	PMDL pMdl = NULL;
	PVOID pSafeAddress = NULL;
	if (!MmIsAddressValid(pDestination) || !MmIsAddressValid(pSourceAddress))
		return FALSE;
	pMdl = IoAllocateMdl(pSourceAddress, (ULONG)SizeOfCopy, FALSE, FALSE, NULL);
	if (!pMdl)
		return FALSE;
	__try
	{
		MmProbeAndLockPages(pMdl, KernelMode, IoReadAccess);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IoFreeMdl(pMdl);
		return FALSE;
	}
	pSafeAddress = MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);
	if (!pSafeAddress)
		return FALSE;
	__try
	{
		RtlCopyMemory(pDestination, pSafeAddress, SizeOfCopy);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		;
	}
	MmUnlockPages(pMdl);
	IoFreeMdl(pMdl);
	return TRUE;
}

//pDestination��Ҫд����ں˵�ַ
BOOLEAN SafeWKM(PVOID pDestination, PVOID pSourceAddress, SIZE_T SizeOfCopy)
{
	PMDL pMdl = NULL;
	PVOID pSafeAddress = NULL;
	if (!MmIsAddressValid(pDestination) || !MmIsAddressValid(pSourceAddress))
		return FALSE;
	pMdl = IoAllocateMdl(pDestination, (ULONG)SizeOfCopy, FALSE, FALSE, NULL);
	if (!pMdl)
		return FALSE;
	__try
	{
		MmProbeAndLockPages(pMdl, KernelMode, IoReadAccess);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		IoFreeMdl(pMdl);
		return FALSE;
	}
	pSafeAddress = MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);
	if (!pSafeAddress)
		return FALSE;
	__try
	{
		RtlCopyMemory(pSafeAddress, pSourceAddress, SizeOfCopy);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		;
	}
	MmUnlockPages(pMdl);
	IoFreeMdl(pMdl);
	return TRUE;
}

//���ܣ������Ҫ�����ĳ���
//���룺��ַ���������С����
ULONG GetPatchSize(PUCHAR Address, ULONG MinLen)
{
	ULONG LenCount = 0, Len = 0;
	while (LenCount<MinLen)        //<=ԭ��������С�ڵ��ڣ����ڸ�ΪС��
	{
		Len = LDE(Address, 64);
		Address = Address + Len;
		LenCount = LenCount + Len;
	}
	return LenCount;
}

//���ܣ�HOOK API��64λ����addr+14��
//���룺��HOOK������ַ����������ַ������ԭʼ������ַ��ָ�룬���ղ������ȵ�ָ�룻���أ�ԭ��ͷN�ֽڵ�����
PVOID HookKernelApi2(IN PVOID ApiAddress, IN PVOID Proxy_ApiAddress, OUT PVOID *Original_ApiAddress, OUT ULONG *PatchSize)
{
	KIRQL irql;
	UINT64 tmpv;
	PVOID head_n_byte, ori_func;
	UCHAR jmp_code[] = "\xFF\x25\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
	UCHAR jmp_code_orifunc[] = "\xFF\x25\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
	//How many bytes shoule be patch
	*PatchSize = GetPatchSize((PUCHAR)ApiAddress, 14);
	//step 1: Read current data
	head_n_byte = kmalloc(*PatchSize);
	// irql=WPOFFx64();
	// memcpy(head_n_byte,ApiAddress,*PatchSize);
	// WPONx64(irql);
	SafeRKM(head_n_byte, ApiAddress, *PatchSize);
	//step 2: Create ori function
	ori_func = kmalloc(*PatchSize + 14);        //ԭʼ������+��ת������
	RtlFillMemory(ori_func, *PatchSize + 14, 0x90);
	tmpv = (ULONG64)ApiAddress + *PatchSize;        //��ת��û���򲹶����Ǹ��ֽ�
	memcpy(jmp_code_orifunc + 6, &tmpv, 8);
	memcpy((PUCHAR)ori_func, head_n_byte, *PatchSize);
	memcpy((PUCHAR)ori_func + *PatchSize, jmp_code_orifunc, 14);
	*Original_ApiAddress = ori_func;
	//step 3: fill jmp code
	tmpv = (UINT64)Proxy_ApiAddress;
	memcpy(jmp_code + 6, &tmpv, 8);
	//step 4: Fill NOP and hook
	// irql=WPOFFx64();
	// memcpy(ApiAddress,jmp_code,14);
	// WPONx64(irql);
	SafeWKM(ApiAddress, jmp_code, 14);
	//return ori code
	return head_n_byte;
}

//���ܣ�HOOK API��64λ����addr-8 & addr+6��
//���룺��HOOK������ַ����������ַ������ԭʼ������ַ��ָ�룬���ղ������ȵ�ָ�룻���أ�ԭ��ͷN�ֽڵ�����
PVOID HookKernelApi1(IN PVOID ApiAddress, IN PVOID Proxy_ApiAddress, OUT PVOID *Original_ApiAddress, OUT ULONG *PatchSize)
{
	KIRQL irql;
	UINT64 tmpv;
	PVOID head_n_byte, ori_func;
	UCHAR jmp_code[] = "\xFF\x25\xF2\xFF\xFF\xFF";
	UCHAR jmp_code_orifunc[] = "\xFF\x25\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
	//How many bytes shoule be patch
	*PatchSize = GetPatchSize((PUCHAR)ApiAddress, 6);//������Ҫ6�ֽ�
	//step 1: Read current data
	head_n_byte = kmalloc(*PatchSize);
	// irql=WPOFFx64();
	// memcpy(head_n_byte,ApiAddress,*PatchSize);
	// WPONx64(irql);
	SafeRKM(head_n_byte, ApiAddress, *PatchSize);
	//step 2: Create ori function
	ori_func = kmalloc(*PatchSize + 14);        //ԭʼ������+��ת������
	RtlFillMemory(ori_func, *PatchSize + 14, 0x90);
	tmpv = (ULONG64)ApiAddress + *PatchSize;        //��ת��û���򲹶����Ǹ��ֽ�
	memcpy(jmp_code_orifunc + 6, &tmpv, 8);
	memcpy((PUCHAR)ori_func, head_n_byte, *PatchSize);
	memcpy((PUCHAR)ori_func + *PatchSize, jmp_code_orifunc, 14);
	*Original_ApiAddress = ori_func;
	DbgPrint("[ori_func]%p\n", ori_func);
	//step 3: fill jmp code
	tmpv = (UINT64)Proxy_ApiAddress;
	//step 4: Fill NOP and hook
	DbgPrint("[proxy_func]%p\n", Proxy_ApiAddress);
	if (!MmIsAddressValid(Proxy_ApiAddress))
		DbgPrint("[proxy_func]invalid proxy address\n");
	else
		DbgPrint("[proxy_func]valid proxy address\n");
	// irql=WPOFFx64();
	// memcpy((PUCHAR)ApiAddress-8,&tmpv,8);
	// memcpy(ApiAddress,jmp_code,*PatchSize);
	// WPONx64(irql);
	SafeWKM((PUCHAR)ApiAddress - 8, &tmpv, 8);
	SafeWKM(ApiAddress, jmp_code, *PatchSize);
	//return ori code
	return head_n_byte;
}

//���ܣ�ȡ��HOOK API��64λ��
//���룺��HOOK������ַ��ԭʼ���ݣ���������
VOID UnhookKernelApi2(IN PVOID ApiAddress, IN PVOID OriCode, IN ULONG PatchSize)
{
	KIRQL irql;
	// irql=WPOFFx64();
	// memcpy(ApiAddress,OriCode,PatchSize);
	// WPONx64(irql);
	SafeWKM(ApiAddress, OriCode, PatchSize);
}
KIRQL WPOFFx64();
void WPONx64(KIRQL irql);
PVOID HookKernelApi4_6bit(IN ULONG64 ApiAddress, IN PVOID Proxy_ApiAddress, PHOOK_64BIT structb)
{

	ULONG64 FreeSpace = 0;
	LONG lng = 0;
	KIRQL irql = 0;
	ULONG64	KrnlBase = 0;
	ULONG	KrnlSize = 0;
	UCHAR jmp_code[] = "\xFF\x25\x00\x00\x00\x00";
	structb->orgcodenumber = GetPatchSize((PUCHAR)ApiAddress, 6);
	structb->orgcode = kmalloc(structb->orgcodenumber);

	GetKernelModuleBase("ntoskrnl.exe", &KrnlBase, &KrnlSize);



	if (KrnlBase == 0 || KrnlSize == 0 || structb->orgcode == NULL || structb->orgcodenumber == NULL)
		return;


	FreeSpace = FindFreeSpace(KrnlBase, KrnlSize);


	if (FreeSpace == 0)
		return;
	structb->FreeSpacebyte = FreeSpace;
	memcpy(structb->orgcode, ApiAddress, structb->orgcodenumber);

	irql = WPOFFx64();
	memset(ApiAddress, 0x90, structb->orgcodenumber);
	WPONx64(irql);

	SafeMemcpy((PVOID)FreeSpace, &Proxy_ApiAddress, 8);
	lng = (LONG)(FreeSpace - (ApiAddress)-6);
	memcpy(&jmp_code[2], &lng, 4);

	SafeMemcpy((PVOID)(ApiAddress), jmp_code, 6);
}

VOID UnHookKernelApi4_6bit(IN ULONG64 ApiAddress, PHOOK_64BIT structb){
	KIRQL irql;
	SafeMemcpy((PVOID)(ApiAddress), structb->orgcode, structb->orgcodenumber);
	irql = WPOFFx64();
	memset(structb->FreeSpacebyte, 0x90, 8);
	WPONx64(irql);
}

//Tesla.Angela's Win64 Kernel API Hook Engine && Example && Explication


/**
KIRQL WPOFFx64()
{
	KIRQL irql = KeRaiseIrqlToDpcLevel();
	UINT64 cr0 = __readcr0();
	cr0 &= 0xfffffffffffeffff;
	__writecr0(cr0);
	_disable();
	return irql;
}

void WPONx64(KIRQL irql)
{
	UINT64 cr0 = __readcr0();
	cr0 |= 0x10000;
	_enable();
	__writecr0(cr0);
	KeLowerIrql(irql);
}
*/
void *GetFunctionAddr(PCWSTR FunctionName)
{
	UNICODE_STRING UniCodeFunctionName;
	RtlInitUnicodeString(&UniCodeFunctionName, FunctionName);
	return MmGetSystemRoutineAddress(&UniCodeFunctionName);
}

ULONG GetPatchSize2(PUCHAR Address)
{
	ULONG LenCount = 0, Len = 0;
	while (LenCount <= 14)        //������Ҫ14�ֽ�
	{
		Len = LDE(Address, 64);
		Address = Address + Len;
		LenCount = LenCount + Len;
	}
	return LenCount;
}

//���룺��HOOK������ַ����������ַ������ԭʼ������ַ��ָ�룬���ղ������ȵ�ָ�룻���أ�ԭ��ͷN�ֽڵ�����
PVOID HookKernelApi(IN PVOID ApiAddress, IN PVOID Proxy_ApiAddress, OUT PVOID *Original_ApiAddress, OUT ULONG *PatchSize)
{
	KIRQL irql;
	UINT64 tmpv;
	PVOID head_n_byte, ori_func;
	UCHAR jmp_code[] = "\xFF\x25\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
	UCHAR jmp_code_orifunc[] = "\xFF\x25\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
	//How many bytes shoule be patch
	*PatchSize = GetPatchSize2((PUCHAR)ApiAddress);
	//step 1: Read current data
	head_n_byte = kmalloc(*PatchSize);
	irql = WPOFFx64();
	memcpy(head_n_byte, ApiAddress, *PatchSize);
	WPONx64(irql);
	//step 2: Create ori function
	ori_func = kmalloc(*PatchSize + 14);        //ԭʼ������+��ת������
	RtlFillMemory(ori_func, *PatchSize + 14, 0x90);
	tmpv = (ULONG64)ApiAddress + *PatchSize;        //��ת��û���򲹶����Ǹ��ֽ�
	memcpy(jmp_code_orifunc + 6, &tmpv, 8);
	memcpy((PUCHAR)ori_func, head_n_byte, *PatchSize);
	memcpy((PUCHAR)ori_func + *PatchSize, jmp_code_orifunc, 14);
	*Original_ApiAddress = ori_func;
	//step 3: fill jmp code
	tmpv = (UINT64)Proxy_ApiAddress;
	memcpy(jmp_code + 6, &tmpv, 8);
	//step 4: Fill NOP and hook
	irql = WPOFFx64();
	RtlFillMemory(ApiAddress, *PatchSize, 0x90);
	memcpy(ApiAddress, jmp_code, 14);
	WPONx64(irql);
	//return ori code
	return head_n_byte;
}

//���룺��HOOK������ַ��ԭʼ���ݣ���������
VOID UnhookKernelApi(IN PVOID ApiAddress, IN PVOID OriCode, IN ULONG PatchSize)
{
	KIRQL irql;
	irql = WPOFFx64();
	memcpy(ApiAddress, OriCode, PatchSize);
	WPONx64(irql);
}


