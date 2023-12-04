#ifndef _TEX_H_
#define _TEX_H_

#include <stdio.h>

void StartTexFile(FILE* fp);
void EndTexFile(FILE* fp);

void PrintSection(FILE* fp, const char* string);
void PrintSubSection(FILE* fp, const char* string);

void PrintPrankPhrase(FILE* fp);

#endif
