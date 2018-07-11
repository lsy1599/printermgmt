// winspool.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#define DLL_EXPORT
#include "winspool.h"
#include "stdafx.h"
#include <iostream>
#include <windows.h>
#include <winspool.h>
#include <atlstr.h>
#include <Winuser.h>
//#include "tcpxcv.h"

#include <string>

using namespace std;

#define _WIN32_DCOM

#include <comdef.h>
#include <Wbemidl.h>

# pragma comment(lib, "wbemuuid.lib")
typedef HRESULT (*LPUploadPrinterDriverPackage)(
    LPCTSTR  pszServer,
    LPCTSTR  pszInfPath,
    LPCTSTR  pszEnvironment,
    DWORD    dwFlags,
    HWND     hwnd,
    LPTSTR   pszDestInfPath,
    PULONG   pcchDestInfPath
);

typedef HRESULT (*LPInstallPrinterDriverFromPackage)(
    LPCTSTR  pszServer,
    LPCTSTR  pszInfPath,
    LPCTSTR  pszDriverName,
    LPCTSTR  pszEnvironment,
    DWORD    dwFlags
);

extern "C"
{
    DECLDIR int DelPrinter(char* name)
    {
        int ret=-1;
        std::string str;
        LPBYTE pchPrinter = NULL;
        PPRINTER_INFO_2 pPrinterInfo = NULL;
        DWORD	dwBuffsize = 0,dwBuffNeeded =0;

        HANDLE     hPrinter = NULL;
        PRINTER_DEFAULTS defaults;

        defaults.pDatatype = NULL;
        defaults.pDevMode = NULL;
        defaults.DesiredAccess = PRINTER_ALL_ACCESS;
        LPWSTR portname;
        wchar_t pname[64];
        mbstowcs(pname, name, strlen(name)+1);
        LPWSTR ptr = pname;
        OpenPrinter(ptr,&hPrinter,&defaults);
        DWORD  cByteNeeded=0, cByteUsed=0;
        if(hPrinter)
        {
            if (!GetPrinter(hPrinter, 2, NULL, 0, &cByteNeeded))
            {
                if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                    return -1;
            }

            pPrinterInfo = (PRINTER_INFO_2 *)malloc(cByteNeeded);
            if (!(pPrinterInfo))
                /* Failure to allocate memory. */
                return FALSE;

            /* Get the printer information. */
            if (!GetPrinter(hPrinter,2,(LPBYTE)pPrinterInfo,cByteNeeded,&cByteUsed))
            {
                /* Failure to access the printer. */
                free(pPrinterInfo);
                pPrinterInfo = NULL;
                return FALSE;
            }
            portname=pPrinterInfo->pPortName;
            DeletePrinter(hPrinter);
        }
        if(hPrinter)
        {
            ClosePrinter(hPrinter);
        }
        if(portname!=NULL)
        {
            DelPort(portname);
        }

        return 0;
    }
    DECLDIR int DelPort(LPWSTR port)
    {
        LPTSTR prnname=NULL;
        DeletePort(prnname,0,port);
        return 0;
    }

    DECLDIR int SetDefPrinter(char* name)
    {
        wchar_t pname[64];
        mbstowcs(pname,name,strlen(name)+1);
        LPWSTR pszPrinter=pname;
        return SetDefaultPrinter(pszPrinter);
    }

    DECLDIR int CreatePort(char* port)
    {
        wchar_t portname[64];
        mbstowcs(portname,port,strlen(port)+1);
        LPWSTR portName=portname;
        HANDLE hPrinter;
        PRINTER_DEFAULTS PrinterDefaults;
        memset(&PrinterDefaults, 0, sizeof(PrinterDefaults));

        PrinterDefaults.pDatatype = NULL;
        PrinterDefaults.pDevMode = NULL;
        PrinterDefaults.DesiredAccess = SERVER_ACCESS_ADMINISTER;

        DWORD needed;
        DWORD rslt;

        if (!OpenPrinter(L",XcvMonitor Local Port", &hPrinter, &PrinterDefaults))
            return -1;

        DWORD xcvresult= 0;
        if (!XcvData(hPrinter, L"AddPort", (BYTE *)portName, (lstrlenW(portName) + 1)*2, NULL, 0, &needed, &xcvresult))
            rslt= GetLastError();

        if (!ClosePrinter(hPrinter))
            rslt= GetLastError();

        return (int)rslt;
    }

    DECLDIR int AddPrintDriver(char* infpath,char* drvname,char* arch)
    {
        HWND     hwnd=NULL;

        wchar_t strInfpath[MAX_PATH*4 + 1];
        mbstowcs(strInfpath,infpath,strlen(infpath)+1);
        LPWSTR pszInfpath=strInfpath;
        wchar_t strdrvname[MAX_PATH*4 + 1];
        mbstowcs(strdrvname,drvname,strlen(drvname)+1);
        LPWSTR pszdrvname=strdrvname;
        wchar_t strarch[MAX_PATH*4 + 1];
        mbstowcs(strarch,arch,strlen(arch)+1);
        LPWSTR pszarch=strarch;

        TCHAR szDestInfPath[MAX_PATH*4 + 1];
        DWORD dwDestLen = MAX_PATH*4;
        LPUploadPrinterDriverPackage pUploadPackage = NULL;
        HINSTANCE hInstance = ::LoadLibrary(_TEXT("Winspool.drv"));
        pUploadPackage = (LPUploadPrinterDriverPackage)GetProcAddress(hInstance, "UploadPrinterDriverPackageW");
        if(pUploadPackage==NULL)
            return 0;

        HRESULT lResult = pUploadPackage(
                              NULL,
                              pszInfpath,
                              //_TEXT("Windows x64"),
                              pszarch,
                              UPDP_SILENT_UPLOAD | UPDP_UPLOAD_ALWAYS,
                              hwnd,
                              szDestInfPath,
                              &dwDestLen
                          );

        LPInstallPrinterDriverFromPackage pInstallPackage = NULL;
        pInstallPackage = (LPInstallPrinterDriverFromPackage)GetProcAddress(hInstance, "InstallPrinterDriverFromPackageW");
        if(pInstallPackage!=NULL)
        {
            lResult = pInstallPackage(NULL,
                                      szDestInfPath,
                                      pszdrvname,
                                      pszarch,
                                      IPDFP_COPY_ALL_FILES
                                     );
        }
        if(pUploadPackage)
            pUploadPackage = NULL;
        if(pInstallPackage)
            pInstallPackage = NULL;

        FreeLibrary(hInstance);
        return (int)lResult;
    }

    DECLDIR int AddMyPrinter(char* name,char* portname, char* drvname)
    {
        HANDLE pHandle=NULL;
        int bret=0;
        wchar_t strname[64];
        mbstowcs(strname,name,strlen(name)+1);
        LPWSTR pszname=strname;
        wchar_t strdrvname[64];
        mbstowcs(strdrvname,drvname,strlen(drvname)+1);
        LPWSTR pszdrvname=strdrvname;
        wchar_t strportname[64];
        mbstowcs(strportname,portname,strlen(portname)+1);
        LPWSTR pszportname=strportname;

        long error;

        PRINTER_INFO_2 pPrinterInfo;
        BOOL Result = FALSE;

// Memory initialisation - pPrinterInfo structure to zero
        memset(&pPrinterInfo,0,sizeof(PRINTER_INFO_2));
        pPrinterInfo.pPrinterName=pszname;
        pPrinterInfo.pPortName =pszportname;
        pPrinterInfo.pDriverName=pszdrvname;
        LPWSTR prtname=NULL;
        pHandle = AddPrinter(prtname,2, (LPBYTE)&pPrinterInfo);
        error=GetLastError();

        if(pHandle)
        {
            ClosePrinter(pHandle);
            bret=1;
        }

        return bret;
    }
    DECLDIR char* EnumDrivers()
    {
        DWORD NoOfDevices =0;
        DWORD ReqSize =0;
        DWORD cbBuf =0;
        BOOL bRetVal=-1;
        long error=0;
        std::string str;
        int retval=-1;
        DRIVER_INFO_1* pDriverEnum=NULL;
        bRetVal=EnumPrinterDrivers(NULL, NULL, 1,  (LPBYTE)pDriverEnum, cbBuf, &ReqSize, &NoOfDevices);
        if(!bRetVal)
        {
            error=GetLastError();
        }
        if((pDriverEnum = (DRIVER_INFO_1*) malloc( sizeof(DRIVER_INFO_1)*ReqSize )) == 0 )
        {
            return NULL;
        }
        bRetVal=EnumPrinterDrivers(NULL, NULL, 1,  (LPBYTE)pDriverEnum, ReqSize, &ReqSize, &NoOfDevices);
        if(!bRetVal)
        {
            error=GetLastError();
        }
        if(pDriverEnum)
        {
            //str.append("[");
            for ( int i = 0, sl = 0; i < (int)NoOfDevices; i++ )
            {
                if(pDriverEnum[i].pName != NULL)
                {
                    str.append("\"");
                    str.append(CW2A(pDriverEnum[i].pName));
                    str.append("\"");
                    if(i != ((int)NoOfDevices-1))
                    {
                        str.append(",");
                    }
                    str.shrink_to_fit();
                }
            }
            //str.append("]");
        }
        char *list = new char[str.length() + 1];
        strcpy(list, str.c_str());
        return list;
    }
    DECLDIR int PrinterHasJob(char* name)
    {
        wchar_t pname[64];
        mbstowcs(pname,name,strlen(name)+1);
        LPWSTR ptr=pname;
        DWORD bRetVal = 0;
        HANDLE     hPrinter = NULL;
        PRINTER_DEFAULTS defaults;
        long error;
        defaults.pDatatype = NULL;
        defaults.pDevMode = NULL;
        defaults.DesiredAccess = PRINTER_ALL_ACCESS;
        OpenPrinter(ptr,&hPrinter,&defaults);
        if(hPrinter)
        {
            JOB_INFO_1* pJob=NULL;

            DWORD pcReturned,pcbNeeded,retval;
            //EnumJobs(hPrinter,firstjob=0,numberofjobs=1,level=1,_Out_  LPBYTE pJob,_In_   DWORD cbBuf,_Out_  LPDWORD pcbNeeded,_Out_  LPDWORD pcReturned);
            retval=EnumJobs(hPrinter,0,1,1,(LPBYTE)pJob,1,&pcbNeeded, &pcReturned);
            if(!bRetVal)
            {
                error=GetLastError();
            }
            if((pJob = (JOB_INFO_1*) malloc( pcbNeeded )) == 0 )
            {
                return 0;
            }

            retval=EnumJobs(hPrinter,0,1,1,(LPBYTE)pJob,pcbNeeded,&pcbNeeded,&pcReturned);
            if((pcbNeeded>0) && (retval==1))
            {
                bRetVal = 1;
                error=GetLastError();
            }
        }
        if(hPrinter)
        {
            ClosePrinter(hPrinter);
        }
        return bRetVal;
    }
    DECLDIR char* GetDefPrinter()
    {
        std::string str;
        char *def;

        WCHAR* szPrinterName=L"NULL";
        DWORD lPrinterNameLength=0;
        DWORD bVal=-1;

        bVal=GetDefaultPrinter(NULL,&lPrinterNameLength);
        szPrinterName = (WCHAR*) malloc(sizeof(WCHAR)*lPrinterNameLength);
        memset(szPrinterName, 0, sizeof(WCHAR)*lPrinterNameLength);

        bVal=GetDefaultPrinter(szPrinterName,&lPrinterNameLength);

        if(bVal!=0)
        {
            str.append("\"");
            str.append(CW2A(szPrinterName));
            str.append("\"");
        }
        def = new char[str.length() + 1];
        strcpy(def, str.c_str());

        return def;
    }

    DECLDIR char* GetLocalPrinters(/*char** strPrinters*/)
    {
        BOOL bRetVal=-1;//EnumPrinters Return value

        std::string str; //by default this string is initialized with zero.

        PRINTER_INFO_2*  pPrinterEnum=NULL;
        DWORD NoOfDevices=0,ReqSize=0;
        long error=0;
        bRetVal=EnumPrinters(PRINTER_ENUM_LOCAL,NULL,2,NULL,1,&ReqSize,&NoOfDevices);
        if(!bRetVal)
        {
            error=GetLastError();
        }
        if((pPrinterEnum = (PRINTER_INFO_2*) malloc(sizeof(PRINTER_INFO_2)*ReqSize )) == 0 )
        {
            return NULL;
        }

        bRetVal=EnumPrinters(PRINTER_ENUM_LOCAL,NULL,2,(LPBYTE)pPrinterEnum,ReqSize,&ReqSize,&NoOfDevices);
        if(!bRetVal)
        {
            error=GetLastError();
        }
        if(pPrinterEnum)
        {
            //str.append("[");
            for ( int i = 0, sl = 0; i < (int)NoOfDevices; i++ )
            {
                if(pPrinterEnum[i].pPrinterName != NULL)
                {
                    str.append("\"");
                    str.append(CW2A(pPrinterEnum[i].pPrinterName));
                    str.append("\"");
                    if(i != ((int)NoOfDevices-1))
                    {
                        str.append(",");
                    }
                    str.shrink_to_fit();
                }
            }
            //str.append("]");
            char *list = new char[str.length() + 1];
            strcpy(list, str.c_str());

            return list;
        }
    }
}