#pragma once

#include "../Config.h"
#include "NativeStructures.h"

namespace blackbone
{

// NtCreateEvent
typedef NTSTATUS( NTAPI* fnNtCreateEvent )
    ( 
        OUT PHANDLE             EventHandle,
        IN ACCESS_MASK          DesiredAccess,
        IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
        IN ULONG                EventType,
        IN BOOLEAN              InitialState 
    );

// NtOpenEvent
typedef NTSTATUS( NTAPI* fnNtOpenEvent )
    ( 
        OUT PHANDLE             EventHandle,
        IN ACCESS_MASK          DesiredAccess,
        IN POBJECT_ATTRIBUTES   ObjectAttributes
    );

// NtQueryVirtualMemory
typedef NTSTATUS( NTAPI* fnNtQueryVirtualMemory )
    (
        IN HANDLE   ProcessHandle,
        IN PVOID    BaseAddress,
        IN MEMORY_INFORMATION_CLASS MemoryInformationClass,
        OUT PVOID   MemoryInformation,
        IN SIZE_T   MemoryInformationLength,
        OUT PSIZE_T ReturnLength
    );

// NtWow64QueryInformationProcess64
typedef NTSTATUS( NTAPI *fnNtWow64QueryInformationProcess64 )
    (
        IN  HANDLE ProcessHandle,
        IN  ULONG  ProcessInformationClass,
        OUT PVOID  ProcessInformation64,
        IN  ULONG  Length,
        OUT PULONG ReturnLength OPTIONAL
    );

// NtWow64ReadVirtualMemory64
typedef NTSTATUS( NTAPI *fnNtWow64ReadVirtualMemory64 )
    (
        IN  HANDLE   ProcessHandle,
        IN  ULONG64  BaseAddress,
        OUT PVOID    Buffer,
        IN  ULONG64  BufferLength,
        OUT PULONG64 ReturnLength OPTIONAL
    );

// NtWow64WriteVirtualMemory64
typedef fnNtWow64ReadVirtualMemory64 fnNtWow64WriteVirtualMemory64;

// NtWow64AllocateVirtualMemory64
typedef NTSTATUS( NTAPI *fnNtWow64AllocateVirtualMemory64 )
    (
        IN  HANDLE   ProcessHandle,
        IN  PULONG64 BaseAddress,
        IN  ULONG64  ZeroBits,
        IN  PULONG64 Size,
        IN  ULONG    AllocationType,
        IN  ULONG    Protection
    );

// NtWow64QueryVirtualMemory64
typedef NTSTATUS( NTAPI *fnNtWow64QueryVirtualMemory64 )
    (
        IN HANDLE   ProcessHandle,
        IN ULONG64  BaseAddress,
        IN DWORD    MemoryInformationClass,
        OUT PVOID   Buffer,
        IN ULONG64  Length,
        OUT PULONG  ResultLength OPTIONAL
    );

// RtlDosApplyFileIsolationRedirection_Ustr
typedef NTSTATUS( NTAPI *fnRtlDosApplyFileIsolationRedirection_Ustr )
    (
        IN ULONG Flags,
        IN PUNICODE_STRING OriginalName,
        IN PUNICODE_STRING Extension,
        IN OUT PUNICODE_STRING StaticString,
        IN OUT PUNICODE_STRING DynamicString,
        IN OUT PUNICODE_STRING *NewName,
        IN PULONG  NewFlags,
        IN PSIZE_T FileNameSize,
        IN PSIZE_T RequiredLength
    );

// RtlDosPathNameToNtPathName_U
typedef BOOLEAN( NTAPI *fnRtlDosPathNameToNtPathName_U )
    (
        IN PCWSTR DosFileName,
        OUT PUNICODE_STRING NtFileName,
        OUT OPTIONAL PWSTR *FilePart,
        OUT OPTIONAL PVOID RelativeName
    );

// RtlHashUnicodeString
typedef NTSTATUS( NTAPI *fnRtlHashUnicodeString )
    ( 
        IN   PCUNICODE_STRING String,
        IN   BOOLEAN CaseInSensitive,
        IN   ULONG HashAlgorithm,
        OUT  PULONG HashValue
    );

// RtlRemoteCall
typedef NTSTATUS( NTAPI *fnRtlRemoteCall )
    (
        IN HANDLE Process,
        IN HANDLE Thread,
        IN PVOID CallSite,
        IN ULONG ArgumentCount,
        IN PULONG Arguments,
        IN BOOLEAN PassContext,
        IN BOOLEAN AlreadySuspended
    );

// NtCreateThreadEx
typedef NTSTATUS( NTAPI* fnNtCreateThreadEx )
    (
        OUT PHANDLE hThread,
        IN ACCESS_MASK DesiredAccess,
        IN LPVOID ObjectAttributes,
        IN HANDLE ProcessHandle,
        IN LPTHREAD_START_ROUTINE lpStartAddress,
        IN LPVOID lpParameter,
        IN DWORD Flags,
        IN SIZE_T StackZeroBits,
        IN SIZE_T SizeOfStackCommit,
        IN SIZE_T SizeOfStackReserve,
        OUT LPVOID lpBytesBuffer
    );

// NtLockVirtualMemory
typedef NTSTATUS( NTAPI* fnNtLockVirtualMemory )
    (
        IN HANDLE process, 
        IN OUT PVOID* baseAddress, 
        IN OUT ULONG* size, 
        IN ULONG flags
    );

// RtlRbInsertNodeEx
typedef int ( NTAPI* fnRtlRbInsertNodeEx )
    (
        PRTL_RB_TREE 	    Tree,
        PRTL_BALANCED_NODE 	Parent,
        BOOLEAN             Right,
        PRTL_BALANCED_NODE 	Node
    );

// NtSetInformationProcess
typedef NTSTATUS( NTAPI* fnNtSetInformationProcess )
    (
        IN HANDLE   ProcessHandle,
        IN PROCESSINFOCLASS ProcessInformationClass,
        IN PVOID    ProcessInformation,
        IN ULONG    ProcessInformationLength 
    );

// NtDuplicateObject
typedef NTSTATUS( NTAPI* fnNtDuplicateObject )
    (
        IN HANDLE SourceProcessHandle,
        IN HANDLE SourceHandle,
        IN HANDLE TargetProcessHandle,
        IN PHANDLE TargetHandle,
        IN ACCESS_MASK DesiredAccess,
        IN ULONG Attributes,
        IN ULONG Options
    );

// RtlRbRemoveNode
typedef int (NTAPI* fnRtlRbRemoveNode)
    (
        PRTL_RB_TREE        Tree,
        PRTL_BALANCED_NODE 	Node
    );

// RtlUpcaseUnicodeChar
typedef WCHAR( NTAPI *fnRtlUpcaseUnicodeChar )
    (
        WCHAR chr
    );

// RtlEncodeSystemPointer
typedef PVOID( NTAPI *fnRtlEncodeSystemPointer )
    (
        IN PVOID Pointer
    );

// NtLoadDriver
typedef NTSTATUS( NTAPI* fnNtLoadDriver )
    (
        IN PUNICODE_STRING path
    );

// NtUnloadDriver
typedef NTSTATUS( NTAPI* fnNtUnloadDriver )
    (
        IN PUNICODE_STRING path
    );

// NtQuerySection
typedef DWORD( NTAPI* fnNtQuerySection )
    (
        HANDLE hSection,
        SECTION_INFORMATION_CLASS InfoClass, 
        PVOID Buffer, 
        ULONG BufferSize, 
        PULONG ReturnLength
    );

// RtlCreateActivationContext
typedef NTSTATUS( NTAPI *fnRtlCreateActivationContext )
    (
        IN ULONG    Flags,
        IN PACTCTXW ActivationContextData,
        IN ULONG    ExtraBytes,
        IN PVOID    NotificationRoutine,
        IN PVOID    NotificationContext,
        OUT PVOID*  ActCtx
    );

// RtlInitUnicodeString
typedef decltype(&RtlInitUnicodeString) fnRtlInitUnicodeString;

// RtlFreeUnicodeString
typedef decltype(&RtlFreeUnicodeString) fnRtlFreeUnicodeString;

// NtQuerySystemInformation
typedef decltype(&NtQuerySystemInformation) fnNtQuerySystemInformation;

// NtQueryInformationProcess
typedef decltype(&NtQueryInformationProcess) fnNtQueryInformationProcess;

// NtQueryInformationThread
typedef decltype(&NtQueryInformationThread) fnNtQueryInformationThread;

// NtQueryObject
typedef decltype(&NtQueryObject) fnNtQueryObject;

//
// GCC compatibility
//

// Wow64GetThreadContext
typedef BOOL( __stdcall* fnWow64GetThreadContext )
    (
        HANDLE hThread,
        PWOW64_CONTEXT lpContext
    );

// Wow64SetThreadContext
typedef BOOL( __stdcall* fnWow64SetThreadContext )
    (
        HANDLE hThread,
        const WOW64_CONTEXT *lpContext
    );

// Wow64SuspendThread
typedef DWORD( __stdcall* fnWow64SuspendThread )
    (
        HANDLE hThread
    );

// GetProcessDEPPolicy
typedef BOOL( __stdcall* fnGetProcessDEPPolicy )
    (
        HANDLE  hProcess,
        LPDWORD lpFlags,
        PBOOL   lpPermanent
    );

// QueryFullProcessImageNameW
typedef BOOL( __stdcall* fnQueryFullProcessImageNameW)
    (
        HANDLE hProcess,
        DWORD  dwFlags,
        PWSTR  lpExeName,
        PDWORD lpdwSize
    );

}