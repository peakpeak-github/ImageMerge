///////////////////////////////////////////////////////////////////////////////////////
// Compile with C++17 or greater
//
#define VERSION "ImageMerge 1.00, Peak 2023-02-16"
//
// Usage example:
// Merge program & FS images, creating everything.bin:
// ImageMerge -prog firmware.bin -fs littlefs.bin -image everything.bin -offset 1024 -v
// *** Note: Find correct offset value for your board: 
// https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
//
// Distribute everything.bin to user(s)
// User uploads to board:
// Esptool.exe --baud 115200 --after no_reset write_flash --flash_size detect --flash_mode qio 0x0 everything.bin
// *** Note that flash_mode must match the board_build.flash_mode in platformio.ini
//
// Debug line in Visual C++
// -prog firmware.bin -fs littlefs.bin -image everything.bin -offset 1024  -v
//
#ifdef _WIN32
#if defined _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif
#include <filesystem>
#include <iostream>
#include <fstream>

using namespace std;
typedef unsigned char uchar;
#include "main.h"

#define MAXOFFSET (uint32_t)32768
#define ONEKILOBYTE (uint32_t)1024

constexpr const char* BYTES = "bytes";
constexpr const char* KILOBYTES = "kilobytes";
constexpr const char* MEGABYTES = "megabytes";

// For use in other projects with greater file sizes
//#define ONEKILOBYTE (uint64_t)1024
//constexpr const char* GIGABYTES = "gigabytes";
//constexpr const char* TERABYTES = "terabytes";
//constexpr const char* PETABYTES = "petabytes";
//constexpr const char* EXABYTES = "exabytes";
//
// Case sensitive option strings
//
const char* g_optTable[] =
{
	"prog",
	"fs",
	"image",
	"offset",
	"fillchar",
	"V",		// First or only letter upper case -> Requires no argument
	"H",
};
const int g_optCount = sizeof(g_optTable) / sizeof(g_optTable[0]);

