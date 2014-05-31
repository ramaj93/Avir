#include "aHelper.h"


extern vDef defs;
extern  repStruct reps;
extern _options opt;
static FILE * lgFile;


#ifndef VWIN32_DIOC_DOS_IOCTL
#define VWIN32_DIOC_DOS_IOCTL 1
// Intel x86 processor status fla
#define CARRY_FLAG  0x1

typedef struct _DIOC_REGISTERS
{
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DIOC_REGISTERS, *PDIOC_REGISTERS;

#endif





#pragma pack(1)
typedef struct _DOSDPB
{
    BYTE    specialFunc;    //
    BYTE    devType;        //
    WORD    devAttr;        //
    WORD    cCyl;           // number of cylinders
    BYTE    mediaType;      //
    WORD    cbSec;          // Bytes per sector
    BYTE    secPerClus;     // Sectors per cluster
    WORD    cSecRes;        // Reserved sectors
    BYTE    cFAT;           // FATs
    WORD    cDir;           // Root Directory Entries
    WORD    cSec;           // Total number of sectors in image
    BYTE    bMedia;         // Media descriptor
    WORD    secPerFAT;      // Sectors per FAT
    WORD    secPerTrack;    // Sectors per track
    WORD    cHead;          // Heads
    DWORD   cSecHidden;     // Hidden sectors
    DWORD   cTotalSectors;  // Total sectors, if cbSec is zero
    BYTE    reserved[6];    //
} DOSDPB, *PDOSDPB;
#pragma pack()


/*
   GetDriveFormFactor returns the drive form factor.

   It returns 350 if the drive is a 3.5" floppy drive.
   It returns 525 if the drive is a 5.25" floppy drive.
   It returns 800 if the drive is a 8" floppy drive.
   It returns   0 on error.
   It returns   1 if the drive is a hard drive.
   It returns   2 if the drive is something else.

   iDrive is 1 for drive A:, 2 for drive B:, etc.

   Note that iDrive is not range-checked or otherwise validated.
   It is the responsibility of the caller only to pass values for
   iDrive between 1 and 26 (inclusive).

   Although the function, as written, returns only the drive form
   factor, it can easily be extended to return other types of
   drive information.
*/

DWORD GetDriveFormFactor(int iDrive)
{
    HANDLE h;
    TCHAR tsz[8];
    DWORD dwRc;

    if ((int)GetVersion() < 0)
    {
        // Windows 95

        /*
           On Windows 95, use the technique described in
           MSDN under "Windows 95 Guide to Programming", "Using Windows 95 features", "Using VWIN32 to Carry Out MS-DOS Functions".
        */
        h = CreateFileA("\\\\.\\VWIN32", 0, 0, 0, 0,
                        FILE_FLAG_DELETE_ON_CLOSE, 0);

        if (h != INVALID_HANDLE_VALUE)
        {
            DWORD          cb;
            DIOC_REGISTERS reg;
            DOSDPB         dpb;

            dpb.specialFunc = 0;  // return default type; do not hit disk

            reg.reg_EBX   = iDrive;       // BL = drive number (1-based)
            reg.reg_EDX   = (DWORD)&dpb;  // DS:EDX -> DPB
            reg.reg_ECX   = 0x0860;       // CX = Get DPB
            reg.reg_EAX   = 0x440D;       // AX = Ioctl
            reg.reg_Flags = CARRY_FLAG;   // assume failure

            // Make sure both DeviceIoControl and Int 21h succeeded.
            if (DeviceIoControl (h, VWIN32_DIOC_DOS_IOCTL, &reg,
                                 sizeof(reg), &reg, sizeof(reg),
                                 &cb, 0)
                    && !(reg.reg_Flags & CARRY_FLAG))
            {
                switch (dpb.devType)
                {
                case 0: // 5.25 360K floppy
                case 1: // 5.25 1.2MB floppy
                    dwRc = 525;
                    break;

                case 2: // 3.5  720K floppy
                case 7: // 3.5  1.44MB floppy
                case 9: // 3.5  2.88MB floppy
                    dwRc = 350;
                    break;

                case 3: // 8" low-density floppy
                case 4: // 8" high-density floppy
                    dwRc = 800;
                    break;

                case 5: // hard drive
                    dwRc = 1;
                    break;

                case 6: // tape drive
                case 8: // optical disk
                    dwRc = 2;
                    break;

                default: // unknown
                    dwRc = 0;
                    break;
                }
            }
            else
                dwRc = 0;


            CloseHandle(h);
        }
        else
            dwRc = 0;

    }
    else
    {
        // Windows NT

        /*
           On Windows NT, use the technique described in the Knowledge
           Base article Q115828 and in the "FLOPPY" SDK sample.
        */
        wsprintf(tsz, TEXT("\\\\.\\%c:"), TEXT('@') + iDrive);
        h = CreateFile(tsz, 0, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
        if (h != INVALID_HANDLE_VALUE)
        {
            DISK_GEOMETRY Geom[20];
            DWORD cb;

            if (DeviceIoControl (h, IOCTL_DISK_GET_MEDIA_TYPES, 0, 0,
                                 Geom, sizeof(Geom), &cb, 0)
                    && cb > 0)
            {
                switch (Geom[0].MediaType)
                {
                case F5_1Pt2_512: // 5.25 1.2MB floppy
                case F5_360_512:  // 5.25 360K  floppy
                case F5_320_512:  // 5.25 320K  floppy
                case F5_320_1024: // 5.25 320K  floppy
                case F5_180_512:  // 5.25 180K  floppy
                case F5_160_512:  // 5.25 160K  floppy
                    dwRc = 525;
                    break;

                case F3_1Pt44_512: // 3.5 1.44MB floppy
                case F3_2Pt88_512: // 3.5 2.88MB floppy
                case F3_20Pt8_512: // 3.5 20.8MB floppy
                case F3_720_512:   // 3.5 720K   floppy
                    dwRc = 350;
                    break;

                case RemovableMedia:
                    dwRc = 2;
                    break;

                case FixedMedia:
                    dwRc = 1;
                    break;

                default:
                    dwRc = 0;
                    break;
                }
            }
            else
                dwRc = 0;

            CloseHandle(h);
        }
        else
            dwRc = 0;
    }

    /*
       If you are unable to determine the drive type via ioctls,
       then it means one of the following:

       1.  It is hard drive and we do not have administrator privilege
       2.  It is a network drive
       3.  The drive letter is invalid

       GetDriveType can distinguish these three cases.
    */
    if (dwRc == 0)
    {
        wsprintf(tsz, TEXT("%c:\\"), TEXT('@') + iDrive);

        switch (GetDriveType(tsz))
        {
        case DRIVE_FIXED:
            dwRc = 1;
            break;
        case DRIVE_REMOVABLE:
        case DRIVE_REMOTE:
        case DRIVE_CDROM:
        case DRIVE_RAMDISK:
            dwRc = 2;
            break;
        default:
            dwRc = 0;
        }
    }
    return dwRc;
}


DWORD killProc(const char *strProc)
{
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    PROCESSENTRY32 pEntry;
    bool isKilled = false,procExist = false;

    pEntry.dwSize = sizeof (pEntry);
    BOOL hRes = Process32First(hSnapShot, &pEntry);

    while (hRes)
    {
        if (strcmp(pEntry.szExeFile, strProc) == 0)
        {
            procExist = true;
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
                                          (DWORD) pEntry.th32ProcessID);
            if (hProcess != NULL)
            {
                isKilled = TerminateProcess(hProcess, 9);
                CloseHandle(hProcess);
            }
        }
        hRes = Process32Next(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);
    if(procExist&&isKilled)return 0;
    else if(procExist&&(!isKilled))return 1;
    else if(!procExist)return 2;
}
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process;

bool IsWow64()
{
    BOOL bIsWow64 = FALSE;

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
                GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
            //handle error
        }
    }
    return bIsWow64;
}
#define HKCR "HKEY_CLASSES_ROOT"
#define HKCU "HKEY_CURRENT_USER"
#define HKLM "HKEY_LOCAL_MACHINE"
#define HKUS "HKEY_USERS"
#define HKCC "HKEY_CURRENT_CONFIG"

