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
#include <iostream>
#include <cctype>
#include <string>
#include "lzio.h"

using std::cerr;
using std::cout;
using std::endl;

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

const char *Token2Str(int);
char alphaStr[2];

void DumpToken(Token &tt)
{
    if(tt.type<FIRST_RESERVED_INDEX){//alpha
        cout<<char(tt.type)<<endl;
        return ;
    }
    cout<<luaX_tokens[tt.type-FIRST_RESERVED_INDEX];
    switch(tt.type){
        case TK_FLT:
            cout<<": "<<tt.value.realNumber<<endl;
            break;
        case TK_INT:
            cout<<": "<<tt.value.integer<<endl;
            break;
        case TK_NAME:
        case TK_STRING:
            cout<<": "<<*(tt.value.str)<<endl;
            break;
        default:
            //nothing
            cout<<endl;
    }

}

Lex::Lex(char *filePath)
    : io_(filePath), lineCounter_(1), lastLineRead_(1), singleTokenBuffer_(Buffer::BUFFER_MIN_SIZE)
{
    nextToken_.type = TK_EOS;
    if(filePath==nullptr)
        source_ = "<stdin>";
    else
        source_ = std::string(filePath);
    for(int i = 0; i < NUM_OF_KEYWORD; i++)
        keyWordTable_[luaX_tokens[i]] = FIRST_RESERVED_INDEX + i;
    curChar_ = io_.GetNextChar();
}

Token Lex::GetNextToken()
{
    Token tt;
    singleTokenBuffer_.ResetBuffer();
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
                    if(curChar_!='-'){ tt.type = '-'; return tt;}
                    Next();
                    if(curChar_ == '['){//long comment
                        int sep = CountSep();
                        singleTokenBuffer_.ResetBuffer();//'count_sep' may dirty the buffer
                        if(sep>=0)//long comment
                        {
                            ReadLongString(nullptr, sep);
                            singleTokenBuffer_.ResetBuffer();
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
                    }else if(sep != -1)
                        LexError("invalid long string delimiter");
                    tt.type = '[';
                    return tt;
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
                    tt.type = TK_STRING;
                    return tt;
                }
            case '.':
                {
                    SaveAndNext();
                    if(CheckCurChar('.')){
                        if(CheckCurChar('.')){
                            tt.type = TK_DOTS; // ...
                            return tt;
                        }else{
                            tt.type = TK_CONCAT; // ..
                            return tt;
                        }
                    }
                    else if(!CurIsDigit()){
                        tt.type = '.';
                        return tt;
                    }else{
                        ReadNumber(&tt);
                        return tt;
                    }
                }
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':{
                ReadNumber(&tt);
                return tt;
            }
            case EOZ:
                {
                    tt.type = TK_EOS;
                    return tt;
                }
            default:
                {
                    if(CurIsAlpha()){//id or keyword
                        do{
                            SaveAndNext();
                        }while(CurIsAlNum());
                        const char *buff = singleTokenBuffer_.GetCurBuffer();
                        int len = singleTokenBuffer_.GetCurLen();
                        std::string* str = CreateString(buff, len);
                        int type = IsKeyWord(*str);
                        tt.type = type;
                        tt.value.str = str;
                        return tt;
                    }else{
                        tt.type = curChar_;
                        Next();
                        return tt;
                    }
                }
            
        }
    }
}

