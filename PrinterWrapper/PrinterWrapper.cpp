#include "PrinterWrapper.h"

using namespace std;

#pragma region Private

static HMODULE	hModule = NULL;		// handle for dll
static DWORD	dwSessionId = 0;	// Session ID for dll's function
DWORD pcWritten = 0;
static BYTE	pLineFeed[] = CMD_LINE_FEED;
static BYTE	pInitializeAndClearBuffer[] = CMD_INITIALIZE;
static BYTE pJustifyCenter[] = CMD_JUSTIFY_CENTER;
static BYTE pImageHeader[] = CMD_RASTER_IMAGE;
static BYTE	pUnderLine[] = "\n________________________________\n";
static int nPrintMode = 0;
static string sPrinterDeviceName;

enum class EPrintModeOption {
	CharacterFont = 0,	// 0th bit controls CharacterFont
	Bold = 3,			// 3rd bit controls Bold
	DoubleHeight = 4,	// 4th bit controls DoubleHeight
	DoubleWidth = 5,	// 5th bit controls DoubleWidth
	Underline = 7		// 7th bit controls Underline
};

/**
* Determines whether or not a character is Base64
*
* Copyright (C) 2004-2008 René Nyffenegger
*/
static inline bool IsBase64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

/**
* Converts a Base64 Encoded String to standard string.
*
* Copyright (C) 2004-2008 René Nyffenegger
*/
std::string Base64Decode(std::string const& encoded_string) {
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && (encoded_string[in_] != '=') && IsBase64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = BASE64_CHARS.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = BASE64_CHARS.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

	return ret;
}

/**
* Converts System::String^ to std::string
*
* @param string: The System::String^ that needs to be converted.
* @returns Converted std::string.
*/
static string MarshalString(String^ string)
{
	const char* str = (const char*)(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(string)).ToPointer();
	return str;
}

#pragma endregion

#pragma region Print Help Functions

/**
 * Resets Print Mode to default
 *
 */
void ResetPrintMode()
{
	nPrintMode = 0;
	BYTE pPrintMode[] = { 0x1B, '!', nPrintMode };

	PFN_SET_PRINTER_DATA(dwSessionId, pPrintMode, sizeof(pPrintMode), &pcWritten);
}

/**
 * Sets Print Mode
 *
 * @param printModeOption: The print mode option that needs to be changed.
 * @param doEnable: Boolean to determine whether to set or unset the bit corresponding to @printModeOption.
 */
void SetPrintMode(EPrintModeOption printModeOption, bool doEnable)
{
	if (doEnable)
		nPrintMode |= 1UL << (int)printModeOption;
	else
		nPrintMode &= ~(1UL << (int)printModeOption);

	BYTE pPrintMode[] = { 0x1B, '!', nPrintMode };

	PFN_SET_PRINTER_DATA(dwSessionId, pPrintMode, sizeof(pPrintMode), &pcWritten);
}

/**
 * Justifies printer output to center
 */
void JustifyCenter()
{
	PFN_SET_PRINTER_DATA(dwSessionId, pJustifyCenter, sizeof(pJustifyCenter), &pcWritten);
}

/**
 * Prints extra blank lines
 */
void ClearAndInitializePrinter()
{
	PFN_SET_PRINTER_DATA(dwSessionId, pInitializeAndClearBuffer, sizeof(pInitializeAndClearBuffer), &pcWritten);
}

/**
 * Prints extra blank lines
 *
 * @param numberOfLines: Number of blank lines the printer should feed
 */
void PrintNewLineFeed(int nNumberOfLines)
{
	for (int i = 0; i < nNumberOfLines; i++)
	{
		PFN_SET_PRINTER_DATA(dwSessionId, pLineFeed, sizeof(pLineFeed), &pcWritten);
	}
}

/**
 * Prints lines
 *
 * @param numberOfLines: Number of lines the printer should print.
 *							The line will cover majority of the width of the paper.
 */
void PrintLine(int nNumberOfLines)
{
	for (int i = 0; i < nNumberOfLines; i++)
	{
		PFN_SET_PRINTER_DATA(dwSessionId, pUnderLine, sizeof(pUnderLine), &pcWritten);
	}
}

/**
 * Prints image graphic
 *
 * @param sLogo: Base64 string of the logo image.
 */
