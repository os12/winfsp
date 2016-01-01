/**
 * @file winfsp/winfsp.h
 *
 * @copyright 2015 Bill Zissimopoulos
 */

#ifndef WINFSP_WINFSP_H_INCLUDED
#define WINFSP_WINFSP_H_INCLUDED

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <winternl.h>
#include <ntstatus.h>

#if defined(WINFSP_DLL_INTERNAL)
#define FSP_API                         __declspec(dllexport)
#else
#define FSP_API                         __declspec(dllimport)
#endif

#include <winfsp/fsctl.h>

/*
 * File System
 */
typedef struct _FSP_FILE_SYSTEM FSP_FILE_SYSTEM;
typedef VOID FSP_FILE_SYSTEM_DISPATCHER(FSP_FILE_SYSTEM *, FSP_FSCTL_TRANSACT_REQ *);
typedef NTSTATUS FSP_FILE_SYSTEM_OPERATION(FSP_FILE_SYSTEM *, FSP_FSCTL_TRANSACT_REQ *);
typedef struct _FSP_FILE_SYSTEM
{
    UINT16 Version;
    WCHAR VolumePath[FSP_FSCTL_VOLUME_NAME_SIZEMAX / sizeof(WCHAR)];
    HANDLE VolumeHandle;
    PVOID UserContext;
    NTSTATUS DispatcherResult;
    FSP_FILE_SYSTEM_DISPATCHER *Dispatcher;
    FSP_FILE_SYSTEM_DISPATCHER *EnterOperation, *LeaveOperation;
    FSP_FILE_SYSTEM_OPERATION *Operations[FspFsctlTransactKindCount];
    NTSTATUS (*AccessCheck)(FSP_FILE_SYSTEM *, FSP_FSCTL_TRANSACT_REQ *,
        PDWORD);
    NTSTATUS (*QuerySecurity)(FSP_FILE_SYSTEM *, FSP_FSCTL_TRANSACT_REQ *,
        SECURITY_INFORMATION, PSECURITY_DESCRIPTOR, SIZE_T *);
} FSP_FILE_SYSTEM;

FSP_API NTSTATUS FspFileSystemCreate(PWSTR DevicePath,
    const FSP_FSCTL_VOLUME_PARAMS *VolumeParams,
    FSP_FILE_SYSTEM_DISPATCHER *Dispatcher,
    FSP_FILE_SYSTEM **PFileSystem);
FSP_API VOID FspFileSystemDelete(FSP_FILE_SYSTEM *FileSystem);
FSP_API NTSTATUS FspFileSystemLoop(FSP_FILE_SYSTEM *FileSystem);
FSP_API VOID FspFileSystemDirectDispatcher(FSP_FILE_SYSTEM *FileSystem,
    FSP_FSCTL_TRANSACT_REQ *Request);
FSP_API VOID FspFileSystemPoolDispatcher(FSP_FILE_SYSTEM *FileSystem,
    FSP_FSCTL_TRANSACT_REQ *Request);

static VOID FspFileSystemGetDispatcherResult(FSP_FILE_SYSTEM *FileSystem,
    NTSTATUS *PDispatcherResult)
{
    /* 32-bit reads are atomic */
    *PDispatcherResult = FileSystem->DispatcherResult;
    MemoryBarrier();
}
static VOID FspFileSystemSetDispatcherResult(FSP_FILE_SYSTEM *FileSystem,
    NTSTATUS DispatcherResult)
{
    if (NT_SUCCESS(DispatcherResult))
        return;
    InterlockedCompareExchange(&FileSystem->DispatcherResult, DispatcherResult, 0);
}

static VOID FspFileSystemEnterOperation(FSP_FILE_SYSTEM *FileSystem,
    FSP_FSCTL_TRANSACT_REQ *Request)
{
    if (0 != FileSystem->EnterOperation)
        FileSystem->EnterOperation(FileSystem, Request);
}
static VOID FspFileSystemLeaveOperation(FSP_FILE_SYSTEM *FileSystem,
    FSP_FSCTL_TRANSACT_REQ *Request)
{
    if (0 != FileSystem->LeaveOperation)
        FileSystem->LeaveOperation(FileSystem, Request);
}

FSP_API NTSTATUS FspSendResponse(FSP_FILE_SYSTEM *FileSystem,
    FSP_FSCTL_TRANSACT_RSP *Response);
FSP_API NTSTATUS FspSendResponseWithStatus(FSP_FILE_SYSTEM *FileSystem,
    FSP_FSCTL_TRANSACT_REQ *Request, NTSTATUS Result);

/*
 * Path Handling
 */
FSP_API VOID FspPathPrefix(PWSTR Path, PWSTR *PPrefix, PWSTR *PRemain);
FSP_API VOID FspPathSuffix(PWSTR Path, PWSTR *PRemain, PWSTR *PSuffix);
FSP_API VOID FspPathCombine(PWSTR Prefix, PWSTR Suffix);

/*
 * Utility
 */
FSP_API PGENERIC_MAPPING FspGetFileGenericMapping(VOID);
FSP_API NTSTATUS FspNtStatusFromWin32(DWORD Error);
FSP_API VOID FspDebugLog(const char *format, ...);

#endif
