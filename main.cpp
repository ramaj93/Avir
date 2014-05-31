#include <stdio.h>
#include "resource.h"
#include <string.h>
#include <string>
#include <cstdio>
#include "aHelper.h"

CRITICAL_SECTION cs;

HINSTANCE hInst;
HANDLE mhThread,hRepair;

FILE *logFile = NULL;

bool Stop = false;
bool pause = false;
char drivers[MAX_PATH];
char drive[25][10];
long det = 0;
bool isScanDrive = false;
bool scanReady = false;

std::vector<int> vLevels;
std::string appDir;
vDef defs;
repStruct reps;
_options opt;

int fillDrives(int nBufSz,bool bDrv)
{
    int i = 0;
    if(bDrv)
    {
        while(drivers[i]!='\0'){
            drive[0][i] = drivers[i];
            i++;
        }
    }
    else {
        int drv = 0;
        int index = 0;
        for(int i = 0; i<nBufSz; i++)
        {
            if(drivers[i]!='\0')drive[drv][index++] = drivers[i];
            else
            {
                drv++;
                index = 0;
            }
        }
        return drv;
    }
}



DWORD WINAPI thread(LPVOID param)
{
    HWND lHwnd = (HWND)param;
    unsigned int count = 0;
    WIN32_FIND_DATA fd;
    HANDLE file = NULL;
    vHandle handles;
    vString roots;
    int dwRet = 0;
    std::string curFolder;
    char strPath[MAX_PATH] ;
    int flags = 0;
    int dlen;
    unsigned int nDirLevl = 0;
    unsigned int MAX_LEV = SendMessage(GetDlgItem(lHwnd,IDC_CMB),CB_GETCURSEL,0,0); //get the max levels
    //if level = 0 make it unlimited
    /*We always start at the drive root*/
    if(MAX_LEV == 0)MAX_LEV = 268435437;



    /*Get Number of Drives and Root Paths*/
    if(isScanDrive)
    {
        fillDrives(1,true);
    }
    else
    {
        int len = GetLogicalDriveStrings(MAX_PATH,drivers);
        dlen = fillDrives(len,false);
    }

    logFile = fopen("log.txt","a");
    GetCurrentDirectory(MAX_PATH,strPath);
    appDir = strPath;
    while(!Stop &&count <=dlen)
    {
        /*Set Current directory*/
        GetCurrentDirectory(MAX_PATH,strPath);
        int driveType = GetDriveType(drive[count]);
        DWORD type = GetDriveFormFactor(count);
        if((driveType == DRIVE_REMOVABLE)&&((type>=0)&&(type<350))||(driveType == DRIVE_FIXED))
        {
            SetCurrentDirectory(drive[count]);
            roots.push_back(drive[count]);
        }
        else
        {
            count++;
            continue;
        }

        //GetCurrentDirectory(MAX_PATH,e);
        if(file == NULL)file = FindFirstFile("*",&fd);

        while(!Stop)          //we shouldn't get out of this loop until we finish the drive
        {
            memset(strPath,0,MAX_PATH);
            memset(strPath,0,MAX_PATH);

            if(!pause)
            {
                Sleep(10);

                dwRet = FindNextFile(file,&fd);

                if(dwRet == 0 && nDirLevl == 0)break;

                if(dwRet > 0) //not the end of root
                {
                    if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
                    {
                        //Its a directory
                        //we'll only go to max level
                        if(!isScanDrive && strcmp(fd.cFileName,"Windows") == 0)
                        {
                            flags = 1;
                        }
                        if(flags == 0)
                        { //we are not in root
                            if(fd.dwFileAttributes&FILE_ATTRIBUTE_ARCHIVE)
                            {
                                if(fd.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) //if its hidden
                                {
                                    if(SetFileAttributes(fd.cFileName,fd.dwFileAttributes&~( FILE_ATTRIBUTE_READONLY|
                                                                                             FILE_ATTRIBUTE_HIDDEN|
                                                                                             FILE_ATTRIBUTE_SYSTEM)) == 0)
                                    {
                                        std::string tmp = fd.cFileName;
                                        ErrorOut(logFile,"Error setting Folder Attribute:"+tmp+":",1);
                                        continue;
                                    }
                                    //check names of suspected viruses
                                }
                            }
                            if(fd.dwFileAttributes&(FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_READONLY)) //if its system and ro
                            {
                                if(flags == 0){
                                    if(SetFileAttributes(fd.cFileName,fd.dwFileAttributes&~(FILE_ATTRIBUTE_HIDDEN|
                                                                                            FILE_ATTRIBUTE_SYSTEM))==0)
                                    {
                                        std::string tmp = fd.cFileName;
                                        ErrorOut(logFile,"Error setting Folder Attribute:"+tmp+":",1);
                                        continue;
                                    }
                                }
                            }
                        }//endof non sys op
                        else
                        {
                            //deal with root probs
                            //}
                        }
                        if(nDirLevl < MAX_LEV)
                        {
                            if(fd.cFileName[0] == '.')continue;
                            if(SetCurrentDirectory(fd.cFileName)>0){
                                handles.push_back(file);
                                curFolder = fd.cFileName;       //save the current dir
                                GetCurrentDirectory(MAX_PATH,strPath);
                                roots.push_back(strPath);
                                SendMessage(lHwnd,WM_UPDATE_MSG,0,(LPARAM)strPath);

                                file = FindFirstFile("*",&fd);              //get the new handle
                                vLevels.push_back(nDirLevl);                   //push the level
                                nDirLevl++;
                            }
                            else
                            {
                                //folder not openable
                                GetFullPathName(fd.cFileName,MAX_PATH,strPath,NULL);
                                SendMessage(lHwnd,WM_UPDATE_MSG,0,(LPARAM)strPath);
                                continue;
                            }
                        }
                        else
                        { //we av reached the highest lvl Just disp em.
                            GetFullPathName(fd.cFileName,MAX_PATH,strPath,NULL);
                            SendMessage(lHwnd,WM_UPDATE_MSG,0,(LPARAM)strPath);
                            continue;
                        }

                    }

                    else
                    {
                        //its a file
                        bool added = false;
                        if(fd.cFileName[0] == '.')continue; //jump the . & ..
                        GetFullPathName(fd.cFileName,MAX_PATH,strPath,NULL);
                        SendMessage(lHwnd,WM_UPDATE_MSG,0,(LPARAM)strPath);
                        for(int i = 0; i<defs.size(); i++)
                        {
                            for(int j = 0; j<defs[i].files.size(); j++)
                            {
                                if(defs[i].files[j][0] == '*')
                                {
                                    std::string tmp = fd.cFileName;     //eg *.lnk
                                    std::string gg = defs[i].files[j];
                                    if(tmp.find(defs[i].files[j].substr(2))!= std::string::npos)
                                    {
                                        addRec(strPath,i,j);
                                        fprintf(logFile,"Suspected File:%s\n",strPath);
                                        added = true;
                                    }
                                }
                                if(strncmp(fd.cFileName,defs[i].files[j].c_str(),strlen(fd.cFileName))== 0 && !added)
                                {
                                    //if its not system and not in sys root
                                    //try change attr
                                    //if succ or failed
                                    //delete file

                                    if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_SYSTEM)&&(flags==0))
                                    { //if not file system
                                        fprintf(logFile,"Suspected File:%s\n",strPath);
                                        addRec(strPath,i,j);
                                        added = true;
                                        if(fd.dwFileAttributes&(FILE_ATTRIBUTE_HIDDEN))
                                        {

                                        }
                                    }
                                    else
                                    {
                                        //do summat here
                                    }
                                }

                            }
                        }
                        if(curFolder.compare(fd.cFileName)==0 && !added)
                        {//suspected
                            if(opt&FIX_ALL)
                                addRec(strPath,-1,-1);
                            added = true;
                            fprintf(logFile,"File is suspected:%s\n",strPath);
                        }
                        if(added)PostMessage(lHwnd,WM_UPDATE_DET,0,0);
                        PostMessage(lHwnd,WM_STEP_PB,0,0);
                    }
                    memset(&fd,0,sizeof(fd));
                }
                else //rt is zero ff has failed
                {
                    //going back one level will return us to the root
                    if(nDirLevl > 0) //if its not drive root
                    {
                        if(roots.back().find("Windows",3)!=std::string::npos){
                            flags = 0; //we are leaving the root
                        }
                        roots.pop_back();
                        if(roots.size() > 0){
                            SetCurrentDirectory(roots.back().c_str());
                            curFolder = roots.back().c_str();
                        }
                        else
                        {
                            break;
                        }
                        FindClose(file);
                        file = handles.back();
                        handles.pop_back();
                        nDirLevl--;
                    }
                    else {
                        if(roots.back().compare("Windows") == 0){
                            flags = 0;
                        }
                        FindClose(file);
                        break; //exit loop we've hit the end of drive
                    }
                    memset(&fd,0,sizeof(fd));
                }
            }
            else
            {
                Sleep(100);
            }
        }
        dwRet = 1;
        file = NULL;
        roots.clear();
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


BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        HICON t = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MYICON));
        SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)t);
        for(int i = 0; i<4; i++)
        {
            char id[2];
            itoa(i,id,5);
            SendMessage(GetDlgItem(hwndDlg,IDC_CMB),CB_ADDSTRING,0,(LPARAM)id);

        }
        opt = UNKNOWN;
        SendMessage(GetDlgItem(hwndDlg,IDC_CMB),CB_SETCURSEL,(WPARAM)1,0);
        char tmp[8];
        itoa(nDefs,tmp,10);
        SetWindowText(GetDlgItem(hwndDlg,IDS_NDEF),tmp);
        hInst = GetModuleHandle(NULL);
        prepString();
    }
        return TRUE;

    case WM_CHK_DONE:
    {
        SendMessage(GetDlgItem(hwndDlg,IDI_PROG),PBM_SETPOS,(WPARAM)100,0);
        SetWindowText(GetDlgItem(hwndDlg,IDSCAN),"Open Log");
        SetWindowText(GetDlgItem(hwndDlg,IDCANCEL),"Exit");
        EnableWindow(GetDlgItem(hwndDlg,IDREPAIR),TRUE);
        scanReady = true;
    }
        return TRUE;

    case WM_CLOSE:
    {
        fflush(logFile);
        fclose(logFile);

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
    case WM_REP_DONE:
    {
        SendMessage(GetDlgItem(hwndDlg,IDI_PROG),PBM_SETPOS,(WPARAM)100,0);
        SetWindowText(GetDlgItem(hwndDlg,IDMSG),(LPSTR)"Done repairing.");
        fflush(logFile);
        fclose(logFile);
    }
        return true;
    case WM_COMMAND:
    {
        _options o = opt;
        switch(LOWORD(wParam))
        {
        case ID_CBSS:
        {
            if(SendMessage(GetDlgItem(hwndDlg,ID_CBSS),BM_GETCHECK,0,0) == BST_CHECKED)
            {
                opt = static_cast<_options>(opt&~(SCAN_DRIVE));
                opt = static_cast<_options>(opt|SCAN_SYSTEM);
                SendMessage(GetDlgItem(hwndDlg,ID_CBSD),BM_SETCHECK,(WPARAM)BST_UNCHECKED,0);
            }
            else
            {
                opt = static_cast<_options>(opt&~(SCAN_SYSTEM));
                opt = static_cast<_options>((int)opt|(int)SCAN_DRIVE);
                SendMessage(GetDlgItem(hwndDlg,ID_CBSD),BM_SETCHECK,(WPARAM)BST_CHECKED,0);
            }
        }
            return true;
        case ID_CBSD:
        {
            if(SendMessage(GetDlgItem(hwndDlg,ID_CBSD),BM_GETCHECK,0,0) == BST_CHECKED)
            {
                opt = static_cast<_options>((int)opt|(int)SCAN_DRIVE);
                o = opt;
                SendMessage(GetDlgItem(hwndDlg,ID_CBSS),BM_SETCHECK,(WPARAM)BST_UNCHECKED,0);
            }
            else
            {
                opt = static_cast<_options>((int)opt|(int)SCAN_SYSTEM);
                SendMessage(GetDlgItem(hwndDlg,ID_CBSS),BM_SETCHECK,(WPARAM)BST_CHECKED,0);
            }
        }
            return true;
        case  ID_CBFH:
        {
            o = opt;
            opt = static_cast<_options>((int)opt|(int)FIX_HIDDEN);
            o = opt;
        }
            break;
        case  ID_CBFA:
        {
            o = opt;
            opt = static_cast<_options>((int)opt|(int)FIX_ALL);
            o = opt;
        }
            break;
        case  IDCB_FR:
        {
            o = opt;
            opt = static_cast<_options>((int)opt|(int)FIX_REGISTRY);
            o = opt;
        }
            break;
        case    IDSCAN:
            if(scanReady)
            {
                const char *hy = appDir.c_str();
                int qw = SetCurrentDirectory(hy);
                system("start log.txt");
            }
            else
            {

                EnableWindow(GetDlgItem(hwndDlg,IDPAUSE),TRUE);
                EnableWindow(GetDlgItem(hwndDlg,IDCANCEL),TRUE);
                if(opt & SCAN_SYSTEM)
                {
                    mhThread = CreateThread(NULL,0,thread,hwndDlg,0,0);
                }
                else if(opt & SCAN_DRIVE)
                {
                    BROWSEINFO bi;
                    LPITEMIDLIST pidlPrograms;  // PIDL for Programs folder
                    LPITEMIDLIST pidlBrowse;    // PIDL selected by user


                    // Get the PIDL for the Programs folder.
                    if (!SUCCEEDED(SHGetSpecialFolderLocation(
                                       hwndDlg, CSIDL_DRIVES, &pidlPrograms))) {
                    }

                    // Fill in the BROWSEINFO structure.
                    bi.hwndOwner = hwndDlg;
                    bi.pidlRoot = pidlPrograms;
                    bi.pszDisplayName = drivers;

                    bi.lpszTitle = "Select a Drive to Scan:";
                    bi.ulFlags = 0;
                    bi.lpfn = NULL;
                    bi.lParam = 0;

                    // Browse for a folder and return its PIDL.
                    pidlBrowse = SHBrowseForFolder(&bi);
                    if (pidlBrowse != NULL) {

                        if (SHGetPathFromIDList(pidlBrowse,drivers))
                        {
                            isScanDrive = true;
                            //consider other options
                            mhThread = CreateThread(NULL,0,thread,hwndDlg,0,0);
                        }

                    }
                }
                else
                {
                    MessageBox(hwndDlg,"Select One Disk Option!","Error",MB_ICONERROR|MB_OK);
                }

            }
            // Clean up.
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
            if(!scanReady)
            {
                int Ret = MessageBox(hwndDlg,"Do you want to Stop Operation!?","Confirm",MB_YESNO);
                if(Ret == IDYES)
                {
                    EnterCriticalSection(&cs);
                    Stop = true;
                    EnableWindow(GetDlgItem(hwndDlg,IDPAUSE),FALSE);
                    LeaveCriticalSection(&cs);
                }
            }
            else
            {
                int Ret = MessageBox(hwndDlg,"Do you want to want to Exit App!?","Confirm",MB_YESNO);
                if(Ret == IDYES)
                {
                    ExitProcess(0);
                }
            }
        }
            break;
        case IDREPAIR:
            SendMessage(GetDlgItem(hwndDlg,IDI_PROG),PBM_SETPOS,(WPARAM)0,0);
            hRepair = CreateThread(NULL,0,runRepairs,hwndDlg,0,0);

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
