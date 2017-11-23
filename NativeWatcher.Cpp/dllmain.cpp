#include <Windows.h>
#include <iostream>
#include <fstream>
#include <winnt.h>

#define EXPORT extern "C" __declspec(dllexport)

// from "patches.asm"
extern "C" void set_native_original_case_address(uint64_t address);
extern "C" uint64_t get_native_case_patch_address();
extern "C" uint64_t get_stack_count_ptr();
extern "C" uint64_t get_natives_calls_stack_ptr();


bool isPatched;
void* switchPatchAddress;
uint32_t* originalSwitchJumpTable;
const int SWITCH_CASES_COUNT = 127;
uint64_t newSwitchJumpTable[SWITCH_CASES_COUNT];
const int SWITCH_CODE_SIZE = 17;
BYTE oldSwitchCode[SWITCH_CODE_SIZE];


uint64_t GetOriginalSwitchCaseAddress(int caseIndex)
{
	static uint64_t moduleBaseAddress = (uint64_t)GetModuleHandle(L"GTA5.exe");

	uint32_t relativeAddress = originalSwitchJumpTable[caseIndex];
	return moduleBaseAddress + relativeAddress;
}

void UnpatchSwitchJumpTable()
{
	if (!isPatched)
	{
		return;
	}

	memcpy(switchPatchAddress, oldSwitchCode, SWITCH_CODE_SIZE);

	isPatched = false;
}

void PatchSwitchJumpTable(void* switchAddress)
{
	if (isPatched)
	{
		return;
	}

/*
original code (48 8D 15 ?? ?? ?? ?? 8B 8C 82 ?? ?? ?? ?? 48 03 CA FF E1 48 2B FE)
.text:00000000014D45C1 0F8   lea rdx, cs:0       ; Load Effective Address
.text:00000000014D45C8 0F8   mov ecx, ds:ExecuteScriptStatementSwitchJumpTable[rdx+rax*4]
.text:00000000014D45CF 0F8   add rcx, rdx        ; Add
*/
	switchPatchAddress = switchAddress;

	//uint64_t address = (uint64_t)switchAddress;
	//address = address + *(int*)(address + 10) + 14;
	// harcoded v1180
	originalSwitchJumpTable = (uint32_t*)((uint64_t)GetModuleHandle(L"GTA5.exe") + 0x1500FFC);//(uint32_t*)address;

	// build newSwitchJumpTable
	for (int i = 0; i < SWITCH_CASES_COUNT; i++)
	{
		newSwitchJumpTable[i] = GetOriginalSwitchCaseAddress(i);
	}
	set_native_original_case_address(GetOriginalSwitchCaseAddress(44));
	newSwitchJumpTable[44] = get_native_case_patch_address();

	memcpy(oldSwitchCode, switchAddress, SWITCH_CODE_SIZE);

	BYTE switchPatchCode[SWITCH_CODE_SIZE] = 
	{
		// mov rcx, newSwitchJumpTable
		0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		// mov rcx, [rcx+rax*8]
		0x48, 0x8B, 0x0C, 0xC1,

		0x90, 0x90, 0x90
	};
	*(uint64_t**)&switchPatchCode[2] = newSwitchJumpTable;

	memcpy(switchAddress, switchPatchCode, SWITCH_CODE_SIZE);

	isPatched = true;
}


void HookInternal(void* switchAddress)
{
	PatchSwitchJumpTable(switchAddress);
}

void UnhookInternal()
{
	UnpatchSwitchJumpTable();
}



EXPORT bool IsHooked()
{
	return isPatched;
}

EXPORT void Hook(void* switchAddress)
{
	HookInternal(switchAddress);
}

EXPORT void Unhook()
{
	UnhookInternal();
}

EXPORT uint64_t GetStackCount()
{
	return get_stack_count_ptr();
}

EXPORT uint64_t GetNativesCallsStack()
{
	return get_natives_calls_stack_ptr();
}


