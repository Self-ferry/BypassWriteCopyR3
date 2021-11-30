#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include"resource.h"

//#define DriverFullPath __T("F:\\Text_Files\\Driver\\BypassWriteCopyR3\\Release\\BypassWriteCopyR0.sys")
#define DriverFullPath "C:\\BypassWriteCopyR0.sys"
#define DriverName "MyDriver"

//输入缓存最大长度
#define IN_BUFFER_MAXLENGTH  0x10
//输出缓存最大长度
#define OUT_BUFFER_MAXLENGTH 0x10

#define OPER_CALL_GATE_R0 CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define OPER_HOOK CTL_CODE(FILE_DEVICE_UNKNOWN,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define OPER_GET_APICALLRECORD CTL_CODE(FILE_DEVICE_UNKNOWN,0x802,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define SYMBOLICLINK_NAME "\\\\.\\MyTestDriver"


/// <summary>
/// 驱动的加载和卸载
/// </summary>
/// <param name="lpszDriverFullPath">驱动文件的路径</param>
/// <param name="hwndDlg">主对话框句柄</param>
VOID DriverProc(PTCHAR lpszDriverFullPath, HWND hwndDlg);

DWORD* GetPDE(DWORD addr);
DWORD* GetPTE(DWORD addr);
USHORT CreateCallGate(DWORD pBaseAddress, DWORD nParam);
BOOL CallInRing0(PVOID pFuncion, PDWORD pParam, DWORD nParam);
void BypassApiWriteCopyNaked();
void BypassApiWriteCopy();
BOOL HookUser32Api();
void UpdateApiCallRecord();

VOID HookProc(HWND hwndDlg);


//主对话框消息处理函数
BOOL CALLBACK DialogMainProc(
	HWND hwndDlg,  // handle to dialog box
	UINT uMsg,     // message
	WPARAM wParam, // first message parameter
	LPARAM lParam  // second message parameter
);


// API调用记录父类
typedef struct _APICALLRECORD
{
	LIST_ENTRY ApiCallRecordList; // 链表
	UINT32 pApiAddress; // API函数地址
	UINT32 nParam; // 参数个数
	UINT32 Param[32]; // 参数列表
} APICALLRECORD, * PAPICALLRECORD;