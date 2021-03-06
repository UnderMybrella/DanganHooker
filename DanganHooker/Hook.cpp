#include "Hook.h"
#include "data.h"
#include "Scripting.h"
#include "MemoryAddresses.h"

const int ASM_NOP = 0x90;
const int ASM_JMP = 0xE9;

bool Hook::InitiateHooks()
{
	if (Data::Game == Data::Games::DR2) 
	{
		for (int a = 0; a < 3; a++)
		{
			DetourInstructions(Data::Dangan2DetourInfo[a].AddressStart, Data::Dangan2DetourInfo[a].AddressEnd, Data::Dangan2DetourInfo[a].DetourFunction);
		}
	}
	if (Data::Game == Data::Games::DR1) 
	{
		for (int a = 0; a < 2; a++)
		{
			DetourInstructions(Data::DanganDetourInfo[a].AddressStart, Data::DanganDetourInfo[a].AddressEnd, Data::DanganDetourInfo[a].DetourFunction);
		}
	}


	return true;
}

bool Hook::InitiateOpcodes()
{
	if (Data::Game == Data::Games::DR2) {
		//Reads the memory for existing opcode addresses and stores them in the new array.
		for (int a = 0; a < 77; a++)
		{
			Scripting::OperationFunctions[a] = ReadPointer(Dr2Addresses::OPCODE_FUNCTION_ARRAY + (4 * a));
		}

		if (Scripting::Cnt_opcodes <= 0xFF)
		{
			//Overwrites the one byte in the opcode count compare since the instruction is too small to detour.
			WriteByte(Dr2Addresses::OPCODE_SIZE_CMP_INSTRUCTION + 3, Scripting::Cnt_opcodes);
		}
		else
		{
			//NOP out the comparison; there's no point
			void* FuncToDetour = reinterpret_cast<void*>(AbsoluteAddress(Dr2Addresses::OPCODE_SIZE_CMP_INSTRUCTION));

			DWORD CurrentProtection;
			VirtualProtect(FuncToDetour, 6, PAGE_EXECUTE_READWRITE, &CurrentProtection);

			memset(FuncToDetour, 0x90, 6);

			DWORD Temp;
			VirtualProtect(FuncToDetour, 6, CurrentProtection, &Temp);
		}

		//Creates a jump to address for the function to return to.
		CommonAddresses::ReturnGetOpFunc = AbsoluteAddress(Dr2Addresses::OPCODE_END);

		//Load custom opcodes into the new array.
		Scripting::LoadCustomOpcodes();
	}

	if (Data::Game == Data::Games::DR1)
	{
		//Reads the memory for existing opcode addresses and stores them in the new array.
		for (int a = 0; a <= 0x3C; a++)
		{
			Scripting::OperationFunctions[a] = ReadPointer(Dr1Addresses::OPCODE_FUNCTION_ARRAY + (4 * a));
		}

		if (Scripting::Cnt_opcodes <= 0xFF) 
		{
			//Overwrites the one byte in the opcode count compare since the instruction is too small to detour.
			WriteByte(Dr1Addresses::OPCODE_SIZE_CMP_INSTRUCTION + 3, Scripting::Cnt_opcodes);
		} else
		{
			//NOP out the comparison; there's no point
			void* FuncToDetour = reinterpret_cast<void*>(AbsoluteAddress(Dr1Addresses::OPCODE_SIZE_CMP_INSTRUCTION));

			DWORD CurrentProtection;
			VirtualProtect(FuncToDetour, 6, PAGE_EXECUTE_READWRITE, &CurrentProtection);

			memset(FuncToDetour, ASM_NOP, 6);
			
			DWORD Temp;
			VirtualProtect(FuncToDetour, 6, CurrentProtection, &Temp);
		}

		//Creates a jump to address for the function to return to.
		CommonAddresses::ReturnGetOpFunc = AbsoluteAddress(Dr1Addresses::OPCODE_END);

		//Load custom opcodes into the new array.
		Scripting::LoadCustomOpcodes();

		//Data::ADDRESS_ReturnCrashAndBurn = AbsoluteAddress(Data::DanganDetourInfo[2].AddressEnd);
	}

	return true;
}

bool Hook::DetourInstructions(DWORD HookAddress, DWORD HookAddressEnd, void * NewFunction)
{
	int Length = HookAddressEnd - HookAddress;
	if (Length < 5)
	{
		Console::WriteLine("[ERROR] Length of instructions to replace was less then 5.");
		return false;
	}

	Console::WriteLine("[DEBUG] Instructions being replaced.");

	void * FuncToDetour = (void*)AbsoluteAddress(HookAddress);

	DWORD CurrentProtection;
	VirtualProtect(FuncToDetour, Length, PAGE_EXECUTE_READWRITE, &CurrentProtection);

	memset(FuncToDetour, ASM_NOP, Length);

	DWORD RelativeAddress = ((DWORD)NewFunction - (DWORD)FuncToDetour) - 5;

	*(BYTE*)FuncToDetour = ASM_JMP;
	*(DWORD*)((DWORD)FuncToDetour + 1) = RelativeAddress;

	DWORD Temp;
	VirtualProtect(FuncToDetour, Length, CurrentProtection, &Temp);

	return true;
}

bool Hook::WritePointer(DWORD PointerAddress, DWORD Pointer)
{
	*(DWORD*)(AbsoluteAddress(PointerAddress)) = Pointer;
	return true;
}

bool Hook::WriteByte(DWORD ByteAddress, BYTE Byte)
{
	char * ByteToWriteTo = (char*)AbsoluteAddress(ByteAddress);
	DWORD curProtection;
	VirtualProtect(ByteToWriteTo, 1, PAGE_EXECUTE_READWRITE, &curProtection);

	memset(ByteToWriteTo, Byte, 1);

	DWORD temp;
	VirtualProtect(ByteToWriteTo, 1, curProtection, &temp);


	return false;
}

void Hook::Init()
{

	if (!Hooked) 
	{
		if (InitiateHooks()) 
		{
			Console::WriteLine("[DEBUG] Instructions have been detoured!");
		}
		if (InitiateOpcodes()) 
		{
			Console::WriteLine("[DEBUG] Custom opcodes have been enabled!");
		}


		Hooked = true;
	}
	

}

DWORD Hook::ReadPointer(DWORD PointerAddress)
{
	return *(DWORD*)(AbsoluteAddress(PointerAddress));
}