void PrintGraphic(String^ sLogo)
{
	if (sLogo == nullptr)
	{
		return;
	}

	std::string sDecodedLogo = Base64Decode(MarshalString(sLogo));

	const int nImageHeaderSize = sizeof(pImageHeader);
	int nDecodedDataSize = sDecodedLogo.length();
	int nFullImageArraySize = nImageHeaderSize + nDecodedDataSize;

	char* pFullImage = new char[nFullImageArraySize];

	// Copy image header commands to pFullImage
	memcpy(pFullImage, pImageHeader, nImageHeaderSize);
	// Append the actual image data to pFullImage
	memcpy(pFullImage + nImageHeaderSize, sDecodedLogo.c_str(), nDecodedDataSize);

	PFN_SET_PRINTER_DATA(dwSessionId, (LPBYTE)pFullImage, nFullImageArraySize, &pcWritten);
}

/**
 * Prints text
 *
 * @param pTextToPrint: Pointer to the text to be printed.
 * @param nNumberOfNewLines: (Optional) Number of new lines to be printed after the text. Default value is 0.
 */
void PrintText(String^ pTextToPrint, int nNumberOfNewLines = 0)
{
	if (pTextToPrint == nullptr)
	{
		return;
	}

	PFN_SET_PRINTER_DATA(dwSessionId, (LPBYTE)MarshalString(pTextToPrint).c_str(), pTextToPrint->Length, &pcWritten);
	if (nNumberOfNewLines > 0)
	{
		PrintNewLineFeed(nNumberOfNewLines);
	}
}

/**
 * Prints text in bold
 *
 * @param pTextToPrint: Pointer to the text to be printed.
 * @param nNumberOfNewLines: (Optional) Number of new lines to be printed after the text. Default value is 0.
 */
void PrintBoldText(String^ pTextToPrint, int nNumberOfNewLines = 0)
{
	SetPrintMode(EPrintModeOption::Bold, true);
	PrintText(pTextToPrint, nNumberOfNewLines);
	ResetPrintMode();
}

/**
 * Prints footer text ensuring text is aligned without being split mid-way on multiple lines.
 *
 * @param pTextToPrint: Pointer to the text to be printed.
 * @param nNumberOfNewLines: (Optional) Number of new lines to be printed after the text. Default value is 0.
 */
void PrintFooterText(String^ pTextToPrint, int nNumberOfNewLines = 0)
{
	SetPrintMode(EPrintModeOption::Bold, true);

	cli::array<String^>^ words = pTextToPrint->Split(' ');

	String^ line = "";

	// This for loops through each word in the text. If the combined length of the line and word is:
	//   1) > 25 : Prints the line.
	//   2) <= 25: Add word to the line.
	for each (String ^ word in words)
	{
		if ((line->Length + word->Length) > NUMBER_OF_CHARACTERS_PER_LINE)
		{
			PrintText(line, 1);
			line = "";
		}

		line += (" " + word);
	}

	// There might be some text left to be printed if the line length was less than 25 when last
	// word was being processed.
	if (line->Length > 0)
	{
		PrintText(line);
	}

	PrintNewLineFeed(nNumberOfNewLines);
	ResetPrintMode();
}

#pragma endregion

SeikoPrinter::PrinterHelperWrapper::PrinterHelperWrapper()
// Allocate some memory for the native implementation
{
}

SeikoPrinter::SlipReceipt::SlipReceipt()
// Allocate some memory for the native implementation
{
}

/**
* Invokes command to open the printer for operations and sets a sessionId at printer.
* This sessionId will be used for subsequent print operation.
*/
bool SeikoPrinter::PrinterHelperWrapper::OpenPrinterDevice()
{
	try
	{
		// TODO: Check if Printer is connected and available. OpenPrinter does not check availability.
		bool isOpen = PFN_OPEN_PRINTER((LPTSTR)sPrinterDeviceName.c_str(), &dwSessionId) ? FALSE : TRUE;

		if (isOpen)
		{
			SetPrinterStatusCallback(TRUE);
		}

		return isOpen;
	}
	catch (exception& e)
	{
		throw e;
	}
}

void ClosePrinterDevice()
{
	try
	{
		PFN_CLOSE_PRINTER(dwSessionId);
	}
	catch (exception& e)
	{
		throw e;
	}
}