enum // Match entries in g_optTable above
{
	PROGNAME,
	FSNAME,
	IMAGENAME,
	OFFSET,
	FILLCHAR,
	V,
	H,
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Let's go!
//
int main(int argc, char **argv)
{
#ifdef _WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); 	// Set flags to call function _CrtDumpMemoryLeaks automatically at program termination
#endif
	uchar* progImage = NULL;
	uchar* fsImage = NULL;
	uchar* image;
	char* optValPtr = NULL;
	char* progFileName = NULL;
	char* fsFileName = NULL;
	char *imageFileName = NULL;
	char* fillChar = (char*)"\0";
	bool verbose = false;
	uint32_t progSize, fsSize, offset = 512;
	uint32_t imageSize;
	int optNum;

	if (argc == 1)		// No options, show help
		help();			// No return
	while (--argc > 0)	// First argument is program name
	{
		optNum = GetOptVal(argc, argv, optValPtr);
		//
		// optValPtr is pointing to an actual value if the option requires a value
		// OR in case the option needs no argument, the option itself
		//
		switch (optNum)
		{
		case PROGNAME:
			progFileName = optValPtr;
			break;
		case FSNAME:
			fsFileName = optValPtr;
			break;
		case IMAGENAME:
			imageFileName = optValPtr;
			break;
		case OFFSET:
			offset = atoi(optValPtr);	// 512, 1024 ... 32768 
			if ((offset % 512) != 0)
			{
				printf("Invalid offset %d. Must be a multiple of 512\n", offset);
				return -1;
			}
			if (offset > MAXOFFSET)
			{
				printf("Invalid offset %d. Must be between 512 and %d\n", offset, MAXOFFSET);
				return -1;
			}
			offset *= ONEKILOBYTE;	// -> kBytes
			break;
		case FILLCHAR:
			fillChar = optValPtr;
			break;
		case V:
			verbose = true;
			break;
		case H:
			help();	// No return
			[[fallthrough]];					// Silence compiler warning
		case MISSINGVALUE:						// Option value missing
			printf("Missing value for %s\n", optValPtr);
			help();
			[[fallthrough]];
		case NOOPTION:							// No option given, just the value
			printf("No options given\n");
			help();
			[[fallthrough]];
		case NOTFOUND:							// -option not found
			printf("%s illegal option\n", optValPtr);
			help();
		} // switch()
	} // while
	if (verbose)
		printf("Offset %d / 0x%x\n", offset, offset);
	progSize = ReadBinaryFile(progFileName, progImage);	// Read in our program part
	if (progSize == 0)
	{
		printf("Cannot read %s\n", progFileName);
		return -1;
	}
	if (verbose)
		printf("%s, size %s\n", progFileName, FormatSize(progSize));
	fsSize = ReadBinaryFile(fsFileName, fsImage);						// Read in the filesystem part
	if (fsSize == 0)
	{
		printf("Cannot read %s\n", fsFileName);
		delete[] progImage;									// Clean up
		return -1;
	}
	if (verbose)
		printf("%s, size %s\n", fsFileName, FormatSize(fsSize));

	imageSize = offset + fsSize;						// Total size of the merged image
	image = new uchar[imageSize];						// Allocate it
	memset(image, *fillChar, imageSize);				// Prefill it
	memcpy(image, progImage, progSize);					// Program in place
	memcpy(image + offset, fsImage, fsSize);			// Filesystem in place
	delete[] progImage;									// Remove originals
	delete[] fsImage;
	WriteBinaryFile(imageFileName, image, imageSize);	// Write the resulting image
	if (FileSize(imageFileName) != imageSize)
		printf("Error writing to %s\n", imageFileName);
	delete[] image;
	if (verbose)
		printf("%s, size %s\n", imageFileName, FormatSize(imageSize));
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////
// Parse one option-value pair
//
int GetOptVal(IN OUT int& argc, IN OUT char**& argv, OUT char*& optValPtr)
{
	int optNum, n;

	optValPtr = *(++argv);								// Next option
	if ((*optValPtr == '-') || (*optValPtr == '/')) 	// Option here 
	{
		optValPtr++;	                          		// Skip option delimiter "-" or "/"
		for (optNum = 0; optNum < g_optCount; optNum++)	// Search in option table
		{
			n = _strnicmp(g_optTable[optNum], optValPtr, strlen(g_optTable[optNum]));
			if (n == 0)	// Found one
				break;
		}
		if (n)						// Not found any matching option
			return NOTFOUND;
		if (isupper(*g_optTable[optNum]))	// No value option?
			return optNum;					// Yes, no argument on this one
		//
		// Make optValPtr point at the option value regardless of spaces
		//
		optValPtr += strlen(g_optTable[optNum]);	// Skip option word
		if (*optValPtr == '\0')						// Space between option specifier & option value OR missing value
		{
			optValPtr = *(++argv);
			argc--;
			if (optValPtr == NULL)		// Last option, missing value
				return MISSINGVALUE;
		}
	} // if ((*argv == '-') || (*argv == '/'))
	else // No option given -> no optNum value
		return NOOPTION;
	return optNum;
}
///////////////////////////////////////////////////////////////////////////////////////////
// Read binary file, allocate buffer
//
uint32_t ReadBinaryFile(const char* binaryFile, uchar *&buf)
{
	basic_ifstream<uchar, char_traits<uchar>> ifs; // uchar ifstream
	uint32_t len = 0;

	if (binaryFile)
	{
		len = (uint32_t)FileSize(binaryFile);
		if (len)
		{
			buf = new uchar[len];
			ifs.open((char*)binaryFile, ifstream::binary | ifstream::in);
			ifs.read(buf, len);
			ifs.close();
		}
	}
	return len;
}
///////////////////////////////////////////////////////////////////////////////////////////
// Write binary file
//
uint32_t WriteBinaryFile(const char *binaryFile, const uchar *buf, uint32_t len)
{
	basic_ofstream<uchar, char_traits<uchar>> ofs; // uchar ofstream

	ofs.open(binaryFile, ofstream::out | ofstream::trunc | ofstream::binary);
	ofs.write(buf, len);
	ofs.close();
	return 0;
}
//////////////////////////////////////////////////////////
// Get file size
//
uint32_t FileSize(const char* fileName)
{
	if (fileName)
		if (filesystem::exists(fileName))
			return (uint32_t)filesystem::file_size(fileName);
	return 0;
}
//////////////////////////////////////////////////////////////////////////
// Format a size with appropriate suffix
//
char* FormatSize(const uint32_t bytes, char* result)
{
	static char buf[36];
	static const char* suffixes[] = { BYTES, KILOBYTES, MEGABYTES }; // , GIGABYTES, TERABYTES, PETABYTES, EXABYTES };
#define SUFFIXSZ (sizeof(suffixes) / sizeof(suffixes[0]))
#define DECIMALCOUNTINDEX 2 // Index position for 2 in "%.2f %s"; below
	char format[] = "%.2f %s";
	int suffix = 0;
	double count = (double)bytes;

	if (result == NULL)
		result = buf;		// Use static buffer
	while ((count >= ONEKILOBYTE) && (suffix < SUFFIXSZ))
	{
		suffix++;
		count /= ONEKILOBYTE;
	}
	if (suffix == 0)									// Bytes only, no decimal
		format[DECIMALCOUNTINDEX] = '0';				// Patch format specification to zero decimals
	sprintf(result, format, count, suffixes[suffix]);
	return result;
}
void help()
{
	printf(VERSION
		"\n"
		"Usage:\n"
		" -prog <file_name>               Program image file\n"
		" -fs <file_name>                 Filesystem image file name\n"
		" -image <file_name>              Resulting image\n"
		" [-offset <512, 1024 ... 32768>] Offset to FS start, default 1024\n"
		" [-fillchar <value>]             Fill character between program image and FS, default 0\n"
		" [-v]                            Verbose\n"
		" [-h]                            This help\n"
		"Usage example:\n"
		"ImageMerge -prog firmware.bin -fs littlefs.bin -image everything.bin -offset 512 -v\n"
	);
	exit(0);
}