// skip a sequence '[=*[' or ']=*]'; if sequence is well formed, return
// its number of '='s; otherwise, return a negative number (-1 iff there
// are no '='s after initial bracket)
int Lex::CountSep()
{
    int count = 0;
    int firstChar = curChar_;//must be '[' or ']'
    if( (firstChar != '[') && (firstChar != ']'))
    {
        LexError("invalid sequence");
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
        LexError("not new line character");
    }
    Next();
    if(CurIsNewLine()&&(prevChar!=curChar_))// \n\r or \r\n regars as a newline, not two
        Next();
    if(++lineCounter_ >= MAX_INT)
    {
        LexError("too many lines");
    }
}
void Lex::ReadNumber(Token* tt)
{
    char first = curChar_;
    const char *expo = "Ee";
    SaveAndNext();
    if( (first == '0') && (CheckCurChar('x', 'X')) )
        expo = "Pp";
    bool isFloat = false;
    for(;;){
        if(CheckCurChar(expo[0], expo[1]))
            CheckCurChar('-', '+');
        if(CurISHexDigit())
            SaveAndNext();
        else if(CheckCurChar('.'))
            isFloat = true, SaveAndNext();
        else break;
    }
    singleTokenBuffer_.SaveChar('\0');
    const char* buff = singleTokenBuffer_.GetCurBuffer();
    std::string str(buff);
    if(isFloat){
        tt->type = TK_FLT;
        tt->value.realNumber = std::stod(str);
    }else{
        tt->type = TK_INT;
        tt->value.integer = std::stol(str, nullptr);
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
                std::string msg = "unfinished long ";
                if(tt==nullptr)
                    msg += "comment ";
                else
                    msg += "string ";
                msg += "(starting at line ";
                msg += std::to_string(beginLine);
                msg += ')';
                LexError(msg.c_str(), TK_EOS);
                break;
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
                singleTokenBuffer_.SaveChar('\n');
                IncreaseLineCounter();
                if(tt==nullptr)
                    singleTokenBuffer_.ResetBuffer();
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
            const char* buff = singleTokenBuffer_.GetCurBuffer();
            int len = singleTokenBuffer_.GetCurLen();
            tt->value.str = CreateString(buff+2+sepNum, len-2*(2+sepNum));
        }
    }
}

