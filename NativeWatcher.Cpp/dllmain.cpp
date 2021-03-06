#include <Windows.h>
#include <stdint.h>
#include <string.h>
#include <winnt.h>

#define EXPORT extern "C" __declspec(dllexport)

// from "patches.asm"
extern "C" void set_native_original_case_address(uint64_t address);
extern "C" uint64_t get_native_case_patch_address();
extern "C" uint64_t get_stack_count_ptr();
extern "C" uint64_t get_natives_calls_stack_ptr();


const uint64_t gta5ModuleBaseAddress = (uint64_t)GetModuleHandleA("GTA5.exe");

bool isInitialized;
bool isPatched;
void* switchAddress;
uint32_t* originalSwitchJumpTable;
const int SWITCH_CASES_COUNT = 127;
uint64_t newSwitchJumpTable[SWITCH_CASES_COUNT];
const int SWITCH_CODE_SIZE = 17;
BYTE oldSwitchCode[SWITCH_CODE_SIZE];


uint64_t GetOriginalSwitchCaseAddress(int caseIndex)
{
	uint32_t offset = originalSwitchJumpTable[caseIndex];
	return gta5ModuleBaseAddress + offset;
}

void PerformInitialization(void* address)
{
	if (isInitialized)
	{
		return;
	}

	switchAddress = address;

	int jumpTableOffset = *(int*)((uint64_t)switchAddress + 10);
	originalSwitchJumpTable = (uint32_t*)(gta5ModuleBaseAddress + jumpTableOffset);

	// build newSwitchJumpTable
	for (int i = 0; i < SWITCH_CASES_COUNT; i++)
	{
		newSwitchJumpTable[i] = GetOriginalSwitchCaseAddress(i);
	}
	set_native_original_case_address(GetOriginalSwitchCaseAddress(44));
	newSwitchJumpTable[44] = get_native_case_patch_address();

	isInitialized = true;
}

void UnpatchSwitchJumpTable()
{
	if (!isPatched)
	{
		return;
	}

	memcpy(switchAddress, oldSwitchCode, SWITCH_CODE_SIZE);

	isPatched = false;
}

void PatchSwitchJumpTable()
{
	if (isPatched)
	{
		return;
	}

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

EXPORT bool IsInitialized()
{
	return isInitialized;
}

EXPORT void Initialize(void* switchAddress) 
{
	PerformInitialization(switchAddress);
}

EXPORT bool IsHooked()
{
	return isPatched;
}

EXPORT void Hook()
{
	PatchSwitchJumpTable();
}

EXPORT void Unhook()
{
	UnpatchSwitchJumpTable();
}

EXPORT uint64_t GetStackCount()
{
	return get_stack_count_ptr();
}

EXPORT uint64_t GetNativesCallsStack()
{
	return get_natives_calls_stack_ptr();
}


