#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"
#include <string.h>
#include <vector>
#include <string>
#include <cstdio>

HINSTANCE hInst;
HANDLE mhThread;
bool Stop = false;
bool pause = false;
char drivers[MAX_PATH];
char drive[25][10];
long det = 0;
FILE *logFile = NULL;
const unsigned int nDefs = IDS_LAST - IDS_DECT;
CRITICAL_SECTION cs;

typedef std::vector<HANDLE> vHandle;
typedef std::vector<std::string> vString;
std::vector<int> vLevels;

struct def
{
    vString files;
    vString regs;
};

typedef std::vector<def> vDef;

vDef defs;
vString roots;

void prepString();
int fillDrives(int len)
{
    int drv = 0;
    int index = 0;
    for(int i = 0; i<len; i++)
    {
        if(drivers[i]!='\0')drive[drv][index++] = drivers[i];
        else
        {
            char *d = drive[drv];
            drv++;
            index = 0;
        }
    }
    return drv;
}

const unsigned int MAX_LEV = 2;

DWORD WINAPI thread(LPVOID param)
{
    HWND lHwnd = (HWND)param;
    int count = 0;
    char buf[MAX_PATH];
    int len = GetLogicalDriveStrings(MAX_PATH,drivers);
    WIN32_FIND_DATA fd;
    HANDLE file = NULL;
    vHandle handles;

    /*We always start at the drive root*/
    int level = 0;
    /*Get Number of Drives and Root Paths*/

    int dlen = fillDrives(len);
    int rt = 1;
    std::string curFolder;

    logFile = fopen("log.txt","a");

    while(!Stop &&count <=dlen)
    {
        /*Set Current directory*/
        SetCurrentDirectory(drive[count]);
        //roots.push_back(drive[count]); //pushed the root
        if(file == NULL)file = FindFirstFile("*",&fd);
        //if file erro deal
        char e[MAX_PATH] ;

        while(!Stop)          //we shouldnt get out of this loop until we finish the drive
        {
            if(!pause)
            {
                Sleep(10);
                rt = FindNextFile(file,&fd);

                GetCurrentDirectory(MAX_PATH,e);
                char *hy = fd.cFileName;
                if(rt > 0) //not the end of root
                {
                    if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
                    {
                        //Its a directory
                        //we'll only go to max level
                        if(level < MAX_LEV)
                        {
                            if(fd.cFileName[0] == '.')continue;
                            handles.push_back(file);
                            GetCurrentDirectory(MAX_PATH,e);
                            roots.push_back(e);
                            char *fld = fd.cFileName;
                            SetCurrentDirectory(fd.cFileName);
                            curFolder = fd.cFileName;       //save the current dir
                            //roots.push_back(fd.cFileName);  //push the new root
                            file = FindFirstFile("*",&fd);  //get the new handle
                            vLevels.push_back(level);       //push the level
                            level++;
                        }
                        else continue;   //goto lower level
                    }
                    else
                    {
                        //its a file
                        if(fd.cFileName[0] == '.')continue; //jump the . & ..
                        char path[MAX_PATH];
                        GetFullPathName(fd.cFileName,MAX_PATH,path,NULL);
                        SendMessage(lHwnd,WM_UPDATE_MSG,0,(LPARAM)path);
                        int id = defs.size();
                        for(int i = 0; i<defs.size(); i++)
                        {
                            for(int j = 0; j<defs[i].files.size(); j++)
                            {
                                if(strcmp(fd.cFileName,defs[i].files[j].c_str()) == 0)
                                {
                                    PostMessage(lHwnd,WM_UPDATE_DET,0,0);
                                    fprintf(logFile,"%s\n",fd.cFileName);
                                }

                            }
                        }
                        if(curFolder.compare(fd.cFileName)==0)
                        {
                            PostMessage(lHwnd,WM_UPDATE_DET,0,0);
                            fprintf(logFile,"%s\n",fd.cFileName);
                        }
                        PostMessage(lHwnd,WM_STEP_PB,0,0);
                    }
                    memset(&fd,0,sizeof(fd));
                }
                else //rt is zero ff has failed
                {
                    //going back one level will return us to the root
                    if(level > 0) //if its not drive root
                    {
                        char *str = (char*)roots.back().c_str();
                        //roots.pop_back();                       //pop this root to the previous one
                        str = (char*)roots.back().c_str();
                        GetCurrentDirectory(MAX_PATH,e);
                        SetCurrentDirectory(str);
                        GetCurrentDirectory(MAX_PATH,e);
                        roots.pop_back();
                        file = handles.back();
                        handles.pop_back();
                        level--;
                        vLevels.pop_back();
                        //rt = FindNextFile(file,&fd); //get the ff of the prev handle
                    }
                    else break; //exit loop weve hit the end of drive
                    memset(&fd,0,sizeof(fd));
                }
            }
            else
            {
                Sleep(100);
            }
        }
        rt = 1;
        file = NULL;
        handles.clear();
        count ++;
    }
    if(Stop)PostMessage(lHwnd,WM_UPDATE_MSG,0,(LPARAM)("Stopped"));
    else PostMessage(lHwnd,WM_UPDATE_MSG,0,(LPARAM)("Finished"));
    SendMessage(lHwnd,WM_CHK_DONE,0,0);
    fflush(logFile);
    fclose(logFile);
    ExitThread(0);
}

void prepString()
{
    def tmp;
    char *strBuf;
    int flag = 0;
    for(int i = 1; i<nDefs; i++)
    {
        char Buf[512] = {0};
        LoadString(hInst,IDS_DECT+i,Buf,512);
        strBuf = strtok(Buf,"~");
        tmp.files.push_back(strBuf);
        while((strBuf = strtok(NULL,"~"))!= NULL)
        {
            if(strBuf[0] == 'r')
            {
                //its a reg string
                flag = 1;
            }
            else
            {
                if(flag == 1)
                {
                    tmp.regs.push_back(strBuf);
                    flag = 0;
                }
                else tmp.files.push_back(strBuf);
            }
        }
        defs.push_back(tmp);
        tmp.files.clear();
        tmp.regs.clear();
    }
}
BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        HICON t = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MYICON));
        SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)t);
        char tmp[8];
        itoa(nDefs,tmp,10);
        SetWindowText(GetDlgItem(hwndDlg,IDS_NDEF),tmp);
        hInst = GetModuleHandle(NULL);
        prepString();
        free(tmp);
    }
    return TRUE;

    case WM_CHK_DONE:
    {
        SendMessage(GetDlgItem(hwndDlg,IDI_PROG),PBM_SETPOS,(WPARAM)100,0);
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
    case WM_UPDATE_DET:
    {
        ++det;
        char Buf[10];
        itoa(det,Buf,10);
        SetWindowText(GetDlgItem(hwndDlg,IDS_DECT),Buf);
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
