#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>

#include "tex.h"

void StartTexFile(FILE* fp)
{
    /*fprintf(fp, "\\documentclass[12pt,a4paper]{extreport}\n"
                "\\usepackage[left=15mm,right=15mm, top=15mm,bottom=15mm,bindingoffset=0cm]{geometry}\n"
                "\\usepackage{indentfirst}\n"
                "\\usepackage[labelsep=period]{caption}\n"
                "\\usepackage{amssymb,amsmath,amsthm}\n"
                "\\usepackage{fontspec}\n"
                "\\usepackage{float}\n"
                "\\usepackage{polyglossia}\n"
                "\\setdefaultlanguage{english}\n"
                "\\setotherlanguage{english}\n"
                "\\usepackage{subcaption}\n"
                "\\usepackage{graphicx}\n"
                "\\graphicspath{img/}\n"
                "\\DeclareGraphicsExtensions{.pdf,.png,.jpg}\n"
                "\\usepackage{color}\n"
                "\\usepackage[normalem]{ulem}\n"
                "\\setlength{\\marginparwidth}{2cm}\n"
                "\\newcommand{\\fix}[2]{{\\textcolor{red}{\\uwave{#1}}\\todo[fancyline]{#2}}}\n"
                "\\newcommand{\\hl}[1]{{\\textcolor{red}{#1}}}\n"
                "\\newcommand{\\cmd}[1]{{\\ttfamily{\textbackslash #1}}}\n"
                "\\newcommand{\\vrb}[1]{\\PVerb{#1}}\n"
                "\\newcommand{\\vrbb}[1]{\\texttt{\\textbackslash}\\PVerb{#1}}\n"
                "\\usepackage[\n"
                    "draft = false,\n"
                    "unicode = true,\n"
                    "colorlinks = true,\n"
                    "allcolors = blue,\n"
                    "hyperfootnotes = true\n"
                "]{hyperref}\n"
                "\\renewcommand \\thesection{\\Roman{section}}\n"
                "\\renewcommand \\thesubsection{\\arabic{subsection}});\n");*/


    fprintf(fp, "\\documentclass[12pt,a4paper]{extreport}\n"
                "\\input{style}\n"
                "\\title{<<I fucking love science>>}\n"
                "\\author{Maklakov Artyom B05-332}\n"
                "\\usepackage[ sorting = none, style = science ]{biblatex}\n"
                "\\addbibresource{library.bib}\n"
                "\\begin{document}\n"
                "\\maketitle\n"
                "\\tableofcontents\n");
}

//-----------------------------------------------------------------------------------------------------

void EndTexFile(FILE* fp)
{
    fprintf(fp, "\\printbibliography\n"
                "\\end{document}\n");
}

//-----------------------------------------------------------------------------------------------------

void PrintSection(FILE* fp, const char* string)
{
    fprintf(fp, "\\section{%s}\n", string);
}

//-----------------------------------------------------------------------------------------------------

void PrintSubSection(FILE* fp, const char* string)
{
    fprintf(fp, "\\subsection{%s}\n", string);
}

//-----------------------------------------------------------------------------------------------------

void PrintPrankPhrase(FILE* fp)
{
    static const int   PHRASE_AMT = 12;
    static const char* PHRASES[] = {"After elementary simplifications, it is obvious that it is equal to",
                                    "For any listener of Lukashov, at the sight of this expression immediately comes to mind",
                                    "ARE YOU SURPRISED????\\cite{Dashkov} It is clear to the hedgehog that this is the same as",
                                    "Let's not bother with obvious proof that this is",
                                    "I would justify this transition, but the article will be more useful if you do it yourself",
                                    "At that very lecture you missed, it was proved that this is equal to",
                                    "A fitness trainer from Simferopol\\cite{SJ} threatens to beat you if you won't continue the transformation",
                                    "This explanation is available only for premium readers of this article (4862 8784 4592 1552)",
                                    "Zhirinovsky suggested \\cite{Zhirinovsky} to do this simplification",
                                    "Are you really still reading this?",
                                    "Some guy from asylum \\cite{Anton} told me that this is equal to",
                                    "Looks impressive. Still not as impressive as this dance from tiktok\\cite{Zolo}, so, we must made another transformation"};

    int nmb = rand() % PHRASE_AMT;

    fprintf(fp, "%s ", PHRASES[nmb]);
}

