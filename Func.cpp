#include "Func.h"


HANDLE g_hDevice;
extern HINSTANCE g_hInstance; //对话框句柄


BOOL CALLBACK DialogMainProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			//加载图标
			HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
			//设置图标
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (long)hIcon);
			SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (long)hIcon);
			return TRUE;
		}
		case WM_CLOSE:
			EndDialog(hwndDlg, 0);
			return TRUE;

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_BUTTON_DRIVER: {
					//加载并启动驱动
					DriverProc((PTCHAR)DriverFullPath, hwndDlg);
					return TRUE;
				}
				case IDC_BUTTON_HOOK: {
					HookProc(hwndDlg);
					return TRUE;
				}
				case IDC_BUTTON_LOG: {
					// 读取调用记录
					UpdateApiCallRecord();
					return TRUE;
				}
				default:
					break;
			}
		}
	default:
		break;
	}
	return 0;
}

VOID DriverProc(PTCHAR lpszDriverFullPath, HWND hwndDlg) {
	//判断是加载驱动还是卸载驱动
	static BOOL flag = FALSE;
	//获取按钮句柄
	HWND hButton = GetDlgItem(hwndDlg, IDC_BUTTON_DRIVER);
	//1.使用OpenSCManager函数打开SCM
	//SCM句柄
	SC_HANDLE hSCM = NULL;
	SC_HANDLE hService = NULL;
	PTCHAR lpszDriverName = NULL;
	LPTSTR ErrorBuffer = NULL;
	DWORD dwRtn = NULL;
	SERVICE_STATUS SerStatus = { 0 };
	//size_t x = NULL;
	//char ch = NULL;
	do
	{
		if (!flag)
		{
			//获取驱动名
			//x = strlen(lpszDriverFullPath);
			//ch = '\\';
			//lpszDriverName = strrchr(lpszDriverFullPath, ch) + 1;
			//_splitpath(lpszDriverFullPath, NULL, NULL, lpszDriverName, NULL);

			lpszDriverName = (PTCHAR)DriverName;

			hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
			if (hSCM == NULL)
			{
				MessageBox(NULL, "Error!", "OpenSCManager fail!", MB_OK);
				break;
			}

			//2.使用CreateService函数利用SCM句柄创建一个服务
			//NT驱动服务句柄
			hService = CreateService(
				hSCM,					//句柄
				lpszDriverName,			//服务名称
				lpszDriverName,			//服务显示的名字
				SERVICE_ALL_ACCESS,		//所有权限打开
				SERVICE_KERNEL_DRIVER,	//内核服务类型
				SERVICE_DEMAND_START,	//服务启动类型，这里是手动启动
				SERVICE_ERROR_IGNORE,	//错误类型
				lpszDriverFullPath,     //sys文件所在磁盘目录
				NULL,					//服务所在的组，不关心顺序，传递NULL
				NULL,					//服务的Tag,这里不关心，传递NULL
				NULL, 					//服务的以依赖，这里没有依赖，传递NULL
				NULL,					//服务的启动用户名，传递NULL
				NULL					//服务启动用户名对应的密码，传递NULL
			);
			if (hService == NULL)
			{
				dwRtn = GetLastError();
				//ERROR_SERVICE_EXISTS表示服务已经存在，不能重复注册，属于正常情况
				if (dwRtn == ERROR_SERVICE_EXISTS)
				{
					hService = OpenService(hSCM, lpszDriverName, SERVICE_ALL_ACCESS);
					if (hService == NULL)
					{
						MessageBox(NULL, "Service open failure", "Error!", MB_OK);
						break;
					}
				}
				else
				{
					sprintf(ErrorBuffer, "CreateService failure! ErrorCode:%d", dwRtn);
					MessageBox(NULL, ErrorBuffer, "Error!", MB_OK);
					break;
				}
			}

			MessageBox(0, "CreateService or OpenService success!", 0, 0);
			//3. 使用StartService函数启动我们刚刚创建的服务

			if (!StartService(hService, NULL, NULL))
			{
				dwRtn = GetLastError();
				sprintf(ErrorBuffer, "StartService fail! ErrorCode:%d", dwRtn);
				MessageBox(NULL, ErrorBuffer, "Error!", MB_OK);
				break;
			}
			MessageBox(0, "StartService Success!", 0, 0);
			//驱动已加载
			flag = TRUE;
			SetWindowText(hButton, "卸载驱动");
		}
		else
		{
			hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
			if (hSCM == NULL)
			{
				MessageBox(NULL, "Error!", "OpenSCManager fail!", MB_OK);
				break;
			}
			hService = OpenService(hSCM, (PTCHAR)DriverName, SERVICE_ALL_ACCESS);
			if (hService == NULL)
			{
				MessageBox(NULL, "Service open failure", "Error!", MB_OK);
				break;
			}

			//停止服务
			if (!ControlService(hService, SERVICE_CONTROL_STOP, &SerStatus))
			{
				MessageBox(NULL, "Service stop failure", "Error!", MB_OK);
				break;
			}
			//删除服务
			if (!DeleteService(hService))
			{
				MessageBox(NULL, "Service detele failure", "Error!", MB_OK);
				break;
			}
			MessageBox(0, "Detele Service Success!", 0, 0);
			//驱动已卸载
			flag = FALSE;
			SetWindowText(hButton, "加载驱动");
		}
	} while (FALSE);
	if (hSCM != NULL)
	{
		CloseServiceHandle(hSCM);
		hSCM = NULL;
	}
	if (hService != NULL)
	{
		CloseServiceHandle(hService);
		hService = NULL;
	}
}