DWORD removeReg(int index)
{

    int err = 0;
    for(int i = 0;i<defs[index].keys.size();i++)
    {
        std::string strKeyName = defs[index].keys[i].key;
        int nHandle;
        int rokRetVal = 0;
        HKEY key;
        if(strncmp(HKCR,strKeyName.c_str(),strlen(HKCR)) == 0)
        {
            nHandle = 0;
            strKeyName =  strKeyName.substr(strlen(HKCR)+1);
        }
        else if(strncmp(HKCU,strKeyName.c_str(),strlen(HKCU)) == 0)
        {
            nHandle = 1;
            strKeyName =  strKeyName.substr(strlen(HKCU)+1);
        }
        else if(strncmp(HKLM,strKeyName.c_str(),strlen(HKLM)) == 0)
        {
            nHandle = 2;
            strKeyName =  strKeyName.substr(strlen(HKLM)+1);
        }
        else    if(strncmp(HKUS,strKeyName.c_str(),strlen(HKUS)) == 0)
        {
            nHandle = 3;
            strKeyName =  strKeyName.substr(strlen(HKUS)+1);
        }
        else    if(strncmp(HKCC,strKeyName.c_str(),strlen(HKCC)) == 0)
        {
            nHandle = 4;
            strKeyName =  strKeyName.substr(strlen(HKCC)+1);
        }

        switch (nHandle)
        {
        case 0:
            rokRetVal = RegOpenKey(HKEY_CLASSES_ROOT,strKeyName.c_str(),&key);
            break;
        case 1:
            rokRetVal = RegOpenKey(HKEY_CURRENT_USER,strKeyName.c_str(),&key);
            break;
        case 2:
            if(IsWow64())
                rokRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE,strKeyName.c_str(),0,KEY_ALL_ACCESS|KEY_WOW64_64KEY,&key);
            else rokRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE,strKeyName.c_str(),0,KEY_ALL_ACCESS,&key);
            break;
        case 3:
            rokRetVal = RegOpenKey(HKEY_USERS,strKeyName.c_str(),&key);
            break;
        case 4:
            rokRetVal = RegOpenKey(HKEY_CURRENT_CONFIG,strKeyName.c_str(),&key);
            break;
        }

        if(rokRetVal == ERROR_SUCCESS)
        {
            ErrorOut(lgFile,"Registry Key :"+defs[index].keys[i].key+" exist",0);
            if(err += RegDeleteValue(key,defs[index].keys[i].value.c_str())!=ERROR_SUCCESS)
            {
                ErrorOut(lgFile,"Error Deleting Registry Value:"+defs[index].keys[i].value,1);
            }
            else
                ErrorOut(lgFile,"Registry Key Value:"+defs[index].keys[i].value+" Deleted.",0);
        }
    }

    if(err == 0)return 0;
    else return 1;
}

