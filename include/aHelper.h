#ifndef AHELPER_H
#define AHELPER_H
#include <string>
#include <iostream>
#include <shlobj.h>
#include <accctrl.h>
#include <aclapi.h>
#include <objbase.h>
#include "resource.h"
#include <process.h>
#include <Tlhelp32.h>
#include <windows.h>
#include <commctrl.h>
#include <winioctl.h>                   // Windows NT IOCTL codes
#include <vector>
#include "resource.h"
#include <cstdio>

#define KEY_WOW64_64KEY (0x0100)

#define     WM_UPDATE_MSG                       WM_USER+0
#define     WM_STEP_PB                          WM_USER+2
#define     WM_UPDATE_DET                       WM_USER+3
#define     WM_CHK_DONE                         WM_USER+4
#define     WM_REP_DONE                         WM_USER+5

//typedef BOOL (WINAPI *PFN_SETRESTOREPTW) (PRESTOREPOINTINFOW, PSTATEMGRSTATUS);
typedef std::vector<HANDLE> vHandle;
typedef std::vector<std::string> vString;

enum _options
{
    UNKNOWN = 0x0,
    SCAN_SYSTEM = 1 << 1,
    SCAN_DRIVE = 1 << 2,
    FIX_REGISTRY = 1 << 3,
    FIX_HIDDEN = 1 << 4,
    FIX_ALL = 1 << 5,
    RESET_PC = 1 <<6
};
//#ifndef DEFINE_ENUM_FLAG_OPERATORS(_options)

struct reg
{
    std::string key;
    std::string value;
};
struct def
{
    vString files;
    std::vector <reg> keys;
};

typedef std::vector<def> vDef;
const unsigned int nDefs = (DF_HIDV - DF_SKYPEE)+1;

struct repStruct
{
    vString paths;
    std::vector<int>defIndex;
    std::vector<int>subIndex;
};



DWORD GetDriveFormFactor(int iDrive);
DWORD removeReg(char *strReg, char *value);
DWORD killProc(const char *strProc);
void ErrorOut(FILE *logFile, std::string, int opt);
void prepString();
void addRec(char*,int,int);
DWORD WINAPI runRepairs(LPVOID);
#endif // AHELPER_H
