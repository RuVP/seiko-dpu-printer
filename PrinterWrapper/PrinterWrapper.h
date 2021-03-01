#pragma once

#include <string>
#include <windows.h>
#include <stdexcept>

using namespace System;
using namespace System::Collections::Generic;

#ifndef EXPORT
#define EXPORT
#endif

typedef INT(CALLBACK EXPORT* FNC_CB_STATUS)(DWORD);
typedef DWORD(WINAPI* F_OpenSiiPrinter)(LPCTSTR, LPDWORD);
typedef DWORD(WINAPI* F_CloseSiiPrinter)(DWORD);
typedef DWORD(WINAPI* F_SetSiiPrinterData)(DWORD, LPBYTE, DWORD, LPDWORD);
typedef DWORD(WINAPI* F_SetSiiPrinterReset)(DWORD);
typedef DWORD(WINAPI* F_SetSiiPrinterCallbackStatus)(DWORD, FNC_CB_STATUS);

#pragma region Commented Code for future reference

//typedef DWORD(WINAPI* F_GetSiiPrinterData)(DWORD, LPTSTR, LPBYTE, DWORD, LPDWORD);
//typedef DWORD(WINAPI* F_GetSiiPrinterAutoStatus)(DWORD, LPDWORD);
//typedef DWORD(WINAPI* F_SetSiiPrinterTimeout)(DWORD, DWORD);

#pragma endregion

#pragma region Pointer to Printer Functions

#define PFN_OPEN_PRINTER		((F_OpenSiiPrinter)GetProcAddress( hModule, "OpenSiiPrinterA"))
#define PFN_CLOSE_PRINTER		((F_CloseSiiPrinter)GetProcAddress( hModule, "CloseSiiPrinter"))
#define PFN_SET_PRINTER_DATA	((F_SetSiiPrinterData)GetProcAddress( hModule, "SetSiiPrinterData"))
#define PFN_SET_PRINTER_RESET	((F_SetSiiPrinterReset)GetProcAddress( hModule, "SetSiiPrinterReset"))
#define PFN_SET_CALLBACK_STATUS	((F_SetSiiPrinterCallbackStatus)GetProcAddress( hModule, "SetSiiPrinterCallbackStatus"))

#pragma endregion

#pragma region Commented Code for future reference

// These might be required in the future.
//#define PFN_GET_PRINTER_STATUS	((F_GetSiiPrinterAutoStatus)GetProcAddress( hModule, "GetSiiPrinterAutoStatus"))
//#define PFN_SET_PRINTER_TIMEOUT	((F_SetSiiPrinterTimeout)GetProcAddress( hModule, "SetSiiPrinterTimeout"))

#pragma endregion

#define	DLL_FILE_NAME		TEXT("SII_DPUD_API.DLL")

#pragma region Printer Commands Definition

#define	CMD_INITIALIZE			{0x1B, 0x40}
#define	CMD_LINE_FEED			{0x0A}
#define	CMD_RASTER_IMAGE		{0x1D, 'v', '0', '\0', 0x20, '\0', 0x78, 0x0, 0x0, 0xf, 0x3e };
#define	CMD_JUSTIFY_CENTER		{0x1B, 'a', '1'}

#pragma endregion

static const std::string BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static const int NUMBER_OF_CHARACTERS_PER_LINE = 25;

namespace SeikoPrinter {

	public ref class SlipReceipt
	{
	public:
		SlipReceipt();

		~SlipReceipt();

		!SlipReceipt();

		String^ logo;
		String^ printDateTime;
		String^ content;
	};

	public ref class PrinterHelperWrapper
	{
	private:

		void PrintReceipt(SlipReceipt^ slipReceipt);
	public:
		// Managed wrappers are generally less concerned
		// with copy constructors and operators, since .NET will
		// not call them most of the time.
		// The methods that do actually matter are:
		// The constructor, the "destructor" and the finalizer
		PrinterHelperWrapper();

		~PrinterHelperWrapper();

		!PrinterHelperWrapper();

		static int printerStatusCode;

		bool OpenPrinterDevice();

		bool ConnectPrinterDevice(String^ sPrinterName);

		void PrintSlipReceipt(SlipReceipt^ slipReceipt);

		int GetPrinterStatusCode();

		void ResetPrinterDevice();

		void SetPrinterStatusCallback(BOOL bStart);
	};
}