void prepString()
{
    def tmp;
    reg rg;
    char *strBuf;
    int flag = 0;
    for(unsigned int i = 0; i<nDefs; i++)
    {
        char Buf[512] = {0};
        LoadString(GetModuleHandle(NULL),DF_SKYPEE+i,Buf,512);
        strBuf = strtok(Buf,"~");
        tmp.files.push_back(strBuf);
        while((strBuf = strtok(NULL,"~"))!= NULL)
        {
            if(strBuf[0] == 'r')
            {
                //its a reg string
                if(flag == 2||flag == 3)
                {
                    tmp.keys.push_back(rg);
                    rg.value.clear();
                    rg.key.clear();
                }
                flag = 1;
            }
            else if(strBuf[0] == 'v')
            {
                //its a key value
                if(flag == 3)tmp.keys.push_back(rg);
                if(flag == 2) flag = 3;
            }
            else
            {
                if(flag == 1)
                {
                    rg.key = strBuf;
                    //strdup(rg.key);
                    flag = 2;
                }
                else if(flag == 3)
                {
                    rg.value = strBuf;
                }
                else {
                    tmp.files.push_back(strBuf);
                }
            }
        }
        tmp.keys.push_back(rg);
        defs.push_back(tmp);
        flag = 0;
        tmp.files.clear();
        tmp.keys.clear();
        rg.key.clear();
        rg.value.clear();
    }
}


