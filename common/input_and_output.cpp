#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>

#include "input_and_output.h"
#include "colorlib.h"

static void ReadLine(FILE* fp, char* buf);

//-----------------------------------------------------------------------------------------------------

void SkipSpaces(FILE* fp)
{
    char ch = 0;
    ch = getc(fp);

    while (isspace(ch))
        ch = getc(fp);

    ungetc(ch, fp);
}

//------------------------------------------------------------------------------------------------------------------

void ClearInput(FILE* fp)
{
    int ch = 0;
    while ((ch = fgetc(fp)) != '\n' && ch != EOF) {}
}

//-----------------------------------------------------------------------------------------------------

char* GetDataFromLine(FILE* fp, error_t* error)
{
    assert(error);

    char* line = (char*) calloc(MAX_STRING_LEN, sizeof(char));
    if (line == nullptr)
    {
        error->code = (int) ERRORS::ALLOCATE_MEMORY;
        return nullptr;
    }

    ReadLine(fp, line);

    return line;
}

//-----------------------------------------------------------------------------------------------------

static void ReadLine(FILE* fp, char* buf)
{
    assert(buf);

    for (size_t i = 0; i < MAX_STRING_LEN; i++)
    {
        char ch = getc(fp);

        if (ch == EOF)
            break;

        if (ch == '\n' || ch == '\0')
        {
            buf[i] = 0;
            break;
        }
        else
            buf[i] = ch;
    }
}

//-----------------------------------------------------------------------------------------------------

bool DoesLineHaveOtherSymbols(FILE* fp)
{
    int ch = 0;

    while ((ch = getc(fp)) != '\n' && ch != EOF)
    {
        if (!isspace(ch))
            {
                ClearInput(fp);
                return true;
            }
    }

    return false;
}

//-----------------------------------------------------------------------------------------------------

FILE* OpenInputFile(const char* file_name, error_t* error)
{
    assert(file_name);
    assert(error);

    FILE* fp = fopen(file_name, "r");
    if (!fp)
    {
        error->code = (int) ERRORS::OPEN_FILE;
        error->data = file_name;
    }

    return fp;
}

//-----------------------------------------------------------------------------------------------------

const char* GetInputFileName(const int argc, const char* argv[], error_t* error)
{
    assert(error);
    assert(argv);

    const char* file_name = nullptr;

    if (argc > 1)
        file_name = argv[1];
    else
    {
        PrintGreenText(stdout, "Enter input file name: \n", nullptr);
        file_name = GetDataFromLine(stdin, error);
    }

    if (file_name != nullptr)
        PrintGreenText(stdout, "INPUT FILE NAME: \"%s\"\n", file_name);

    return file_name;
}

//-----------------------------------------------------------------------------------------------------

void PrintPrankPhrase(FILE* fp)
{
    static const int   PHRASE_AMT = 5;
    static const char* PHRASES[] = {"Очевидно, что",
                                    "Несложно показать, что",
                                    "При виде формулы становится ясно, что",
                                    "Нет такого?",
                                    "Согл?"};

    int nmb = rand() % 5;

    fprintf(fp, "%s ", PHRASES[nmb]);
}
