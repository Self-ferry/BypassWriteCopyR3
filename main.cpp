#include"Func.h"

/*
在ring3：
注册和卸载驱动
找到User32，定位Messagebox函数地址，传给0环

编写shellcode注入进user32，对Messagebox进行INLINE HOOK

打印使用Messagebox的程序
*/

HINSTANCE g_hInstance; //对话框句柄

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