#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "main.h"

class vmtHook {
public:
	vmtHook(blackbone::Process* hProc, LPVOID vTablePtr);
	~vmtHook();

	blackbone::Process* proc;

	LPVOID originalTable; // the original VTable
	LPVOID newTable; // our custom VTable

	LPVOID tablePtr; // this points to the VTable

	int methodCount;


	void detour(int idx, LPVOID newMethod);


private:
	void updateMethodCount();
};