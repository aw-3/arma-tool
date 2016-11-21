#include "BlackBoneDrv.h"
#include "Routines.h"
#include <Ntstrsafe.h>

LIST_ENTRY g_PhysProcesses;

/// <summary>
/// Find allocated memory region entry
/// </summary>
/// <param name="pList">Region list</param>
/// <param name="pBase">Region base</param>
/// <returns>Found entry, NULL if not found</returns>
PMEM_PHYS_ENTRY BBLookupPhysMemEntry( IN PLIST_ENTRY pList, IN PVOID pBase );

#pragma alloc_text(PAGE, BBDisableDEP)
#pragma alloc_text(PAGE, BBSetProtection)
#pragma alloc_text(PAGE, BBGrantAccess)
#pragma alloc_text(PAGE, BBCopyMemory)
#pragma alloc_text(PAGE, BBAllocateFreeMemory)
#pragma alloc_text(PAGE, BBAllocateFreePhysical)
#pragma alloc_text(PAGE, BBProtectMemory)

#pragma alloc_text(PAGE, BBLookupPhysProcessEntry)
#pragma alloc_text(PAGE, BBLookupPhysMemEntry)

#pragma alloc_text(PAGE, BBCleanupPhysMemEntry)
#pragma alloc_text(PAGE, BBCleanupProcessPhysEntry)
#pragma alloc_text(PAGE, BBCleanupProcessPhysList)