std::string* Lex::CreateString(const char* lhs, int rhs)
{
    std::string * newStr = new std::string(lhs, rhs);
    auto got = strTable_.find(newStr);
    if(got == strTable_.end()){
        strTable_.insert(newStr);
        return newStr;
    }else{
        delete newStr;
        return *got;
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

int Lex::IsKeyWord(std::string& str)
{
    auto got = keyWordTable_.find(str);
    if(got == keyWordTable_.end())
        return TK_NAME;
    else
        return got->second;
}

void Lex::ReadShortString(Token *tt, int del)
{
    SaveAndNext(); // keep delimiter (for error messages)
    while(curChar_!=del)
    {
        switch(curChar_)
        {
            case EOZ:
                LexError("unfinished string", TK_EOS);
                break;
            case '\n': case '\r':
                LexError("unfinished string", TK_STRING);
                break;
            case '\\'://escape sequences
                {
                    char ch; //final character to be saved
                    SaveAndNext();//keep '\\' for error messages
                    bool haveToRead = false, haveToSave = false;
                    switch(curChar_)
                    {
                        case 'a': ch = '\a'; haveToRead = haveToSave = true; break;
                        case 'b': ch = '\b'; haveToRead = haveToSave = true; break;
                        case 'f': ch = '\f'; haveToRead = haveToSave = true; break;
                        case 'n': ch = '\n'; haveToRead = haveToSave = true; break;
                        case 'r': ch = '\r'; haveToRead = haveToSave = true; break;
                        case 't': ch = '\t'; haveToRead = haveToSave = true; break;
                        case 'v': ch = '\v'; haveToRead = haveToSave = true; break;
                        case 'x': ch = ReadHex(); haveToRead = haveToSave = true; break;
                        case 'u': ReadUnicodeSaveAsUTF8(); break;
                        case '\n': case '\r': IncreaseLineCounter(); ch = '\n'; haveToSave = true; break;
                        case '\\': case '\"': case '\'':
                            ch = curChar_; haveToRead = haveToSave = true; break;
                        case EOZ: break;
                        case 'z': //zap following span of spaces
                            singleTokenBuffer_.RemoveBuffer(1); //remove '\\'
                            Next();
                            while(CurIsSpace()){
                                if(CurIsNewLine())
                                    IncreaseLineCounter();
                                else
                                    Next();
                            }
                            break;
                        default:
                            {
                                if(!CurIsDigit()){
                                    LexError("invalid escape sequence");
                                }else{
                                    ch = ReadDec();
                                }
                            }
                    }
                    if(haveToRead)
                        Next();
                    if(haveToSave){
                        singleTokenBuffer_.RemoveBuffer(1);
                        singleTokenBuffer_.SaveChar(ch);
                    }
                    break;
                }
            default:
                SaveAndNext();
        }
    }
    SaveAndNext();
    tt->value.str = CreateString(singleTokenBuffer_.GetCurBuffer() + 1, singleTokenBuffer_.GetCurLen() - 2);
}

bool Lex::CurIsSpace()
{
    return isspace(curChar_);
}
bool Lex::CurIsAlNum()
{
    return isalnum(curChar_) || curChar_ == '_' ;
}
bool Lex::CurIsAlpha()
{
    return isalpha(curChar_) || curChar_ == '_';
}
bool Lex::CurIsDigit()
{
    return isdigit(curChar_);
}
bool Lex::CurISHexDigit()
{
    return isxdigit(curChar_);
}
int Lex::ReadDec()
{
    int r = 0;
    int i = 0;
    for( ;i<3 && CurIsDigit(); i++){ //read up to 3 digits
        r = 10*r + curChar_ - '0';
        SaveAndNext();
    }
    singleTokenBuffer_.RemoveBuffer(i);
    return r;
}
int Lex::GetNextHex()
{
    SaveAndNext();//save 'x'
    if(CurISHexDigit())
        return ToHex(curChar_);
    LexError("hexadecimal digit expected");
    return -1;//try exception!! 
}
int Lex::ReadHex()
{
    int r = GetNextHex();
    r = (r<<4) + GetNextHex();
    singleTokenBuffer_.RemoveBuffer(2);
    return r;
}
int Lex::ToHex(char ch)
{
    if(isdigit(ch))
        return ch -'0';
    else
        return 10 + (ch | 0x20) - 'a';
}

void Lex::LexError(const char *msg)
{
    cerr<<source_<<"::"<<lineCounter_<<":"<<msg<<endl;
}

void Lex::LexError(const char *msg, int token)
{
    cerr<<source_<<"::"<<lineCounter_<<":"<<msg<<" near "<<Name2Str(token)<<endl;
}

const char *Token2Str(int token)
{
    if(token < FIRST_RESERVED_INDEX){
        alphaStr[0] = token;
        alphaStr[1] = '\0';
        return alphaStr;
    }
    return luaX_tokens[token-FIRST_RESERVED_INDEX];
}

std::string Lex::Name2Str(int token)
{
    switch(token)
    {
        case TK_NAME: case TK_STRING:
        case TK_FLT: case TK_INT:
            singleTokenBuffer_.SaveChar('\0');
            return std::string(singleTokenBuffer_.GetCurBuffer());
        default:
            return std::string(Token2Str(token));
    }
}

void Lex::ReadUnicodeSaveAsUTF8()
{
    const int UTF8_BUF_SIZE = 8;
    char byte[UTF8_BUF_SIZE] = {0};
    int count = 1;//number of bytes put in buffer (backwards)
    int i = 4; //chars to be removed: '\', 'u', '{', and first digit
    SaveAndNext();
    if(!CheckCurChar('{')){
        LexError("missing '{'");
        //error handler
    }
    unsigned int r = GetNextHex();//read unicode
    while((SaveAndNext(), CurISHexDigit())){
        i++;
        r = (r<<4) + ToHex(curChar_);
        if(r > 0x10FFFF){
            LexError("unicode value too large");
            //error handler
        }
    }
    if(!CheckCurChar('}')){
        LexError("missing '}'");
        //error handler
    }
    Next();//skip '}'
    singleTokenBuffer_.RemoveBuffer(i);
    if(r < 0x80)
        byte[UTF8_BUF_SIZE-1] = (char)r;
    else{
        unsigned int mfb = 0x3f; //maximum that fits in first byte
        do{
            byte[UTF8_BUF_SIZE - (count++)] = (char) (0x80 | (r & 0x3f));
            r >>= 6;// remove added bits 
            mfb >>= 1;//now there is one less bit available in first byte
        }while(r > mfb);//still needs continuation byte?
        byte[UTF8_BUF_SIZE - count] = (char) ((~mfb << 1) | r);
    }
    while(count > 0){
        singleTokenBuffer_.SaveChar(byte[UTF8_BUF_SIZE-count]);
    }
}

void Lex::NextToken()
{
    lastLineRead_ = lineCounter_;
    if(nextToken_.type != TK_EOS){
        curToken_ = nextToken_;
        nextToken_.type = TK_EOS;
    }else{
        curToken_ = GetNextToken();
    }
}

int Lex::LookAhead()
{
    nextToken_ = GetNextToken();
    return nextToken_.type;
}