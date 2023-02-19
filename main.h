#pragma once
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif

enum
{
	MISSINGVALUE = -1,	// Option value missing
	NOOPTION = -2,		// No option given, just the value -> pgm filename
	NOTFOUND = -3		// -option not found
};
int GetOptVal(IN OUT int& argc, IN OUT char**& argv, OUT char*& optValPtr);
uint32_t ReadBinaryFile(const char* binaryFile, uchar*& buf);
uint32_t  WriteBinaryFile(const char* binaryFile, const uchar* buf, uint32_t len);
uint32_t FileSize(const char* fileName);
char* FormatSize(const uint32_t bytes, char* result = NULL);
void help();
