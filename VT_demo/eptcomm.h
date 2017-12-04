#include "ntddk.h"
#define _HARDWARE_PTE_WORKING_SET_BITS  11
typedef struct _MMPTE {
	ULONGLONG Valid : 1;
	ULONGLONG Writable : 1;        // changed for MP version
	ULONGLONG Owner : 1;
	ULONGLONG WriteThrough : 1;
	ULONGLONG CacheDisable : 1;
	ULONGLONG Accessed : 1;
	ULONGLONG Dirty : 1;
	ULONGLONG LargePage : 1;
	ULONGLONG Global : 1;
	ULONGLONG CopyOnWrite : 1; // software field
	ULONGLONG Prototype : 1;   // software field
	ULONGLONG Write : 1;       // software field - MP change
	ULONGLONG PageFrameNumber : 28;
	ULONG64 reserved1 : 24 - (_HARDWARE_PTE_WORKING_SET_BITS + 1);
	ULONGLONG SoftwareWsIndex : _HARDWARE_PTE_WORKING_SET_BITS;
	ULONG64 NoExecute : 1;
} MMPTE, *PMMPTE;
#define VIRTUAL_ADDRESS_BITS 48
#define VIRTUAL_ADDRESS_MASK ((((ULONG_PTR)1) << VIRTUAL_ADDRESS_BITS) - 1)

#define PTE_BASE          0xFFFFF68000000000UI64
#define PTI_SHIFT 12
#define PDI_SHIFT 21
#define PPI_SHIFT 30
#define PXI_SHIFT 39

#define PTE_SHIFT 3
#define PDE_SHIFT 2

#define MiGetPteAddress(va)  ((PMMPTE)(((((ULONG_PTR)(va) & VIRTUAL_ADDRESS_MASK) >> PTI_SHIFT) << PTE_SHIFT) + PTE_BASE))
//#define MiGetPteAddress(va) ((PULONG64)(((((ULONG64)(va)&0x0000FFFFFFFFF000) >> 12))*8 + PTE_BASE))
#define MiGetPdeAddress(va) ((PULONG64)(((((ULONG64)(va)&0x0000FFFFFFFFF000) >> 21))*8 + PDE_BASE))
#define MiGetPpeAddress(va) ((PULONG64)(((((ULONG64)(va)&0x0000FFFFFFFFF000) >> 30))*8 + PPE_BASE))
#define MiGetPxeAddress(va) ((PULONG64)(((((ULONG64)(va)&0x0000FFFFFFFFF000) >>39))*8 + PXE_BASE))
#define   GUEST_INTERRUPTIBILITY_INFO  0x00004824

#define TRAP_MTF						0
#define TRAP_DEBUG						1
#define TRAP_INT3						3
#define TRAP_INTO						4
#define TRAP_GP					    13
#define TRAP_PAGE_FAULT					14
#define TRAP_INVALID_OP					6

#define DIVIDE_ERROR_EXCEPTION 0
#define DEBUG_EXCEPTION 1
#define NMI_INTERRUPT 2
#define BREAKPOINT_EXCEPTION 3
#define OVERFLOW_EXCEPTION 4
#define BOUND_EXCEPTION 5
#define INVALID_OPCODE_EXCEPTION 6
#define DEVICE_NOT_AVAILABLE_EXCEPTION 7
#define DOUBLE_FAULT_EXCEPTION 8
#define COPROCESSOR_SEGMENT_OVERRUN 9
#define INVALID_TSS_EXCEPTION 10
#define SEGMENT_NOT_PRESENT 11
#define STACK_FAULT_EXCEPTION 12
#define GENERAL_PROTECTION_EXCEPTION 13
#define PAGE_FAULT_EXCEPTION 14
#define X87_FLOATING_POINT_ERROR 16
#define ALIGNMENT_CHECK_EXCEPTION 17
//#define MACHINE_CHECK_EXCEPTION 18
#define SIMD_FLOATING_POINT_EXCEPTION 19

#define EXTERNAL_INTERRUPT 0
#define HARDWARE_EXCEPTION 3
#define SOFTWARE_INTERRUPT 4
#define PRIVILEGED_SOFTWARE_EXCEPTION 5
#define SOFTWARE_EXCEPTION 6
#define OTHER_EVENT 7

#define HVM_DELIVER_NO_ERROR_CODE (-1)

#pragma pack (push, 1)
typedef struct _DBG_ESP
{
	ULONG64   Esp0;
	ULONG64   Esp1;
	ULONG64   Esp2;
	ULONG64   Esp3;
	ULONG64   Esp4;
	ULONG64   Esp5;
} DBG_ESP, *PDBG_ESP;

typedef struct _DEBG_ESP
{
	LIST_ENTRY  ListEntry;
	HANDLE      Threaid;
	DBG_ESP  Stack;
	PDBG_ESP PStack;
	ULONG64  GuestPA;
} DEBG_ESP, *PDEBG_ESP;

