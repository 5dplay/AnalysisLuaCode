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
    Token res;
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
                            ReadLongString(nullptr, sep);
                            singleTokenBuffer.ResetBuffer();
                            break;
                        }
                    }
                    //short comment
                    while( !CurIsNewLine() && (curChar_ != EOZ) )
                        Next();
                    break;
                }
            case '[':
                {
                    int sep = CountSep();
                    if(sep>=0){
                        ReadLongString(&tt, sep);
                        tt.type = TK_STRING;
                        return tt;
                    }
                }
            case '=':
                {
                    Next();
                    if(CheckCurChar('='))
                        tt.type = TK_EQ;
                    else
                        tt.type = '=';
                    return tt;
                }
            case '<':
                {
                    Next();
                    if(CheckCurChar('='))
                        tt.type = TK_LE;
                    else if(CheckCurChar('<'))
                        tt.type = TK_SHL;
                    else
                        tt.type = '<';
                    return tt;
                }
            case '>':
                {
                    Next();
                    if(CheckCurChar('='))
                        tt.type = TK_GE;
                    else if(CheckCurChar('<'))
                        tt.type = TK_SHR;
                    else
                        tt.type = '>';
                    return tt;
                }
            case '/':
                {
                    Next();
                    if(CheckCurChar('/'))
                        tt.type = TK_IDIV;
                    else
                        tt.type = '/';
                    return tt;
                }
            case '~':
                {
                    Next();
                    if(CheckCurChar('='))
                        tt.type = TK_NE;
                    else
                        tt.type = '~';
                    return tt;
                }
            case ':':
                {
                    Next();
                    if(CheckCurChar(':'))
                        tt.type = TK_DBCOLON;
                    else
                        tt.type = ':';
                    return tt;
                }
            case '"': case '\'':
                {
                    ReadShortString(&tt, curChar_);
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

void Lex::ReadLongString(Token* tt, int sepNum)
{
    int beginLine = lineCounter_;
    SaveAndNext();//skip 2nd '['
    if(CurIsNewLine())
        IncreaseLineCounter();
    for(;;)
    {
        switch(curChar_)
        {
            case EOZ://unfinished string
            {
                cerr<<"unfinished string"<<std::endl;
                ERROR_EXIT;
            }
            case ']':
            {
                if(CountSep()==sepNum){
                    SaveAndNext();
                    goto EndLoop;
                }
                break;
            }
            case '\n': case '\r':
            {
                singleTokenBuffer.SaveChar('\n');
                IncreaseLineCounter();
                if(tt==nullptr)
                    singleTokenBuffer.ResetBuffer();
                break;
            }
            default:
            {
                if(tt!=nullptr)
                    SaveAndNext();
                else
                    Next();
            }
        }
        EndLoop:
        if(tt!=nullptr)
        {
            const char* buff = singleTokenBuffer.GetCurBuffer();
            int len = singleTokenBuffer.GetCurLen();
            std::string * newStr = new std::string(buff+2+sepNum, len-2*(2+sepNum));
            auto got = strTable.find(newStr);
            if(got == strTable.end()){
                tt->value.str = newStr;
                strTable.insert(newStr);
            }else{
                tt->value.str = *got;
                delete newStr;
            }
        }
    }
}

//use for operator, no need to save
bool Lex::CheckCurChar(char check)
{
    if(curChar_ == check){
        Next();
        return true;
    }
    return false;
}

/*
** Check whether current char is in set 'set' (with two chars) and
** saves it, use for number
*/
bool Lex::CheckCurChar(char lhs, char rhs)
{
    if(curChar_ == lhs || curChar_ == rhs){
        SaveAndNext();
        return true;
    }
    return false;
}

void Lex::ReadLongString(Token *tt, int del)
{
    SaveAndNext(); // keep delimiter (for error messages)
    while(curChar_!=del)
    {
        switch(curChar_)
    }
    const char* str = "asd
    asdsadadasd
    ";
    
}