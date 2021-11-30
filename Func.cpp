#include "Func.h"


HANDLE g_hDevice;
extern HINSTANCE g_hInstance; //�Ի�����


BOOL CALLBACK DialogMainProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			//����ͼ��
			HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
			//����ͼ��
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
					//���ز���������
					DriverProc((PTCHAR)DriverFullPath, hwndDlg);
					return TRUE;
				}
				case IDC_BUTTON_HOOK: {
					HookProc(hwndDlg);
					return TRUE;
				}
				case IDC_BUTTON_LOG: {
					// ��ȡ���ü�¼
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
	//�ж��Ǽ�����������ж������
	static BOOL flag = FALSE;
	//��ȡ��ť���
	HWND hButton = GetDlgItem(hwndDlg, IDC_BUTTON_DRIVER);
	//1.ʹ��OpenSCManager������SCM
	//SCM���
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
			//��ȡ������
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

			//2.ʹ��CreateService��������SCM�������һ������
			//NT����������
			hService = CreateService(
				hSCM,					//���
				lpszDriverName,			//��������
				lpszDriverName,			//������ʾ������
				SERVICE_ALL_ACCESS,		//����Ȩ�޴�
				SERVICE_KERNEL_DRIVER,	//�ں˷�������
				SERVICE_DEMAND_START,	//�����������ͣ��������ֶ�����
				SERVICE_ERROR_IGNORE,	//��������
				lpszDriverFullPath,     //sys�ļ����ڴ���Ŀ¼
				NULL,					//�������ڵ��飬������˳�򣬴���NULL
				NULL,					//�����Tag,���ﲻ���ģ�����NULL
				NULL, 					//�����������������û������������NULL
				NULL,					//����������û���������NULL
				NULL					//���������û�����Ӧ�����룬����NULL
			);
			if (hService == NULL)
			{
				dwRtn = GetLastError();
				//ERROR_SERVICE_EXISTS��ʾ�����Ѿ����ڣ������ظ�ע�ᣬ�����������
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
			//3. ʹ��StartService�����������Ǹոմ����ķ���

			if (!StartService(hService, NULL, NULL))
			{
				dwRtn = GetLastError();
				sprintf(ErrorBuffer, "StartService fail! ErrorCode:%d", dwRtn);
				MessageBox(NULL, ErrorBuffer, "Error!", MB_OK);
				break;
			}
			MessageBox(0, "StartService Success!", 0, 0);
			//�����Ѽ���
			flag = TRUE;
			SetWindowText(hButton, "ж������");
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

			//ֹͣ����
			if (!ControlService(hService, SERVICE_CONTROL_STOP, &SerStatus))
			{
				MessageBox(NULL, "Service stop failure", "Error!", MB_OK);
				break;
			}
			//ɾ������
			if (!DeleteService(hService))
			{
				MessageBox(NULL, "Service detele failure", "Error!", MB_OK);
				break;
			}
			MessageBox(0, "Detele Service Success!", 0, 0);
			//������ж��
			flag = FALSE;
			SetWindowText(hButton, "��������");
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
	
	//��user32.dllע��ShellCode����MessageBoxA ����INLINE HOOK

	//�޸�MessageBoxA������ͷΪ   int 0x20  �ж���

	TCHAR szOutBuffer[OUT_BUFFER_MAXLENGTH] = { 0 };
	TCHAR szInBuffer[OUT_BUFFER_MAXLENGTH] = { 0 };
	TCHAR szBuffer[30] = { 0 };
	DWORD err = NULL;
	DWORD dw = NULL;
	BOOL Stutas = NULL;
	static BOOL flag = FALSE;
	//��ȡ��ť���
	HWND hButton = GetDlgItem(hwndDlg, IDC_BUTTON_HOOK);
	do
	{
		if (!flag)
		{
			// ͨ���������ӣ����豸
		//��3����ȡ��������
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
			// ��д����
			MessageBox(0, "��д����", 0, 0);
			BypassApiWriteCopy();
			
			HANDLE hDevice = CreateFile(SYMBOLICLINK_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			TCHAR ErroCode[20] = { 0 };
			sprintf(ErroCode, "[Fuc->HookUser32Api]ErroCode:%d", GetLastError());
			MessageBox(0, ErroCode, 0, 0);
			sprintf(ErroCode, "[Fuc->HookUser32Api]hDevice:%X", (DWORD)hDevice);
			MessageBox(0, ErroCode, 0, 0);
			if (hDevice == INVALID_HANDLE_VALUE)
			{
				MessageBox(0, "���豸ʧ��", 0, 0);
				break;
			}
			USHORT IntGateNum; // �жϺ�
			DWORD dwRetBytes; // ���ص��ֽ���

			DeviceIoControl(hDevice, OPER_HOOK, NULL, 0, &IntGateNum, sizeof(USHORT), &dwRetBytes, NULL);

			if (dwRetBytes != 2 || IntGateNum == 0)
			{
				MessageBox(0, "�����ж���ʧ��", 0, 0);
				break;
			}
			// HOOK MessageBoxA
			USHORT IntInstructions = (IntGateNum << 8);
			IntInstructions |= (USHORT)0x00CD;
			*(PUSHORT)MessageBoxA = IntInstructions;
			CloseHandle(hDevice);
			hDevice = NULL;
			MessageBox(0, "�����ж���Hook�ɹ�!", 0, 0);

			flag = TRUE;
			SetWindowText(hButton, "�ر�ȫ�ּ��");
		}
		else
		{
			// ȡ��HOOK
			((PUSHORT)MessageBoxA)[0] = 0xff8b;

			flag = FALSE;
			SetWindowText(hButton, "����ȫ�ּ��");
		}
		
		
	} while (FALSE);

	
	if (g_hDevice != NULL)
	{
		//�رվ��
		CloseHandle(g_hDevice);
		g_hDevice = NULL;
	}
}


// ��ȡPDE
DWORD* GetPDE(DWORD addr)
{
	return (DWORD*)(0xc0600000 + ((addr >> 18) & 0x3ff8));
}

// ��ȡPTE
DWORD* GetPTE(DWORD addr)
{
	return (DWORD*)(0xc0000000 + ((addr >> 9) & 0x7ffff8));
}

// ���������ţ���Ȩ���вΣ�
USHORT CreateCallGate(DWORD pBaseAddress, DWORD nParam)
{
	HANDLE hDevice = CreateFile(SYMBOLICLINK_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		MessageBox(0, "[Function]CreateCallGate: INVALID_HANDLE_VALUE", 0, 0);
		return 0;
	}
	USHORT CallGateDescriptor = NULL;	// ������ѡ����
	DWORD dwRetBytes;			// ���ص��ֽ���
	DWORD InBuffer[2];
	InBuffer[0] = pBaseAddress;
	InBuffer[1] = nParam;
	DeviceIoControl(hDevice, OPER_CALL_GATE_R0, InBuffer, 8, &CallGateDescriptor, sizeof(USHORT), &dwRetBytes, NULL);
	if (dwRetBytes != 2 || CallGateDescriptor == NULL)
	{
		MessageBox(0, "���������ʧ��", 0, 0);
		return 0;
	}
	CloseHandle(hDevice);
	hDevice = NULL;
	return CallGateDescriptor;
}

// ��0��Ȩ�޵���ĳ���㺯����֧�ִ���
BOOL CallInRing0(PVOID pFuncion, PDWORD pParam, DWORD nParam)
{
	// ������������������
	USHORT CallGateDescriptor = CreateCallGate((DWORD)pFuncion, nParam);
	if (CallGateDescriptor == 0)
	{
		MessageBox(0, "����������ʧ�ܣ�", 0, 0);
		return FALSE;
	}
	MessageBox(0, "���������ųɹ���", 0, 0);
	// ���������������
	USHORT buff[3] = { 0 };
	buff[2] = CallGateDescriptor;
	// ����ѹջ
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
	// �����ŵ���
	TCHAR b[20] = { 0 };
	sprintf(b, "��������������%#X", buff[2]);
	MessageBox(0, b, 0, 0);
	__asm call fword ptr[buff]; // �����ã�ʹ�õ�������Ȩ
	return TRUE;
}

// API������д��������ʵ���ǽ��������Ե�ַ��PDE��PTE�ĳɿ�д
// ����0��Ҫ��д�����ĺ�����ַ
// ����1��PDE���Ե�ַ
// ����2��PTE���Ե�ַ
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
		mov eax, [esp + 0x24 + 0x8 + 0x0];	// ����2��PTE�ĵ�ַ
		or dword ptr[eax], 0x00000006;		//0110
		mov eax, [esp + 0x24 + 0x8 + 0x4];	// ����1��PDE�ĵ�ַ
		or dword ptr[eax], 0x00000006;
		mov eax, [esp + 0x24 + 0x8 + 0x8];	// ����0��Ҫ��д�����ĺ�����ַ

		invlpg [eax]; // ���TLB����
	}
	__asm
	{
		popfd;
		popad;
		retf 0xC;
	}
}

