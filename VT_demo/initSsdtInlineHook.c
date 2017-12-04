#include "ntddk.h"
#include "./Hooks/PageHook.h"

/////////////////////////////

NTSTATUS __fastcall proxyNtCreateDebugObject(
	OUT PHANDLE DebugObjectHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG Flags
	);

NTSTATUS
NTAPI
proxyNtDebugContinue(IN HANDLE DebugHandle,
IN PCLIENT_ID AppClientId,
IN NTSTATUS ContinueStatus);
NTSTATUS
__fastcall
proxyNtWaitForDebugEvent(IN HANDLE DebugHandle,
IN BOOLEAN Alertable,
IN PLARGE_INTEGER Timeout OPTIONAL,
OUT ULONG64 StateChange);
NTSTATUS
__fastcall
proxyNtDebugActiveProcess(IN HANDLE ProcessHandle,
IN HANDLE DebugHandle);
NTSTATUS
NTAPI
NtRemoveProcessDebug(IN HANDLE ProcessHandle,
IN HANDLE DebugHandle);
NTSTATUS __fastcall myNtQueryInformationThread(IN   HANDLE   ThreadHandle,
	IN   THREADINFOCLASS   ThreadInformationClass,
	OUT   PVOID   ThreadInformation,
	IN   ULONG   ThreadInformationLength,
	OUT   PULONG   ReturnLength   OPTIONAL
	);



//////////////////////////
PVOID HookKernelApi(IN PVOID ApiAddress, IN PVOID Proxy_ApiAddress, OUT PVOID *Original_ApiAddress, OUT ULONG *PatchSize);
VOID UnhookKernelApi(IN PVOID ApiAddress, IN PVOID OriCode, IN ULONG PatchSize);


ULONG pslp_patch_size40 = 0;		//proxyNtCreateDebugObject���޸���N�ֽ�
PUCHAR pslp_head_n_byte40 = NULL;	//proxyNtCreateDebugObject��ǰN�ֽ�����
PVOID ori_pslp40 = NULL;			//proxyNtCreateDebugObject��ԭ����
ULONG64 NtCreateDebugObject;


ULONG pslp_patch_size41 = 0;		//proxyNtWaitForDebugEvent���޸���N�ֽ�
PUCHAR pslp_head_n_byte41 = NULL;	//proxyNtWaitForDebugEvent��ǰN�ֽ�����
PVOID ori_pslp41 = NULL;			//proxyNtWaitForDebugEvent��ԭ����
ULONG64 NtWaitForDebugEvent;


ULONG pslp_patch_size42 = 0;		//proxyNtDebugContinue���޸���N�ֽ�
PUCHAR pslp_head_n_byte42 = NULL;	//proxyNtDebugContinue��ǰN�ֽ�����
PVOID ori_pslp42 = NULL;			//proxyNtDebugContinue��ԭ����
ULONG64 NtDebugContinue;

ULONG pslp_patch_size43 = 0;		//proxyNtDebugActiveProcess���޸���N�ֽ�
PUCHAR pslp_head_n_byte43 = NULL;	//proxyNtDebugActiveProcess��ǰN�ֽ�����
PVOID ori_pslp43 = NULL;			//proxyNtDebugActiveProcess��ԭ����
ULONG64 NtDebugActiveProcess;

ULONG pslp_patch_size44 = 0;		//myNtRemoveProcessDebug���޸���N�ֽ�
PUCHAR pslp_head_n_byte44 = NULL;	//myNtRemoveProcessDebug��ǰN�ֽ�����
PVOID ori_pslp44 = NULL;			//myNtRemoveProcessDebug��ԭ����
ULONG64 myNtRemoveProcessDebug;



void InitIalzeSsdtInlineHook(){

	pslp_head_n_byte40 = HookKernelApi(NtCreateDebugObject,
		(PVOID)proxyNtCreateDebugObject,
		&ori_pslp40,
		&pslp_patch_size40);

	pslp_head_n_byte41 = HookKernelApi(NtWaitForDebugEvent,
		(PVOID)proxyNtWaitForDebugEvent,
		&ori_pslp41,
		&pslp_patch_size41);

	pslp_head_n_byte42 = HookKernelApi(NtDebugContinue,
		(PVOID)proxyNtDebugContinue,
		&ori_pslp42,
		&pslp_patch_size42);

	pslp_head_n_byte43 = HookKernelApi(NtDebugActiveProcess,
		(PVOID)proxyNtDebugActiveProcess,
		&ori_pslp43,
		&pslp_patch_size43);

	pslp_head_n_byte44 = HookKernelApi(myNtRemoveProcessDebug,
		(PVOID)NtRemoveProcessDebug,
		&ori_pslp44,
		&pslp_patch_size44);
	
}

VOID EPT_InitIalzeSsdtInlineHook(){
	PHHook(NtCreateDebugObject, proxyNtCreateDebugObject);
	PHHook(NtWaitForDebugEvent, proxyNtWaitForDebugEvent);
	PHHook(NtDebugContinue, proxyNtDebugContinue);
	PHHook(NtDebugActiveProcess, proxyNtDebugActiveProcess);
	PHHook(myNtRemoveProcessDebug, NtRemoveProcessDebug);
}
VOID EPT_UnLoadSsdtInlineHook(){
	PHRestore(NtCreateDebugObject);
	PHRestore(NtWaitForDebugEvent);
	PHRestore(NtDebugContinue);
	PHRestore(NtDebugActiveProcess);
	PHRestore(myNtRemoveProcessDebug);

}

void unloadSsdtInlineHook(){

	UnhookKernelApi(NtCreateDebugObject, pslp_head_n_byte40, pslp_patch_size40);
	UnhookKernelApi(NtWaitForDebugEvent, pslp_head_n_byte41, pslp_patch_size41);
	UnhookKernelApi(NtDebugContinue, pslp_head_n_byte42, pslp_patch_size42);
	UnhookKernelApi(NtDebugActiveProcess, pslp_head_n_byte43, pslp_patch_size43);
	UnhookKernelApi(myNtRemoveProcessDebug, pslp_head_n_byte44, pslp_patch_size44);

}