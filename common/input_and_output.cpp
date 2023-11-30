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

FILE* OpenOutputFile(const char* file_name, error_t* error)
{
    assert(file_name);
    assert(error);

    FILE* fp = fopen(file_name, "w");
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

const char* GetOutputFileName(const int argc, const char* argv[], error_t* error)
{
    assert(error);
    assert(argv);

    const char* file_name = nullptr;

    if (argc > 2)
        file_name = argv[2];
    else
    {
        PrintGreenText(stdout, "Enter output file name: \n", nullptr);
        file_name = GetDataFromLine(stdin, error);
    }

    if (file_name != nullptr)
        PrintGreenText(stdout, "OUTPUT FILE NAME: \"%s\"\n", file_name);

    return file_name;
}

//-----------------------------------------------------------------------------------------------------

void PrintPrankPhrase(FILE* fp)
{
    static const int   PHRASE_AMT = 11;
    static const char* PHRASES[] = {"After elementary simplifications, it is obvious that it is equal to",
                                    "For any listener of Lukashov, at the sight of this expression immediately comes to mind",
                                    "ARE YOU SURPRISED???? It is clear to the hedgehog that this is the same as",
                                    "Let's not bother with obvious proof that this is",
                                    "You are obliged to stop further studies at the university if it is not obvious to you that this is equal to",
                                    "I would justify this transition, but the article will be more useful if you do it yourself",
                                    "At that very lecture you missed, it was proved that this is equal to",
                                    "This explanation is available only for premium readers of this article (4862 8784 4592 1552)",
                                    "Zhirinovsky suggested to do this simplification",
                                    "djwfjawdfjifibjewfweiuofawubiefwuiwbufwbusdkdksadiwqioefw",
                                    "Are you really still reading this?"};

    int nmb = rand() % PHRASE_AMT;

    fprintf(fp, "%s ", PHRASES[nmb]);
}
