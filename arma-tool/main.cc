#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "main.h"
#include "vmt.h"
#include <locale>
#include <codecvt>


int engineVmtOffset = 0xDA5C1C;
int worldOffset = 0x00DAD8C0;
int flagsPtr = 0x00D897F4;
int gameStatePtr = 0x00DC4D80;
int executePtr = 0x009DAAD0;

blackbone::ptr_t szScript;
blackbone::ptr_t bRunScript;

std::vector<char> data;

bool FileExists(const std::wstring& name) {
  std::ifstream f(name);
  if (f.good()) {
    f.close();
    return true;
  }
  else {
    f.close();
    return false;
  }
}

bool FileExists(const std::string& name) {
	std::ifstream f(name);
	if (f.good()) {
		f.close();
		return true;
	}
	else {
		f.close();
		return false;
	}
}

std::vector<char> FileReadAllBytes(const std::wstring& name) {
  std::ifstream input(name, std::ios::binary);
  if (input.is_open()) {

    std::vector<char> buffer((
      std::istreambuf_iterator<char>(input)),
      (std::istreambuf_iterator<char>()));

    return buffer;
  }
  return std::vector<char>();
}

std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

int LoadDriver()
{
	return blackbone::Driver().EnsureLoaded(blackbone::Utils::GetExeDirectory() + L"\\" + L"MDDrvr.sys");
}

int Read(blackbone::Process &process, blackbone::ptr_t addr, blackbone::ptr_t size, PVOID buff)
{
	return blackbone::Driver().ReadMem(process.pid(), addr, size, buff);
}

int Write(blackbone::Process &process, blackbone::ptr_t addr, blackbone::ptr_t size, PVOID buff)
{
	return blackbone::Driver().WriteMem(process.pid(), addr, size, buff);
}

blackbone::ptr_t DoAllocate(blackbone::Process &process, blackbone::ptr_t size, DWORD protection)
{
	blackbone::ptr_t baseaddr = 0;
	blackbone::Driver().AllocateMem(process.pid(), baseaddr, size, MEM_COMMIT, protection);
	return baseaddr;
}

void HookRenderVTable(blackbone::Process &process)
{
	int renderVmt = 0;
	Read(process, engineVmtOffset, 4, &renderVmt);

	vmtHook *renderer = new vmtHook(&process, (LPVOID)renderVmt);

	blackbone::ptr_t nFuncAddr = DoAllocate(process, 1024, PAGE_EXECUTE_READWRITE);
	szScript = DoAllocate(process, 1024*1024, PAGE_READWRITE);
	bRunScript = DoAllocate(process, 1024, PAGE_READWRITE);

	//nFuncAddr.Release();
	//szScript.Release();
	//bRunScript.Release();

	char hookFunc[] = {
		0x60,							// pushad
		0xA1, 0,0,0,0,					// mov eax,[mRunScript]
		0x83, 0xF8, 0x01,				// cmp eax,01
		0x75, 0x2A,						// jne popad
		0xC7, 0x05, 0,0,0,0, 0,0,0,0,	// mov [mRunScript],00000000
		0xA1, 0,0,0,0,					// mov eax,[WorldPtr]
		0x8B, 0x88, 0,6,0,0,	// mov ecx,[eax+00000888] // World->MissionNamespace
		0x51,							// push ecx
		0x68, 0,0,0,0,					// push 99999999 // flags
		0x68, 0,0,0,0,					// push 69696969 // str
		0xB9, 0,0,0,0,					// mov ecx,00000000 // gamestate
		0xE8, 0,0,0,0,					// call Execute(ptr, str, f, ns);
		0x61,							// popad
		0xE8, 0,0,0,0,					// call originalFunc
		0xC3							// ret
	};

	int realOrigFunc = 0x0;
	Read(process, (DWORD)((int)renderer->originalTable + (2 * sizeof(LPVOID))), 4, &realOrigFunc);
	int origFuncAddr = realOrigFunc - ((int)nFuncAddr + 54) - 5;

	int executeFunctionOffset = executePtr - ((int)nFuncAddr + 48) - 5;

	int ptrBRunScript = bRunScript;
	int ptrSzScript = szScript;
	//int ptrBRunScript = 0;

	memcpy(&hookFunc[2], &ptrBRunScript, sizeof(LPVOID));
	memcpy(&hookFunc[13], &ptrBRunScript, sizeof(LPVOID));
	memcpy(&hookFunc[22], &worldOffset, sizeof(LPVOID));
	memcpy(&hookFunc[34], &flagsPtr, sizeof(LPVOID));
	memcpy(&hookFunc[39], &ptrSzScript, sizeof(LPVOID));
	memcpy(&hookFunc[44], &gameStatePtr, sizeof(LPVOID));

	memcpy(&hookFunc[49], &executeFunctionOffset, sizeof(LPVOID));
	memcpy(&hookFunc[55], &origFuncAddr, sizeof(LPVOID));

	// a2/a3 have diff mission ns offsets


	Write(process, nFuncAddr, sizeof(hookFunc), hookFunc);

	renderer->detour(2, (LPVOID)nFuncAddr);
}