/// <summary>
/// Disable process DEP
/// Has no effect on native x64 process
/// </summary>
/// <param name="pData">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBDisableDEP( IN PDISABLE_DEP pData )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKEXECUTE_OPTIONS pExecOpt = NULL;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pData->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        if (dynData.KExecOpt != 0)
        {
            pExecOpt = (PKEXECUTE_OPTIONS)((PUCHAR)pProcess + dynData.KExecOpt);

            // Reset all flags
            pExecOpt->ExecuteOptions = 0;

            pExecOpt->Flags.ExecuteEnable = 1;          //
            pExecOpt->Flags.ImageDispatchEnable = 1;    // Disable all checks
            pExecOpt->Flags.ExecuteDispatchEnable = 1;  //
        }
        else
        {
            DPRINT( "BlackBone: %s: Invalid _KEXECUTE_OPTIONS offset\n", __FUNCTION__ );
            status = STATUS_INVALID_ADDRESS;
        }
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Enable/disable process protection flag
/// </summary>
/// <param name="pProtection">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBSetProtection( IN PSET_PROC_PROTECTION pProtection )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pProtection->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        if (dynData.Protection != 0)
        {
            // Win7
            if (dynData.ver <= WINVER_7_SP1)
            {
                if (pProtection->enableState)
                    *(PULONG)((PUCHAR)pProcess + dynData.Protection) |= 1 << 0xB;
                else
                    *(PULONG)((PUCHAR)pProcess + dynData.Protection) &= ~(1 << 0xB);
            }
            // Win8
            else if (dynData.ver == WINVER_8)
            {
                *((PUCHAR)pProcess + dynData.Protection) = pProtection->enableState;
            }
            // Win8.1
            else if (dynData.ver >= WINVER_81)
            {
                PS_PROTECTION protBuf = { 0 };

                if (pProtection->enableState == FALSE)
                {
                    protBuf.Level = 0;
                }
                else
                {
                    protBuf.Flags.Signer = PsProtectedSignerWinTcb;
                    protBuf.Flags.Type = PsProtectedTypeProtected;
                }

                *((PUCHAR)pProcess + dynData.Protection) = protBuf.Level;
            }
            else
                status = STATUS_NOT_SUPPORTED;
        }
        else
        {
            DPRINT( "BlackBone: %s: Invalid protection flag offset\n", __FUNCTION__ );
            status = STATUS_INVALID_ADDRESS;
        }
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Change handle granted access
/// </summary>
/// <param name="pAccess">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBGrantAccess( IN PHANDLE_GRANT_ACCESS pAccess )
{
    NTSTATUS  status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;
    PHANDLE_TABLE pTable = NULL;
    PHANDLE_TABLE_ENTRY pHandleEntry = NULL;
    EXHANDLE exHandle;

    // Validate dynamic offset
    if (dynData.ObjTable == 0)
    {
        DPRINT( "BlackBone: %s: Invalid ObjTable address\n", __FUNCTION__ );
        return STATUS_INVALID_ADDRESS;
    }

    status = PsLookupProcessByProcessId( (HANDLE)pAccess->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        pTable = *(PHANDLE_TABLE*)((PUCHAR)pProcess + dynData.ObjTable);
        exHandle.Value = (ULONG_PTR)pAccess->handle;

        if (pTable)
            pHandleEntry = ExpLookupHandleTableEntry( pTable, exHandle );

        if (ExpIsValidObjectEntry( pHandleEntry ))
        {
            pHandleEntry->GrantedAccessBits = pAccess->access;
        }
        else
        {
            DPRINT( "BlackBone: %s: 0x%X:0x%X handle is invalid. HandleEntry = 0x%p\n", 
                    __FUNCTION__, pAccess->pid, pAccess->handle, pHandleEntry );

            status = STATUS_UNSUCCESSFUL;
        }
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Change handle granted access
/// </summary>
/// <param name="pAccess">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBUnlinkHandleTable( IN PUNLINK_HTABLE pUnlink )
{
    NTSTATUS  status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    // Validate dynamic offset
    if (dynData.ExRemoveTable == 0 || dynData.ObjTable == 0)
    {
        DPRINT( "BlackBone: %s: Invalid ExRemoveTable/ObjTable address\n", __FUNCTION__ );
        return STATUS_INVALID_ADDRESS;
    }

    // Validate build
    if (dynData.correctBuild == FALSE)
    {
        DPRINT( "BlackBone: %s: Unsupported kernel build version\n", __FUNCTION__ );
        return STATUS_INVALID_KERNEL_INFO_VERSION;
    }

    status = PsLookupProcessByProcessId( (HANDLE)pUnlink->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        PHANDLE_TABLE pTable = *(PHANDLE_TABLE*)((PUCHAR)pProcess + dynData.ObjTable);

        // Unlink process handle table
        fnExRemoveHandleTable ExRemoveHandleTable = (fnExRemoveHandleTable)((ULONG_PTR)GetKernelBase() + dynData.ExRemoveTable);
        //DPRINT( "BlackBone: %s: ExRemoveHandleTable address 0x%p. Object Table offset: 0x%X\n", 
               // __FUNCTION__, ExRemoveHandleTable, dynData.ObjTable );

        ExRemoveHandleTable( pTable );
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Read/write process memory
/// </summary>
/// <param name="pCopy">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBCopyMemory( IN PCOPY_MEMORY pCopy )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL, pSourceProc = NULL, pTargetProc = NULL;
    PVOID pSource = NULL, pTarget = NULL;

    __try
    {
        status = PsLookupProcessByProcessId( (HANDLE)pCopy->pid, &pProcess );

        if (NT_SUCCESS( status ))
        {
            SIZE_T bytes = 0;

            // Write
            if (pCopy->write != FALSE)
            {
                pSourceProc = PsGetCurrentProcess();
                pTargetProc = pProcess;
                pSource = (PVOID)pCopy->localbuf;
                pTarget = (PVOID)pCopy->targetPtr;
            }
            // Read
            else
            {
                pSourceProc = pProcess;
                pTargetProc = PsGetCurrentProcess();
                pSource = (PVOID)pCopy->targetPtr;
                pTarget = (PVOID)pCopy->localbuf;
            }

            status = MmCopyVirtualMemory( pSourceProc, pSource, pTargetProc, pTarget, pCopy->size, KernelMode, &bytes );
        }
        else
            DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception\n", __FUNCTION__ );
        status = STATUS_UNHANDLED_EXCEPTION;
    }

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}


/// <summary>
/// Allocate/Free process memory
/// </summary>
/// <param name="pAllocFree">Request params.</param>
/// <param name="pResult">Allocated region info.</param>
/// <returns>Status code</returns>
NTSTATUS BBAllocateFreeMemory( IN PALLOCATE_FREE_MEMORY pAllocFree, OUT PALLOCATE_FREE_MEMORY_RESULT pResult )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    ASSERT( pResult != NULL );
    if (pResult == NULL)
        return STATUS_INVALID_PARAMETER;

    status = PsLookupProcessByProcessId( (HANDLE)pAllocFree->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        KAPC_STATE apc;
        PVOID base = (PVOID)pAllocFree->base;
        ULONG_PTR size = pAllocFree->size;

        KeStackAttachProcess( pProcess, &apc );

        if (pAllocFree->allocate)
        {
            if (pAllocFree->physical != FALSE)
            {
                status = BBAllocateFreePhysical( pProcess, pAllocFree, pResult );
            }
            else
            {
                status = ZwAllocateVirtualMemory( ZwCurrentProcess(), &base, 0, &size, pAllocFree->type, pAllocFree->protection );
                pResult->address = (ULONGLONG)base;
                pResult->size = size;
            }
        }
        else
        {
            MI_VAD_TYPE vadType = VadNone;
            BBGetVadType( pProcess, pAllocFree->base, &vadType );

            if (vadType == VadDevicePhysicalMemory)
                status = BBAllocateFreePhysical( pProcess, pAllocFree, pResult );
            else
                status = ZwFreeVirtualMemory( ZwCurrentProcess(), &base, &size, pAllocFree->type );
        }

        KeUnstackDetachProcess( &apc );        
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}


/// <summary>
/// Allocate kernel memory and map into User space. Or free previously allocated memory
/// </summary>
/// <param name="pProcess">Target process object</param>
/// <param name="pAllocFree">Request params.</param>
/// <param name="pResult">Allocated region info.</param>
/// <returns>Status code</returns>
NTSTATUS BBAllocateFreePhysical( IN PEPROCESS pProcess, IN PALLOCATE_FREE_MEMORY pAllocFree, OUT PALLOCATE_FREE_MEMORY_RESULT pResult )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID pRegionBase = NULL;
    PMDL pMDL = NULL;

    ASSERT( pProcess != NULL && pResult != NULL );
    if (pProcess == NULL || pResult == NULL)
        return STATUS_INVALID_PARAMETER;

    // MDL doesn't support regions this large
    if (pAllocFree->size > 0xFFFFFFFF)
    {
        DPRINT( "BlackBone: %s: Region size if too big: 0x%p\n", __FUNCTION__, pAllocFree->size );
        return STATUS_INVALID_PARAMETER;
    }

    // Align on page boundaries   
    pAllocFree->base = (ULONGLONG)PAGE_ALIGN( pAllocFree->base );
    pAllocFree->size = ADDRESS_AND_SIZE_TO_SPAN_PAGES( pAllocFree->base, pAllocFree->size ) << PAGE_SHIFT;

    // Allocate
    if (pAllocFree->allocate != FALSE)
    {
        PMMVAD_SHORT pVad = NULL;
        if (pAllocFree->base != 0 && BBFindVAD( pProcess, pAllocFree->base, &pVad ) != STATUS_NOT_FOUND)
            return STATUS_ALREADY_COMMITTED;

        pRegionBase = ExAllocatePoolWithTag( NonPagedPool, pAllocFree->size, BB_POOL_TAG );
        if (!pRegionBase)
            return STATUS_NO_MEMORY;

        // Cleanup buffer before mapping it into UserMode to prevent exposure of kernel data
        RtlZeroMemory( pRegionBase, pAllocFree->size );

        pMDL = IoAllocateMdl( pRegionBase, (ULONG)pAllocFree->size, FALSE, FALSE, NULL );
        if (pMDL == NULL)
        {
            ExFreePoolWithTag( pRegionBase, BB_POOL_TAG );
            return STATUS_NO_MEMORY;
        }

        MmBuildMdlForNonPagedPool( pMDL );

        // Map at original base
        __try {
            pResult->address = (ULONGLONG)MmMapLockedPagesSpecifyCache( pMDL, UserMode, MmCached,
                                                                        (PVOID)pAllocFree->base, FALSE, NormalPagePriority );
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { }

        // Map at any suitable
        if (pResult->address == 0 && pAllocFree->base != 0)
        {
            __try {
                pResult->address = (ULONGLONG)MmMapLockedPagesSpecifyCache( pMDL, UserMode, MmCached,
                                                                            NULL, FALSE, NormalPagePriority );
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { }
        }

        if (pResult->address)
        {
            PMEM_PHYS_PROCESS_ENTRY pEntry = NULL;
            PMEM_PHYS_ENTRY pMemEntry = NULL;

            pResult->size = pAllocFree->size;

            // Set initial protection
            BBProtectVAD( PsGetCurrentProcess(), pResult->address, BBConvertProtection( pAllocFree->protection, FALSE ) );

            // Make pages executable
            if (pAllocFree->protection & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))
            {
                for (ULONG_PTR pAdress = pResult->address; pAdress < pResult->address + pResult->size; pAdress += PAGE_SIZE)
                    GetPTEForVA( (PVOID)pAdress )->u.Hard.NoExecute = 0;
            }

            // Add to list
            pEntry = BBLookupPhysProcessEntry( (HANDLE)pAllocFree->pid );
            if (pEntry == NULL)
            {
                pEntry = ExAllocatePoolWithTag( PagedPool, sizeof( MEM_PHYS_PROCESS_ENTRY ), BB_POOL_TAG );
                pEntry->pid = (HANDLE)pAllocFree->pid;

                InitializeListHead( &pEntry->pVadList );
                InsertTailList( &g_PhysProcesses, &pEntry->link );
            }

            pMemEntry = ExAllocatePoolWithTag( PagedPool, sizeof( MEM_PHYS_ENTRY ), BB_POOL_TAG );
            
            pMemEntry->pMapped = (PVOID)pResult->address;
            pMemEntry->pMDL = pMDL;
            pMemEntry->ptr = pRegionBase;
            pMemEntry->size = pAllocFree->size;

            InsertTailList( &pEntry->pVadList, &pMemEntry->link );
        }
        else
        {
            // Failed, cleanup
            IoFreeMdl( pMDL );
            ExFreePoolWithTag( pRegionBase, BB_POOL_TAG );

            status = STATUS_NONE_MAPPED;
        }
    }
    // Free
    else
    {
        PMEM_PHYS_PROCESS_ENTRY pEntry = BBLookupPhysProcessEntry( (HANDLE)pAllocFree->pid );

        if (pEntry != NULL)
        {
            PMEM_PHYS_ENTRY pMemEntry = BBLookupPhysMemEntry( &pEntry->pVadList, (PVOID)pAllocFree->base );

            if (pMemEntry != NULL)
                BBCleanupPhysMemEntry( pMemEntry, TRUE );
            else
                status = STATUS_NOT_FOUND;
        }
        else
            status = STATUS_NOT_FOUND;
    }

    return status;
}


/// <summary>
/// Change process memory protection
/// </summary>
/// <param name="pProtect">Request params</param>
/// <returns>Status code</returns>
NTSTATUS BBProtectMemory( IN PPROTECT_MEMORY pProtect )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pProtect->pid, &pProcess );
    if (NT_SUCCESS( status ))
    {
        KAPC_STATE apc;
        MI_VAD_TYPE vadType = VadNone;
        PVOID base = (PVOID)pProtect->base;
        SIZE_T size = (SIZE_T)pProtect->size;
        ULONG oldProt = 0;

        KeStackAttachProcess( pProcess, &apc );

        // Handle physical allocations
        status = BBGetVadType( pProcess, pProtect->base, &vadType );
        if (NT_SUCCESS( status ))
        {
            if (vadType == VadDevicePhysicalMemory)
            {
                // Align on page boundaries   
                pProtect->base = (ULONGLONG)PAGE_ALIGN( pProtect->base );
                pProtect->size = ADDRESS_AND_SIZE_TO_SPAN_PAGES( pProtect->base, pProtect->size ) << PAGE_SHIFT;

                status = BBProtectVAD( pProcess, pProtect->base, BBConvertProtection( pProtect->newProtection, FALSE ) );

                // Update PTE
                for (ULONG_PTR pAdress = pProtect->base; pAdress < pProtect->base + pProtect->size; pAdress += PAGE_SIZE)
                {
                    PMMPTE pPTE = GetPTEForVA( (PVOID)pAdress );

                    // Executable
                    if (pProtect->newProtection & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
                        pPTE->u.Hard.NoExecute = 0;

                    // Read-only
                    if (pProtect->newProtection & (PAGE_READONLY | PAGE_EXECUTE | PAGE_EXECUTE_READ))
                        pPTE->u.Hard.Dirty1 = pPTE->u.Hard.Write = 0;
                }
            }
            else
                status = ZwProtectVirtualMemory( ZwCurrentProcess(), &base, &size, pProtect->newProtection, &oldProt );
        }

        KeUnstackDetachProcess( &apc );
    }
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Hide VAD containing target address
/// </summary>
/// <param name="pData">Address info</param>
/// <returns>Status code</returns>
NTSTATUS BBHideVAD( IN PHIDE_VAD pData )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS pProcess = NULL;

    status = PsLookupProcessByProcessId( (HANDLE)pData->pid, &pProcess );
    if (NT_SUCCESS( status ))
        status = BBProtectVAD( pProcess, pData->base, MM_ZERO_ACCESS );
    else
        DPRINT( "BlackBone: %s: PsLookupProcessByProcessId failed with status 0x%X\n", __FUNCTION__, status );

    if (pProcess)
        ObDereferenceObject( pProcess );

    return status;
}

/// <summary>
/// Find memory allocation process entry
/// </summary>
/// <param name="pid">Target PID</param>
/// <returns>Found entry, NULL if not found</returns>
PMEM_PHYS_PROCESS_ENTRY BBLookupPhysProcessEntry( IN HANDLE pid )
{
    for (PLIST_ENTRY pListEntry = g_PhysProcesses.Flink; pListEntry != &g_PhysProcesses; pListEntry = pListEntry->Flink)
    {
        PMEM_PHYS_PROCESS_ENTRY pEntry = CONTAINING_RECORD( pListEntry, MEM_PHYS_PROCESS_ENTRY, link );
        if (pEntry->pid == pid)
            return pEntry;
    }

    return NULL;
}


/// <summary>
/// Find allocated memory region entry
/// </summary>
/// <param name="pList">Region list</param>
/// <param name="pBase">Region base</param>
/// <returns>Found entry, NULL if not found</returns>
PMEM_PHYS_ENTRY BBLookupPhysMemEntry( IN PLIST_ENTRY pList, IN PVOID pBase )
{
    ASSERT( pList != NULL );
    if (pList == NULL)
        return NULL;

    for (PLIST_ENTRY pListEntry = pList->Flink; pListEntry != pList; pListEntry = pListEntry->Flink)
    {
        PMEM_PHYS_ENTRY pEntry = CONTAINING_RECORD( pListEntry, MEM_PHYS_ENTRY, link );
        if (pBase >= pEntry->pMapped && pBase < (PVOID)((ULONG_PTR)pEntry->pMapped + pEntry->size))
            return pEntry;
    }

    return NULL;
}

//
// Cleanup routines
//

void BBCleanupPhysMemEntry( IN PMEM_PHYS_ENTRY pEntry, BOOLEAN attached )
{
    ASSERT( pEntry != NULL );
    if (pEntry == NULL)
        return;

    if (attached)
        MmUnmapLockedPages( pEntry->pMapped, pEntry->pMDL );

    IoFreeMdl( pEntry->pMDL );
    ExFreePoolWithTag( pEntry->ptr, BB_POOL_TAG );

    RemoveEntryList( &pEntry->link );
    ExFreePoolWithTag( pEntry, BB_POOL_TAG );
}

void BBCleanupProcessPhysEntry( IN PMEM_PHYS_PROCESS_ENTRY pEntry, BOOLEAN attached )
{
    ASSERT( pEntry != NULL );
    if (pEntry == NULL)
        return;

    while (!IsListEmpty( &pEntry->pVadList ))
        BBCleanupPhysMemEntry( (PMEM_PHYS_ENTRY)pEntry->pVadList.Flink, attached );

    RemoveEntryList( &pEntry->link );
    ExFreePoolWithTag( pEntry, BB_POOL_TAG );
}

void BBCleanupProcessPhysList()
{
    while (!IsListEmpty( &g_PhysProcesses ))
        BBCleanupProcessPhysEntry( (PMEM_PHYS_PROCESS_ENTRY)g_PhysProcesses.Flink, FALSE );
}