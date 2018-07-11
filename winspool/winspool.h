#ifndef _DLL_WINSPOOL_H_
#define _DLL_WINSPOOL_H_
#include <iostream>
#include <windows.h>
#include <winspool.h>
#include <atlstr.h>

#if defined DLL_EXPORT
#define DECLDIR __declspec(dllexport)
#else
#define DECLDIR __declspec(dllimport)
#endif

extern "C"
{
    DECLDIR int DelPrinter(char* name);
    DECLDIR int DelPort(LPWSTR port);
    DECLDIR char* GetDefPrinter();
    DECLDIR int SetDefPrinter(char* name);
    DECLDIR char* GetLocalPrinters();
    DECLDIR char* EnumDrivers();
    DECLDIR int PrinterHasJob(char* name);
    DECLDIR int AddPrintDriver(char* infpath,char* drvname,char* arch);
    DECLDIR int AddMyPrinter(char* name,char* portname, char* drvname);
    DECLDIR int CreatePort(char* port);
}

#endif