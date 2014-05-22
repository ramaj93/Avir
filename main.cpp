#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"
#include <string.h>

HINSTANCE hInst;
HANDLE mhThread;
bool Stop = false;
bool pause = false;
char drivers[MAX_PATH];
char drive[25][10];

CRITICAL_SECTION cs;

int fillDrives(int len)
{
    int drv = 0;
    int index = 0;
    for(int i = 0;i<len;i++)
    {
        if(drivers[i]!='\0')drive[drv][index++] = drivers[i];
        else {
                char *d = drive[drv];
                drv++;
                index = 0;
        }
    }
    return drv;
}
DWORD WINAPI thread(LPVOID param)
{
    HWND lHwnd = (HWND)param;
    long long count = 0;
    char buf[MAX_PATH];
    int len = GetLogicalDriveStrings(MAX_PATH,drivers);
    /*Get Number of Drives and Root Paths*/
    int dlen = fillDrives(len);
    while(!Stop && (count < dlen) )
    {
        if(!pause)
        {

            SendMessage(lHwnd,WM_UPDATE_MSG,0,(LPARAM)drive[count%dlen]);
            //if(count == 2)break;
            if(!(count%10000))PostMessage(lHwnd,WM_STEP_PB,0,0);

            count ++;
        }
        else
        {
            Sleep(100);
        }
    }
    //if(Stop)PostMessage(lHwnd,WM_UPDATE_MSG,0,(LPARAM)("Stopped"));
    //else PostMessage(lHwnd,WM_UPDATE_MSG,0,(LPARAM)("Finished"));
    ExitThread(0);
}

BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        HICON t = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MYICON));
        SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)t);
        hInst = GetModuleHandle(NULL);
    }
    return TRUE;

    case WM_CLOSE:
    {
        CloseHandle(mhThread);

        EndDialog(hwndDlg, 0);
    }
    return TRUE;
    case WM_UPDATE_MSG:
    {
        SetWindowText(GetDlgItem(hwndDlg,IDMSG),(LPCSTR)lParam);
    }
    return true;
    case WM_STEP_PB:
    {
        SendMessage(GetDlgItem(hwndDlg,IDI_PROG),PBM_STEPIT,0,0);
    }
    return true;
    case WM_COMMAND:
    {
        switch(LOWORD(wParam))
        {
        case    IDSCAN:
            mhThread = CreateThread(NULL,0,thread,hwndDlg,0,0);
            //char Buf[MAX_PATH];
            //LoadString(hInst,FL_TEST,Buf,MAX_PATH);
            //MessageBox(hwndDlg,Buf,"StringListTest",MB_OK);
            EnableWindow(GetDlgItem(hwndDlg,IDPAUSE),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDCANCEL),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDREPAIR),TRUE);
            break;
        case IDPAUSE:
        {
            EnterCriticalSection(&cs);
            if(!Stop)
            {
                if(!pause)
                {
                    pause = true;
                    SetWindowText(GetDlgItem(hwndDlg,IDPAUSE),"CONTINUE");
                }
                else
                {
                    pause = false;
                    SetWindowText(GetDlgItem(hwndDlg,IDPAUSE),"PAUSE");
                }
            }
            LeaveCriticalSection(&cs);
        }
        break;
        case IDCANCEL:
        {
            int Ret = MessageBox(hwndDlg,"Do you want to exit!?","Confirm",MB_YESNO);
            if(Ret == IDYES)
            {
                EnterCriticalSection(&cs);
                Stop = true;
                EnableWindow(GetDlgItem(hwndDlg,IDPAUSE),FALSE);
                LeaveCriticalSection(&cs);
            }
        }
        break;
        case IDREPAIR:
            //MessageBox(NULL,"Pressed Repair","ButtonTest",MB_OK);
            SetWindowText(GetDlgItem(hwndDlg,IDMSG),"Label Test:clicked Repair");
            break;
        }
    }
    return TRUE;
    }
    return FALSE;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    hInst=hInstance;
    InitCommonControls();
    InitializeCriticalSection(&cs);
    return DialogBox(hInst, MAKEINTRESOURCE(DLG_MAIN), NULL, (DLGPROC)DlgMain);
}