void addRec(char *path, int index, int subindex)
{
    reps.defIndex.push_back(index);
    reps.subIndex.push_back(subindex);
    reps.paths.push_back(path);
}

void ErrorOut(FILE *logFile,std::string args,int opt = 1)
{
    LPWSTR lpBuffer = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|
                  FORMAT_MESSAGE_FROM_SYSTEM|
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                  (LPSTR)&lpBuffer,0,NULL);
    if(opt >0)
        fprintf(logFile,"%s:%s\n",args.c_str(),lpBuffer);
    else
        fprintf(logFile,"%s.\n",args.c_str());
}

extern std::string appDir;

DWORD WINAPI runRepairs(LPVOID args)
{
    SetCurrentDirectory(appDir.c_str());
    lgFile = fopen("log.txt","a");
    for(unsigned int i = 0;i<reps.paths.size();i++)
    {
        int nDefIndx = reps.defIndex[i];
        int nSubIndex = reps.subIndex[i];
        if(nDefIndx == -1 &&(opt&FIX_ALL))
        {
            SendMessage((HWND)args,WM_UPDATE_MSG,0,(LPARAM)"Deleting a supsected file");
            if(DeleteFile(reps.paths[i].c_str()))
            {
                ErrorOut(lgFile,"Error Deleting:"+reps.paths[i]);
            }
            else ErrorOut(lgFile,"File deleted"+reps.paths[i],0);
            continue;
        }

        if(opt &(FIX_REGISTRY|FIX_ALL))
        {
            SendMessage((HWND)args,WM_UPDATE_MSG,0,(LPARAM)"Trying to remove a Registry Entry..:)");
            if(removeReg(nDefIndx)>0)
            {
                ErrorOut(lgFile,"Error removing some registry...:(",0);
            }
            SendMessage((HWND)args,WM_UPDATE_MSG,0,(LPARAM)"Trying to stop a process..:)");
            int nKill = killProc(defs[nDefIndx].files[nSubIndex].c_str());
            switch(nKill)
            {
            case 0:
                ErrorOut(lgFile,"Process:"+defs[nDefIndx].files[nSubIndex]+" Successfully Stopped.",0);
                break;
            case 1:
                ErrorOut(lgFile,"Error Stopping a Process..:(");
                break;
            case 2:
                ErrorOut(lgFile,"Process:"+defs[nDefIndx].files[nSubIndex]+" is not Running.",0);
                break;
            }
        }
        if(opt&(FIX_HIDDEN|FIX_ALL))
        {
            SendMessage((HWND)args,WM_UPDATE_MSG,0,(LPARAM)"Showing Hidden file..:)");
            if(SetFileAttributes(reps.paths[i].c_str(),(GetFileAttributes(reps.paths[i].c_str())
                                                        &~(FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY))) == 0)
            {
                ErrorOut(lgFile,"Error Changing Attributes of:"+reps.paths[i]);
            }

        }
        if((int)opt&(int)FIX_ALL)
        {
            SendMessage((HWND)args,WM_UPDATE_MSG,0,(LPARAM)"Deleting a file..:)");
            if(DeleteFile(reps.paths[i].c_str()))
            {
                ErrorOut(lgFile,"File Deleted:"+reps.paths[i],0);
            }
            else ErrorOut(lgFile,"Error Deleting File:"+reps.paths[i]);
        }
        SendMessage((HWND)args,WM_STEP_PB,0,0);
    }

    fflush(lgFile);
    fclose(lgFile);
    SendMessage((HWND)args,WM_REP_DONE,0,0);
}
