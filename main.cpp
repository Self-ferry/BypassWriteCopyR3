#include"Func.h"

/*
��ring3��
ע���ж������
�ҵ�User32����λMessagebox������ַ������0��

��дshellcodeע���user32����Messagebox����INLINE HOOK

��ӡʹ��Messagebox�ĳ���
*/

HINSTANCE g_hInstance; //�Ի�����

int APIENTRY WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd
) 
{
	g_hInstance = hInstance;
	DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, DialogMainProc);
}