void ExecuteArmaScript(blackbone::Process &process, std::string src)
{
	char bExecute = 1;
	char bEmpty = 0;
	Write(process, szScript, src.length(), (PVOID)src.data());
	Write(process, szScript + src.length(), 1, &bEmpty);
	Write(process, bRunScript, 1, &bExecute);
}

void ExecuteArmaScriptFromFile(blackbone::Process &process, std::string path)
{
	path = ws2s(blackbone::Utils::GetExeDirectory()) + "//scripts//" + path;

	if(!FileExists(path))
	{
		return;
	}

	std::vector<char> data = FileReadAllBytes(s2ws(path));

	if(data.size() <= 0)
	{
		return;
	}

	ExecuteArmaScript(process, data.data());
}

int wmain(int argc, wchar_t * argv[]) {
	auto target = L"ArmA2OA.exe";

	std::vector<DWORD> found;
	if (target) blackbone::Process::EnumByName(target, found);
	int pid = 0;

	std::wcout << L"Searching for " << (target) << "... " << std::endl;

	if (pid == 0 && found.size() > 0) {
		pid = found.front();
	}

	if (pid == 0) {
		std::wcout << L"Could not find " << (target) << "!" << std::endl;
		system("pause");
		return 1;
	}

	blackbone::Process process;

	if (process.Attach(pid) != STATUS_SUCCESS) {
		std::wcout << L"Could not attach to " << target << "!" << std::endl;
		system("pause");
		return 1;
	}

	std::wcout << "Loading drvr... ";

	int drv = LoadDriver();

	if (drv)
	{
		std::wcout << "Could not load driver... " << std::hex << drv << std::endl;
		system("pause");
		return 0;
	}

	std::wcout << L"Escalating privs... ";

	blackbone::Driver().PromoteHandle(process.pid(), process.core().handle(), PROCESS_ALL_ACCESS);

	std::wcout << L"Hooking... ";

	HookRenderVTable(process);

	std::wcout << "\nReady 4 action! Enter scripts below!" << std::endl << std::endl;
	
	while(1)
	{
		std::wcout << "> ";

		std::string input;
		std::getline(std::cin, input);

		if (input == "unload")
		{
			blackbone::Driver().Unload();
			break;
		}
		if(input.substr(0, 5) == "exec " && input.length() > 5)
		{
			ExecuteArmaScriptFromFile(process, input.substr(5));
			continue;
		}
		if(input == "getpatch")
		{
			int scriptCallback = 0;
			int temp = 0;
			Read(process, /*networkmgr*/0xD9F5C0 + 0x24, 4, &temp);
			Read(process, temp + 0x200, 4, &scriptCallback);

			std::wcout << "pcb: " << std::hex << scriptCallback << std::endl;
			continue;
		}
		if(input == "patch")
		{
			int scriptCallback = 0;
			int temp = 0;
			Read(process, /*networkmgr*/0xD9F5C0 + 0x24, 4, &temp);
			Read(process, temp + 0x200, 4, &scriptCallback);

			scriptCallback += 0x8F;
			Write(process, temp + 0x200, 4, &scriptCallback);
			continue;
		}

		ExecuteArmaScript(process, input);

		std::wcout << std::endl;
	}

	return 0;
}