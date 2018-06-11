/*
Copyright (C) 2018 Chunsheng Chen. All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

This is used to scan the srouce and produce TOKEN for parser
*/
#include "llex.h"
#include "lzio.h"
#include <iostream>

using std::cerr;

/* ORDER RESERVED */
static const char *const luaX_tokens [] = {
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "goto", "if",
    "in", "local", "nil", "not", "or", "repeat",
    "return", "then", "true", "until", "while",
    "//", "..", "...", "==", ">=", "<=", "~=",
    "<<", ">>", "::", "<eof>",
    "<number>", "<integer>", "<name>", "<string>"
};

Lex::Lex(char *filePath, char curChar)
    : io(filePath), curChar_(curChar), lineCounter_(1), lastLineRead_(1)
{
    nextToken.type = TK_EOS;
}

Token Lex::GetNextToken()
{
    singleTokenBuffer.ResetBuffer();
    for(;;){
        switch(curChar_){
            case '\n': case '\r'://line breaks
                {
                    IncreaseLineCounter();
                    break;
                }
            case ' ': case '\f': case '\t': case '\v'://spaces
                {
                    Next();
                    break;
                }
            case '-':// - or -- comment
                {
                    Next();
                    if(curChar_!='-') return '-';
                    Next();
                    if(curChar_ == '['){//long comment
                        int sep = CountSep();
                        singleTokenBuffer.ResetBuffer();//'count_sep' may dirty the buffer
                        if(sep>=0)//long comment
                        {
                            ReadLongString(sep);
                            singleTokenBuffer.ResetBuffer();
                            break;
                        }
                    }
                    //short comment
                    while( !CurIsNewLine() && (curChar_ != EOZ) )
                        Next();
                    break;
                }
        }
    }
}

// skip a sequence '[=*[' or ']=*]'; if sequence is well formed, return
// its number of '='s; otherwise, return a negative number (-1 iff there
// are no '='s after initial bracket)
// ccs: but i don't know why nees [=*[]=*]
int Lex::CountSep()
{
    int count = 0;
    int firstChar = curChar_;//must be '[' or ']'
    if( (firstChar != '[') && (firstChar != ']'))
    {
        cerr<<"invalid sequence"<<std::endl;
        ERROR_EXIT;
    }
    SaveAndNext();
    while(curChar_ == '=')
    {
        SaveAndNext();
        count++;
    }
    return curChar_ == firstChar ? count : (-count) - 1;
}

void Lex::IncreaseLineCounter()
{
    int prevChar = curChar_;
    if(!CurIsNewLine())
    {
        cerr<<"not new line character"<<std::endl;
        ERROR_EXIT;
    }
    Next();
    if(CurIsNewLine()&&(prevChar!=curChar_))// \n\r or \r\n regars as a newline, not two
        Next();
    if(++lineCounter_ >= MAX_INT)
    {
        cerr<<"too many lines"<<std::endl;
        ERROR_EXIT;
    }
}

void Lex::ReadLongString(int sepNum)
{
    int beginLine = lineCounter_;
}