// ��д����
void BypassApiWriteCopy()
{
	// MessageBoxA ������ҳ��������������MessageBoxA��PTE��������Ч��
	__asm
	{
		mov eax, dword ptr ds : [MessageBoxA] ;
		mov eax, [eax];
	}
	// MessageBoxA��д����	
	DWORD pParam[3];
	pParam[0] = (DWORD)MessageBoxA;
	pParam[1] = (DWORD)GetPDE(pParam[0]);
	pParam[2] = (DWORD)GetPTE(pParam[0]);
	MessageBox(0, "����", 0, 0);
	CallInRing0(BypassApiWriteCopyNaked, pParam, 3);
}

// ��������ȡ���ü�¼
void UpdateApiCallRecord()
{
	HANDLE hDevice = CreateFile(SYMBOLICLINK_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		MessageBox(0, "���豸ʧ��", 0, 0);
		return;
	}
	MessageBox(0, "���豸�ɹ�", 0, 0);
	APICALLRECORD ApiCallRecord;
	DWORD dwRetBytes; // ���ص��ֽ���
	while (1)
	{
		Sleep(50);
		MessageBox(0, "R3��R0��������", 0, 0);
		DeviceIoControl(hDevice, OPER_GET_APICALLRECORD, NULL, 0, &ApiCallRecord, sizeof(ApiCallRecord), &dwRetBytes, NULL);
		if (dwRetBytes == 0)
		{
			//MessageBox(0, "��API���ü�¼", 0, 0);
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
// �����Ͽ��� HOOK User32.dll ������⺯��
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
		MessageBox(0, "���豸ʧ��", 0, 0);
		return FALSE;
	}
	USHORT IntGateNum; // �жϺ�
	DWORD dwRetBytes; // ���ص��ֽ���

	DeviceIoControl(hDevice, OPER_HOOK, NULL, 0, &IntGateNum, sizeof(USHORT), &dwRetBytes, NULL);
	__asm int 3;
	if (dwRetBytes != 2 || IntGateNum == 0)
	{
		MessageBox(0, "�����ж���ʧ��", 0, 0);
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