typedef struct _DEBUG_BP_
{
	LIST_ENTRY        ListEntry;
	PEPROCESS         Proces;
	ULONG64           Cr3;
	ULONG64           PT;
	ULONG64           EPTPTEVA;
	PHYSICAL_ADDRESS  EPTPTEPA;
	ULONG64           OldEPTPDEPA;
	ULONG64           EPTPDEPA;
	ULONG64           EPTPDEVA;
	PHYSICAL_ADDRESS  NewEPTPDEPA;
	ULONG64           NewEPTPDEVA;
	PHYSICAL_ADDRESS  NewEPTPtePA;
	ULONG64           NewEPTPteVA;
	ULONG64           BPVA;
	ULONG64           BPPA;
	BOOLEAN           BPHOOK;
	BOOLEAN           BPUNHOOK;
	PHYSICAL_ADDRESS  GuestPA;
	ULONG64           OldEPTPtePA;

	ULONG64           OldRip;
	ULONG64           NewRip;
} DEB_BP, *PDEB_BP;
#pragma pack (pop)
#pragma pack (push, 1)
typedef struct _DEBUG_ESP
{
	ULONG64   Esp0;
	ULONG64   Esp1;
	ULONG64   Esp2;
	ULONG64   Esp3;
	ULONG64   Esp4;
	ULONG64   Esp5;
} DEBUG_ESP, *PDEBUG_ESP;
#pragma pack (pop)
typedef struct tagItem
{
	LIST_ENTRY  ListEntry;
	PEPROCESS   Proces;
	HANDLE      ThreadId;
	ULONG64     Cr3;
	ULONG64      Page;
	ULONG64		 Flags;
	PDEBUG_ESP   PDEsp;
	DEBUG_ESP    DEsp;
	ULONG64      BpIndex;
	ULONG64       GuestRip;
} VT_ITEM, *PVT_ITEM;
typedef struct _KAPC_STATE {
	LIST_ENTRY ApcListHead[MaximumMode];
	struct _KPROCESS *Process;
	BOOLEAN KernelApcInProgress;
	BOOLEAN KernelApcPending;
	BOOLEAN UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *PRKAPC_STATE;



typedef struct _DEBUG_BP
{
	PEPROCESS         Proces;
	ULONG64           PteOffset;
	ULONG64           Pte;
	PHYSICAL_ADDRESS  Ptephy;
	PHYSICAL_ADDRESS  GuestPA;
	ULONG64           OldPtePA;
	ULONG64           NewPtePA;
	ULONG64           OldRip;
	ULONG64           NewRip;
	ULONG64           NEWPdeVA;
	ULONG64           Cr3;
	ULONG64           Addr;
	BOOLEAN           Addrhook;
} DEBUG_BP, *PDEBUG_BP;


typedef struct Buffer_Lock
{
	PMDL mdl;
	PVOID *lpData;
} BUFFER_LOCK;
typedef struct _MMPTE_HARDWARE_PAE {

	ULONGLONG Valid : 1;			//0 ӳ��״̬ ��ҳ��ǰû��ӳ�������ڴ�=0 ��ҳ��ǰӳ���������ڴ�=1
	ULONGLONG Write : 1;			// 1��д���� ֻ��=0 �ɶ���д=1 (��λ����ֻ���û���������Ч �ں�̬�������� ���ܿ��Զ�/д)
	ULONGLONG Owner : 1;			//2 ����ģʽ ֻ���ں�ģʽ���Է���=0 �ں����û�ģʽ���ɷ���=1
	ULONGLONG WriteThrough : 1;		//3 д��ģʽ ��дģʽ(д�������ͬ��)=0 ֱдģʽ(д�����ͬ��)=1
	ULONGLONG CacheDisable : 1;		//4 ����ģʽ ������=0 ���û���=1
	ULONGLONG Accessed : 1;			//5 ����״̬ ��ҳδ����/д��=0 ��ҳ�ѱ���/д��=1
	ULONGLONG Dirty : 1;			//6 ��ҳ״̬ ��ҳδ���Ķ���=0 ��ҳ�ѱ��Ķ���=1
	ULONGLONG LargePage : 1;		//7 ��ҳ��С ��λֻ��PDE��Ч PAEģʽ��Ϊ4K��2M����ҳ 4K��ҳ=0 2M��ҳ=1
	ULONGLONG Global : 1;			//8 ȫ��ҳ�� ��PTE��Ӧ�����н��̿ռ�=1 ��PTE��Ӧ��ǰ���̿ռ�=0
	ULONGLONG CopyOnWrite : 1; 		//9 дʱ����
	ULONGLONG Prototype : 1;		//10 �ڴ湲�� ����R3����̹����ڴ�
	ULONGLONG reserved0 : 1;		//11 ����λ δʹ��11
	ULONGLONG PageFrameNumber : 28;	// PFN����ҳ��ҳ֡�� ����PTE�������0xfffffffffffff000�������� �ɵõ���ҳ�����ڴ��ַ
	ULONGLONG reserved1 : 23;		// ����λ δʹ��
	ULONGLONG Execute : 1;			// �Ƿ�ִ�� ����ִ��=1 ��ִ��=0

} MMPTE_HARDWARE_PAE, *PMMPTE_HARDWARE_PAE;