VOID HookProc(HWND hwndDlg)
{
	
	//向user32.dll注入ShellCode，对MessageBoxA 进行INLINE HOOK

	//修改MessageBoxA函数开头为   int 0x20  中断门

	TCHAR szOutBuffer[OUT_BUFFER_MAXLENGTH] = { 0 };
	TCHAR szInBuffer[OUT_BUFFER_MAXLENGTH] = { 0 };
	TCHAR szBuffer[30] = { 0 };
	DWORD err = NULL;
	DWORD dw = NULL;
	BOOL Stutas = NULL;
	static BOOL flag = FALSE;
	//获取按钮句柄
	HWND hButton = GetDlgItem(hwndDlg, IDC_BUTTON_HOOK);
	do
	{
		if (!flag)
		{
			// 通过符号链接，打开设备
		//在3环获取驱动程序
			g_hDevice = CreateFile(SYMBOLICLINK_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			err = GetLastError();
			if (err)
			{
				sprintf(szBuffer, "Error Code: %d", err);
				MessageBox(0, szBuffer, "Error!", 0);
			}
			if (g_hDevice == INVALID_HANDLE_VALUE)
			{

				MessageBox(0, "INVALID_HANDLE_VALUE", "Error!", 0);
				break;
			}
			// 过写拷贝
			MessageBox(0, "过写拷贝", 0, 0);
			BypassApiWriteCopy();
			
			HANDLE hDevice = CreateFile(SYMBOLICLINK_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			TCHAR ErroCode[20] = { 0 };
			sprintf(ErroCode, "[Fuc->HookUser32Api]ErroCode:%d", GetLastError());
			MessageBox(0, ErroCode, 0, 0);
			sprintf(ErroCode, "[Fuc->HookUser32Api]hDevice:%X", (DWORD)hDevice);
			MessageBox(0, ErroCode, 0, 0);
			if (hDevice == INVALID_HANDLE_VALUE)
			{
				MessageBox(0, "打开设备失败", 0, 0);
				break;
			}
			USHORT IntGateNum; // 中断号
			DWORD dwRetBytes; // 返回的字节数

			DeviceIoControl(hDevice, OPER_HOOK, NULL, 0, &IntGateNum, sizeof(USHORT), &dwRetBytes, NULL);

			if (dwRetBytes != 2 || IntGateNum == 0)
			{
				MessageBox(0, "构造中断门失败", 0, 0);
				break;
			}
			// HOOK MessageBoxA
			USHORT IntInstructions = (IntGateNum << 8);
			IntInstructions |= (USHORT)0x00CD;
			*(PUSHORT)MessageBoxA = IntInstructions;
			CloseHandle(hDevice);
			hDevice = NULL;
			MessageBox(0, "设置中断门Hook成功!", 0, 0);

			flag = TRUE;
			SetWindowText(hButton, "关闭全局监控");
		}
		else
		{
			// 取消HOOK
			((PUSHORT)MessageBoxA)[0] = 0xff8b;

			flag = FALSE;
			SetWindowText(hButton, "开启全局监控");
		}
		
		
	} while (FALSE);

	
	if (g_hDevice != NULL)
	{
		//关闭句柄
		CloseHandle(g_hDevice);
		g_hDevice = NULL;
	}
}


// 获取PDE
DWORD* GetPDE(DWORD addr)
{
	return (DWORD*)(0xc0600000 + ((addr >> 18) & 0x3ff8));
}

// 获取PTE
DWORD* GetPTE(DWORD addr)
{
	return (DWORD*)(0xc0000000 + ((addr >> 9) & 0x7ffff8));
}

// 构建调用门（提权、有参）
USHORT CreateCallGate(DWORD pBaseAddress, DWORD nParam)
{
	HANDLE hDevice = CreateFile(SYMBOLICLINK_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		MessageBox(0, "[Function]CreateCallGate: INVALID_HANDLE_VALUE", 0, 0);
		return 0;
	}
	USHORT CallGateDescriptor = NULL;	// 调用门选择子
	DWORD dwRetBytes;			// 返回的字节数
	DWORD InBuffer[2];
	InBuffer[0] = pBaseAddress;
	InBuffer[1] = nParam;
	DeviceIoControl(hDevice, OPER_CALL_GATE_R0, InBuffer, 8, &CallGateDescriptor, sizeof(USHORT), &dwRetBytes, NULL);
	if (dwRetBytes != 2 || CallGateDescriptor == NULL)
	{
		MessageBox(0, "构造调用门失败", 0, 0);
		return 0;
	}
	CloseHandle(hDevice);
	hDevice = NULL;
	return CallGateDescriptor;
}

// 以0环权限调用某个裸函数，支持传参
BOOL CallInRing0(PVOID pFuncion, PDWORD pParam, DWORD nParam)
{
	// 命令驱动构建调用门
	USHORT CallGateDescriptor = CreateCallGate((DWORD)pFuncion, nParam);
	if (CallGateDescriptor == 0)
	{
		MessageBox(0, "构建调用门失败！", 0, 0);
		return FALSE;
	}
	MessageBox(0, "构建调用门成功！", 0, 0);
	// 构造调用门描述符
	USHORT buff[3] = { 0 };
	buff[2] = CallGateDescriptor;
	// 参数压栈
	if (nParam && pParam)
	{
		for (DWORD i = 0; i < nParam; i++)
		{
			__asm
			{
				mov eax, pParam;
				push [eax];
			}
			pParam++;
		}
	}
	// 调用门调用
	TCHAR b[20] = { 0 };
	sprintf(b, "调用门描述符：%#X", buff[2]);
	MessageBox(0, b, 0, 0);
	__asm call fword ptr[buff]; // 长调用，使用调用门提权
	return TRUE;
}

// API函数过写拷贝，其实就是将函数线性地址的PDE，PTE改成可写
// 参数0：要过写拷贝的函数地址
// 参数1：PDE线性地址
// 参数2：PTE线性地址
void __declspec(naked) BypassApiWriteCopyNaked()
{
	__asm
	{
		pushad;
		pushfd;
	}
	__asm
	{
		// R/W = 1, U/S = 1
		mov eax, [esp + 0x24 + 0x8 + 0x0];	// 参数2，PTE的地址
		or dword ptr[eax], 0x00000006;		//0110
		mov eax, [esp + 0x24 + 0x8 + 0x4];	// 参数1，PDE的地址
		or dword ptr[eax], 0x00000006;
		mov eax, [esp + 0x24 + 0x8 + 0x8];	// 参数0，要过写拷贝的函数地址

		invlpg [eax]; // 清除TLB缓存
	}
	__asm
	{
		popfd;
		popad;
		retf 0xC;
	}
}

// 过写拷贝
void BypassApiWriteCopy()
{
	// MessageBoxA 挂物理页，不这样操作，MessageBoxA的PTE可能是无效的
	__asm
	{
		mov eax, dword ptr ds : [MessageBoxA] ;
		mov eax, [eax];
	}
	// MessageBoxA过写拷贝	
	DWORD pParam[3];
	pParam[0] = (DWORD)MessageBoxA;
	pParam[1] = (DWORD)GetPDE(pParam[0]);
	pParam[2] = (DWORD)GetPTE(pParam[0]);
	MessageBox(0, "传参", 0, 0);
	CallInRing0(BypassApiWriteCopyNaked, pParam, 3);
}

// 从驱动获取调用记录
void UpdateApiCallRecord()
{
	HANDLE hDevice = CreateFile(SYMBOLICLINK_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		MessageBox(0, "打开设备失败", 0, 0);
		return;
	}
	MessageBox(0, "打开设备成功", 0, 0);
	APICALLRECORD ApiCallRecord;
	DWORD dwRetBytes; // 返回的字节数
	while (1)
	{
		Sleep(50);
		MessageBox(0, "R3向R0发送命令", 0, 0);
		DeviceIoControl(hDevice, OPER_GET_APICALLRECORD, NULL, 0, &ApiCallRecord, sizeof(ApiCallRecord), &dwRetBytes, NULL);
		if (dwRetBytes == 0)
		{
			//MessageBox(0, "无API调用记录", 0, 0);
			continue;
		}
		if (ApiCallRecord.pApiAddress == (DWORD)MessageBoxA)
		{
			PTCHAR Buffer = (PTCHAR)VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
			memset(Buffer, 0, 0x1000);
			sprintf(
				Buffer,
				"MessageBoxA(%x, %x, %x, %x)",
				ApiCallRecord.Param[0], ApiCallRecord.Param[1], ApiCallRecord.Param[2], ApiCallRecord.Param[3]);
			MessageBox(0, Buffer, 0, 0);
		}
	}
	CloseHandle(hDevice);
}

// HOOK MessageBoxA
// 理论上可以 HOOK User32.dll 里的任意函数
BOOL HookUser32Api()
{
	HANDLE hDevice = CreateFile(SYMBOLICLINK_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	TCHAR ErroCode[20] = { 0 };
	sprintf(ErroCode, "[Fuc->HookUser32Api]ErroCode:%d", GetLastError());
	MessageBox(0, ErroCode, 0, 0);
	sprintf(ErroCode, "[Fuc->HookUser32Api]hDevice:%X", (DWORD)hDevice);
	MessageBox(0, ErroCode, 0, 0);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		MessageBox(0, "打开设备失败", 0, 0);
		return FALSE;
	}
	USHORT IntGateNum; // 中断号
	DWORD dwRetBytes; // 返回的字节数

	DeviceIoControl(hDevice, OPER_HOOK, NULL, 0, &IntGateNum, sizeof(USHORT), &dwRetBytes, NULL);
	__asm int 3;
	if (dwRetBytes != 2 || IntGateNum == 0)
	{
		MessageBox(0, "构造中断门失败", 0, 0);
		return FALSE;
	}
	// HOOK MessageBoxA
	USHORT IntInstructions = (IntGateNum << 8);
	IntInstructions |= (USHORT)0x00CD;
	*(PUSHORT)MessageBoxA = IntInstructions;
	CloseHandle(hDevice);
	hDevice = NULL;
	return TRUE;
}