bool SeikoPrinter::PrinterHelperWrapper::ConnectPrinterDevice(String^ sPrinterName)
{
	try
	{
		if (sPrinterName == nullptr)
		{
			return FALSE;
		}

		if (hModule = LoadLibrary((LPCTSTR)DLL_FILE_NAME))
		{
			sPrinterDeviceName = MarshalString(sPrinterName);

			// TODO: Check if Printer is connected and available. OpenPrinter does not check availability.

			return OpenPrinterDevice();
		}
		return FALSE;
	}
	catch (exception& e)
	{
		throw e;
	}
}

/**
 * Calls the native Print function with required parameters
 *
 */
void SeikoPrinter::PrinterHelperWrapper::PrintReceipt(SlipReceipt^ slipReceipt)
{
	try
	{
		String^ sCPPText = "Hey, I am a C++ text.";
		String^ sPrintDateTimeLabel = "Printed on: ";

		if (hModule = LoadLibrary((LPCTSTR)DLL_FILE_NAME))
		{
			OpenPrinterDevice();

			PrintGraphic(slipReceipt->logo);

			// Sometimes, the buffer will have junk value after printing image. So just clear it. 
			// But this clears all settings. We have to justify center again.
			ClearAndInitializePrinter();
			JustifyCenter();

			PrintNewLineFeed(2);

			PrintBoldText(slipReceipt->content, 1);
			PrintText(sCPPText, 2);

			PrintText(sPrintDateTimeLabel, 1);
			PrintText(slipReceipt->printDateTime, 2);

			PrintNewLineFeed(2);
		}
	}
	catch (exception& e)
	{
		throw e;
	}
	finally
	{
		ClosePrinterDevice();
	}
}

/**
 * Calls the local PrintReceipt function with required parameters to print slip receipt. This is exposed to managed code.
 *
 */
void SeikoPrinter::PrinterHelperWrapper::PrintSlipReceipt(SlipReceipt^ slipReceipt)
{
	try
	{
		PrintReceipt(slipReceipt);
	}
	catch (exception& e)
	{
		throw std::runtime_error("Printer error while printing receipt.");
	}
}

int SeikoPrinter::PrinterHelperWrapper::GetPrinterStatusCode()
{
	return SeikoPrinter::PrinterHelperWrapper::printerStatusCode;
}

/**
* Does a reset operation on Printer like buffer clearing.
*/
void SeikoPrinter::PrinterHelperWrapper::ResetPrinterDevice()
{
	try
	{
		if (hModule = LoadLibrary((LPCTSTR)DLL_FILE_NAME))
		{
			OpenPrinterDevice();
			if (dwSessionId)
			{
				PFN_SET_PRINTER_RESET(dwSessionId);
				dwSessionId = 0;
			}
		}
	}
	catch (exception& e)
	{
		throw e;
	}
}

EXPORT INT CALLBACK OnStatusReceived(DWORD statusCode)
{
	SeikoPrinter::PrinterHelperWrapper::printerStatusCode = statusCode;
	return 0;
}

void SeikoPrinter::PrinterHelperWrapper::SetPrinterStatusCallback(BOOL bStart)
{
	if (bStart)
	{
		PFN_SET_CALLBACK_STATUS(dwSessionId, (FNC_CB_STATUS)OnStatusReceived);
	}
	else
	{
		PFN_SET_CALLBACK_STATUS(dwSessionId, (FNC_CB_STATUS)NULL);
	}
}

SeikoPrinter::PrinterHelperWrapper::~PrinterHelperWrapper()
{
	// C++ CLI compiler will automaticly make all ref classes implement IDisposable.
	// The default implementation will invoke this method + call GC.SuspendFinalize.
}

SeikoPrinter::PrinterHelperWrapper::!PrinterHelperWrapper()
{
	// This is the finalizer
	// It's essentially a fail-safe, and will get called
	// in case PrinterHelperWrapper was not used inside a using block.
}

SeikoPrinter::SlipReceipt::~SlipReceipt()
{
	// C++ CLI compiler will automaticly make all ref classes implement IDisposable.
	// The default implementation will invoke this method + call GC.SuspendFinalize.
}

SeikoPrinter::SlipReceipt::!SlipReceipt()
{
	// This is the finalizer
	// It's essentially a fail-safe, and will get called
	// in case PrinterHelperWrapper was not used inside a using block.
}
