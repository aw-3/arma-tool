#pragma once

#include "../Include/Winheaders.h"
#include "RPC/RemoteMemory.h"
#include "MemBlock.h"

#include <vector>
#include <list>

namespace blackbone
{

class ProcessMemory : public RemoteMemory
{
public:
    BLACKBONE_API ProcessMemory( class Process* process );
    BLACKBONE_API ~ProcessMemory();

    /// <summary>
    /// Allocate new memory block
    /// </summary>
    /// <param name="size">Block size</param>
    /// <param name="protection">Memory protection</param>
    /// <param name="desired">Desired base address of new block</param>
    /// <returns>Memory block. If failed - returned block will be invalid</returns>
    BLACKBONE_API MemBlock Allocate( size_t size, DWORD protection = PAGE_EXECUTE_READWRITE, ptr_t desired = 0 );

    /// <summary>
    /// Free memory
    /// </summary>
    /// <param name="pAddr">Memory address to release.</param>
    /// <param name="size">Region size</param>
    /// <param name="freeType">Release/decommit</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS Free( ptr_t pAddr, size_t size = 0, DWORD freeType = MEM_RELEASE );

    /// <summary>
    /// Get memory region info
    /// </summary>
    /// <param name="pAddr">Memory address</param>
    /// <param name="pInfo">Retrieved info</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS Query( ptr_t pAddr, PMEMORY_BASIC_INFORMATION64 pInfo );

    /// <summary>
    /// Change memory protection
    /// </summary>
    /// <param name="pAddr">Memory address</param>
    /// <param name="size">Region size</param>
    /// <param name="flProtect">New memory protection flags</param>
    /// <param name="pOld">Old protection flags</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS Protect( ptr_t pAddr, size_t size, DWORD flProtect, DWORD *pOld = NULL );

    /// <summary>
    /// Read data
    /// </summary>
    /// <param name="dwAddress">Memoey address to read from</param>
    /// <param name="dwSize">Size of data to read</param>
    /// <param name="pResult">Output buffer</param>
    /// <param name="handleHoles">
    /// If true, function will try to read all committed pages in range ignoring uncommitted ones.
    /// Otherwise function will fail if there is at least one non-committed page in region.
    /// </param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS Read( ptr_t dwAddress, size_t dwSize, PVOID pResult, bool handleHoles = false );

    /// <summary>
    /// Write data
    /// </summary>
    /// <param name="pAddress">Memory address to write to</param>
    /// <param name="dwSize">Size of data to write</param>
    /// <param name="pData">Buffer to write</param>
    /// <returns>Status</returns>
    BLACKBONE_API NTSTATUS Write( ptr_t pAddress, size_t dwSize, const void* pData );

    /// <summary>
    /// Read data
    /// </summary>
    /// <param name="dwAddress">Address to read from</param>
    /// <returns>Read data</returns>
    template<class T>
    inline T Read( ptr_t dwAddress )
    {
        T res; 
        Read( dwAddress, sizeof(T), &res );
        return res;
    };

    /// <summary>
    /// Read data
    /// </summary>
    /// <param name="dwAddress">Address to read from</param>
    /// <param name="result">Read data</param>
    /// <returns>Status code</returns>
    template<class T>
    inline NTSTATUS Read( ptr_t dwAddress, T& result )
    {
        return Read( dwAddress, sizeof( result ), &result );
    };

    /// <summary>
    /// Write data
    /// </summary>
    /// <param name="dwSize">Size of data to write</param>
    /// <param name="data">Data to write</param>
    /// <returns>Status</returns>
    template<class T>
    inline NTSTATUS Write( ptr_t dwAddress, const T& data )
    {
        return Write( dwAddress, sizeof(T), &data );
    }

    /// <summary>
    /// Enumerate valid memory regions
    /// </summary>
    /// <param name="results">Found regions</param>
    /// <param name="includeFree">If true - non-allocated regions will be included in list</param>
    /// <returns>Number of regions found</returns>
    BLACKBONE_API size_t EnumRegions( std::list<MEMORY_BASIC_INFORMATION64>& results, bool includeFree = false );

    BLACKBONE_API inline class ProcessCore& core() { return _core; }
    BLACKBONE_API inline class Process* process()  { return _process; }

private:
    ProcessMemory( const ProcessMemory& ) = delete;
    ProcessMemory& operator =(const ProcessMemory&) = delete;

private:
    class Process* _process;    // Owning process object
    class ProcessCore& _core;   // Core routines
};

}