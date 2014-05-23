#ifndef AHELPER_H
#define AHELPER_H


#include <string>
#include <iostream>
#include <shlobj.h>
#include <accctrl.h>
#include <aclapi.h>
#include <objbase.h>
#include <process.h>
#include <Tlhelp32.h>
#include <winbase.h>
#include <windows.h>
#include <commctrl.h>
#include <winioctl.h>                   // Windows NT IOCTL codes





DWORD GetDriveFormFactor(int iDrive);

#endif // AHELPER_H
