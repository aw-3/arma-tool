#include "vmt.h"
#include "main.h"

vmtHook::vmtHook(blackbone::Process* hProc, LPVOID vTablePtr) {
	this->proc = hProc;
	this->tablePtr = vTablePtr;

	// Store original table for later (unhooking)
	Read(*this->proc, (int)this->tablePtr, sizeof(LPVOID), &this->originalTable);

	// Get total VTable size
	//this->updateMethodCount(); // BROKEN ATM
	this->methodCount = 512;

	int tableSize = this->methodCount * sizeof(LPVOID);

	this->newTable = (LPVOID)DoAllocate(*this->proc, tableSize, PAGE_READWRITE);
	LPVOID *tableTemp = new LPVOID[methodCount];

	Read(*this->proc, (int)this->originalTable, tableSize, tableTemp);
	Write(*this->proc, (int)this->newTable, tableSize, tableTemp);

	delete tableTemp;

	Write(*this->proc, (int)this->tablePtr, sizeof(LPVOID), &this->newTable);

}

vmtHook::~vmtHook() {
	// Restore original vtable
	Write(*this->proc, (int)this->tablePtr, sizeof(LPVOID), &this->originalTable);
}

void vmtHook::detour(int idx, LPVOID newMethod) {
	if (this->newTable != NULL) {
		Write(*this->proc, (int)((int)this->newTable + idx * sizeof(LPVOID)), sizeof(LPVOID), &newMethod);
	}
}


void vmtHook::updateMethodCount() {
	for (this->methodCount = 0; /* INFINITE LOOP */; this->methodCount++) {
		LPVOID temp = NULL;
		Read(*this->proc, (int)((int)this->originalTable + this->methodCount * 4), sizeof(temp), &temp);
		

		if (temp == 0)
			break;
	}
}