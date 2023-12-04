#include <stdio.h>
#include <time.h>

#include "common/logs.h"
#include "common/errors.h"
#include "common/input_and_output.h"
#include "common/file_read.h"
#include "expression/expression.h"
#include "expression/expr_input_and_output.h"
#include "calculation.h"
#include "tex.h"

void UnrealTangent(const int argc, const char* argv[], FILE* out_stream, error_t* error);
void EasyX3Differentiation(const int argc, const char* argv[], FILE* out_stream, error_t* error);
void UnrealTaylor(const int argc, const char* argv[], FILE* out_stream, error_t* error);

int main(const int argc, const char* argv[])
{
    OpenLogFile(argv[0]);

    error_t error = {};

    const char* output_file = GetFileName(argc, argv, 1, "OUTPUT", &error);
    EXIT_IF_ERROR(&error);
    FILE* out_stream = OpenOutputFile(output_file, &error);
    EXIT_IF_ERROR(&error);
    StartTexFile(out_stream);

    UnrealTangent(argc, argv, out_stream, &error);
    EXIT_IF_EXPRESSION_ERROR(&error);

    UnrealTaylor(argc, argv, out_stream, &error);
    EXIT_IF_EXPRESSION_ERROR(&error);

    EasyX3Differentiation(argc, argv, out_stream, &error);
    EXIT_IF_EXPRESSION_ERROR(&error);

    EndTexFile(out_stream);
    fclose(out_stream);
}

void UnrealTangent(const int argc, const char* argv[], FILE* out_stream, error_t* error)
{
    expr_t  expr  = {};
    ExpressionCtor(&expr, error);

    const char* data_file = GetFileName(argc, argv, 2, "TANGENT FILE", error);
    BREAK_IF_ERROR(error);
    FILE* fp = OpenInputFile(data_file, error);
    BREAK_IF_ERROR(error);

    LinesStorage info = {};
    CreateTextStorage(&info, error, data_file);

    ExpressionInfixRead(&info, &expr, error);
    BREAK_IF_ERROR(error);

    PrintSection(out_stream, "Getting superhard tangent");

    expr_t* kasat = GetTangent(&expr, "x", 0.5, error, out_stream);
    BREAK_IF_ERROR(error);

    DrawTwoExprGraphics(out_stream, &expr, kasat);

    ExpressionDtor(&expr);
    fclose(fp);
}

void UnrealTaylor(const int argc, const char* argv[], FILE* out_stream, error_t* error)
{
    expr_t  expr  = {};
    ExpressionCtor(&expr, error);

    const char* data_file = GetFileName(argc, argv, 3, "TAYLOR FILE", error);
    BREAK_IF_ERROR(error);
    FILE* fp = OpenInputFile(data_file, error);
    BREAK_IF_ERROR(error);

    LinesStorage info = {};
    CreateTextStorage(&info, error, data_file);

    ExpressionInfixRead(&info, &expr, error);
    BREAK_IF_ERROR(error);

    PrintSection(out_stream, "Getting superhard Taylor series");

    expr_t* taylor = TaylorSeries(&expr, 5, "x", 3, error, out_stream);
    BREAK_IF_ERROR(error);

    PrintTaylorLatex(out_stream, taylor, 5, 3);

    DrawTwoExprGraphics(out_stream, &expr, taylor);

    expr_t* diff = GetExpressionsDifference(&expr, taylor, error, out_stream);
    BREAK_IF_ERROR(error);

    DrawExprGraphic(out_stream, diff);

    ExpressionDtor(&expr);
    ExpressionDtor(taylor);
    ExpressionDtor(diff);
    fclose(fp);
}

void EasyX3Differentiation(const int argc, const char* argv[], FILE* out_stream, error_t* error)
{
    expr_t  expr  = {};
    ExpressionCtor(&expr, error);

    const char* data_file = GetFileName(argc, argv, 4, "DIFFERENTIATION FILE", error);
    BREAK_IF_ERROR(error);
    FILE* fp = OpenInputFile(data_file, error);
    BREAK_IF_ERROR(error);

    LinesStorage info = {};
    CreateTextStorage(&info, error, data_file);

    ExpressionInfixRead(&info, &expr, error);
    BREAK_IF_ERROR(error);

    PrintSection(out_stream, "Calculating too easy differentiation");

    PrintExpression(out_stream, &expr);

    expr_t* d_expr = DifferentiateExpression(&expr, "x", error, out_stream);
    BREAK_IF_ERROR(error);

    expr_t* dd_expr = DifferentiateExpression(d_expr, "x", error, out_stream);
    BREAK_IF_ERROR(error);

    expr_t* ddd_expr = DifferentiateExpression(dd_expr, "x", error, out_stream);
    BREAK_IF_ERROR(error);

    ExpressionDtor(&expr);
    ExpressionDtor(d_expr);
    ExpressionDtor(dd_expr);
    ExpressionDtor(ddd_expr);
    fclose(fp);
}
