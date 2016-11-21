#include "VadRoutines.h"
#include "VadHelpers.h"

#pragma alloc_text(PAGE, BBProtectVAD)
#pragma alloc_text(PAGE, BBUnlinkVAD)
#pragma alloc_text(PAGE, BBGetVadType)
#pragma alloc_text(PAGE, BBFindVAD)

extern DYNAMIC_DATA dynData;

ULONG MmProtectToValue[32] = 
{
    PAGE_NOACCESS,
    PAGE_READONLY,
    PAGE_EXECUTE,
    PAGE_EXECUTE_READ,
    PAGE_READWRITE,
    PAGE_WRITECOPY,
    PAGE_EXECUTE_READWRITE,
    PAGE_EXECUTE_WRITECOPY,
    PAGE_NOACCESS,
    PAGE_NOCACHE | PAGE_READONLY,
    PAGE_NOCACHE | PAGE_EXECUTE,
    PAGE_NOCACHE | PAGE_EXECUTE_READ,
    PAGE_NOCACHE | PAGE_READWRITE,
    PAGE_NOCACHE | PAGE_WRITECOPY,
    PAGE_NOCACHE | PAGE_EXECUTE_READWRITE,
    PAGE_NOCACHE | PAGE_EXECUTE_WRITECOPY,
    PAGE_NOACCESS,
    PAGE_GUARD | PAGE_READONLY,
    PAGE_GUARD | PAGE_EXECUTE,
    PAGE_GUARD | PAGE_EXECUTE_READ,
    PAGE_GUARD | PAGE_READWRITE,
    PAGE_GUARD | PAGE_WRITECOPY,
    PAGE_GUARD | PAGE_EXECUTE_READWRITE,
    PAGE_GUARD | PAGE_EXECUTE_WRITECOPY,
    PAGE_NOACCESS,
    PAGE_WRITECOMBINE | PAGE_READONLY,
    PAGE_WRITECOMBINE | PAGE_EXECUTE,
    PAGE_WRITECOMBINE | PAGE_EXECUTE_READ,
    PAGE_WRITECOMBINE | PAGE_READWRITE,
    PAGE_WRITECOMBINE | PAGE_WRITECOPY,
    PAGE_WRITECOMBINE | PAGE_EXECUTE_READWRITE,
    PAGE_WRITECOMBINE | PAGE_EXECUTE_WRITECOPY
};

/// <summary>
/// Change VAD protection flags
/// </summary>
/// <param name="pProcess">Target process object</param>
/// <param name="address">Target address</param>
/// <param name="prot">New protection flags</param>
/// <returns>Status code</returns>
NTSTATUS BBProtectVAD( IN PEPROCESS pProcess, IN ULONG_PTR address, IN ULONG prot )
{
    NTSTATUS status = STATUS_SUCCESS;
    PMMVAD_SHORT pVad = NULL;

    status = BBFindVAD( pProcess, address, &pVad );
    if (!NT_SUCCESS( status ))
        return status;

    pVad->u.VadFlags.Protection = prot;

    return status;
}

#pragma warning(disable : 4055)

/// <summary>
/// Hide memory from NtQueryVirtualMemory
/// </summary>
/// <param name="pProcess">Target process object</param>
/// <param name="address">Target address</param>
/// <returns>Status code</returns>
NTSTATUS BBUnlinkVAD( IN PEPROCESS pProcess, IN ULONG_PTR address )
{
    return BBProtectVAD( pProcess, address, MM_ZERO_ACCESS );  

    /*
#ifdef _WIN81_
    RtlAvlRemoveNode( (PMM_AVL_TABLE)((PUCHAR)pProcess + dynData.VadRoot), (PMMADDRESS_NODE)pVad );
#else
    MiRemoveNode( (PMMADDRESS_NODE)pVad, (PMM_AVL_TABLE)((PUCHAR)pProcess + dynData.VadRoot) );
#endif
    */
}

#pragma warning(default : 4055)

/// <summary>
/// Get region VAD type
/// </summary>
/// <param name="pProcess">Target process object</param>
/// <param name="address">Target address</param>
/// <param name="pType">Resulting VAD type</param>
/// <returns>Status code</returns>
NTSTATUS BBGetVadType( IN PEPROCESS pProcess, IN ULONG_PTR address, OUT PMI_VAD_TYPE pType )
{
    NTSTATUS status = STATUS_SUCCESS;
    PMMVAD_SHORT pVad = NULL;

    status = BBFindVAD( pProcess, address, &pVad );
    if (!NT_SUCCESS( status ))
        return status;

    *pType = pVad->u.VadFlags.VadType;

    return status;
}

/// <summary>
/// Find VAD that describes target address
/// </summary>
/// <param name="pProcess">Target process object</param>
/// <param name="address">Address to find</param>
/// <param name="pResult">Found VAD. NULL if not found</param>
/// <returns>Status code</returns>
NTSTATUS BBFindVAD( IN PEPROCESS pProcess, IN ULONG_PTR address, OUT PMMVAD_SHORT* pResult )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR vpnStart = address >> PAGE_SHIFT;

    ASSERT( pProcess != NULL && pResult != NULL );
    if (pProcess == NULL || pResult == NULL)
        return STATUS_INVALID_PARAMETER;

    if (dynData.VadRoot == 0)
    {
        DPRINT( "BlackBone: %s: Invalid VadRoot offset\n", __FUNCTION__ );
        status = STATUS_INVALID_ADDRESS;
    }

    __try
    {
        PMM_AVL_TABLE pTable = (PMM_AVL_TABLE)((PUCHAR)pProcess + dynData.VadRoot);
        PMM_AVL_NODE pNode = GET_VAD_ROOT( pTable );

        // Search VAD
        if (MiFindNodeOrParent( pTable, vpnStart, &pNode ) == TableFoundNode)
        {
            *pResult = (PMMVAD_SHORT)pNode;
        }
        else
        {
            DPRINT( "BlackBone: %s: VAD entry for address 0x%p not found\n", __FUNCTION__, address );
            status = STATUS_NOT_FOUND;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT( "BlackBone: %s: Exception\n", __FUNCTION__ );
        status = STATUS_UNHANDLED_EXCEPTION;
    }

    return status;
}

/// <summary>
/// Convert protection flags
/// </summary>
/// <param name="prot">Protection flags.</param>
/// <param name="fromPTE">If TRUE - convert to PTE protection, if FALSE - convert to Win32 protection</param>
/// <returns>Resulting protection flags</returns>
ULONG BBConvertProtection( IN ULONG prot, IN BOOLEAN fromPTE )
{
    if (fromPTE != FALSE)
    {
        // Sanity check
        if (prot < ARRAYSIZE( MmProtectToValue ))
            return MmProtectToValue[prot];
    }
    else
    {
        for (int i = 0; i < ARRAYSIZE( MmProtectToValue ); i++)
            if (MmProtectToValue[i] == prot)
                return i;
    }

    return 0;
}