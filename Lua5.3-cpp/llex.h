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

#ifndef LUACPP_LLEX_H_
#define LUACPP_LLEX_H_

#include "lzio.h"

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
// #include <cctype>
#include "luaconf.h"

#define FIRST_RESERVED_INDEX    257


/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER RESERVED"
*/
enum TokenType {
    /* terminal symbols denoted by reserved words */
    TK_AND = FIRST_RESERVED_INDEX, TK_BREAK,
    TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
    TK_GOTO, TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
    TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,
    /* other terminal symbols */
    TK_IDIV, TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE,
    TK_SHL, TK_SHR,
    TK_DBCOLON, TK_EOS,
    TK_FLT, TK_INT, TK_NAME, TK_STRING
};

//number of reserved keywords
#define NUM_OF_KEYWORD  (TK_WHILE-FIRST_RESERVED_INDEX+1)



union TokenValue{
    LuaInteger integer;
    LuaNumber realNumber;
    std::string *str;
};

struct Token{
    int type;
    TokenValue value;
};

namespace std{
    typedef string * pString;
    template<>
    struct hash<pString>
    {
        size_t operator() (const pString &str) const {
            return hash<string>()(*str);
        }
    };
    template<>
    struct equal_to<pString>
    {
        bool operator() (const pString &lhs, const pString &rhs) const{
            return *lhs == *rhs;
        }
    };
}

class Lex{

public:
    Lex(char*);
    void NextToken();
    inline Token GetCurToken() { return curToken_; }
    int LookAhead();
private:
    Token GetNextToken();
    inline void Next() { curChar_ = io_.GetNextChar(); }
    inline void SaveAndNext() { singleTokenBuffer_.SaveChar((char)curChar_); Next(); }
    inline bool CurIsNewLine() { return curChar_ == '\n' || curChar_ == '\r'; }
    bool CurIsSpace();
    bool CurIsDigit();
    bool CurIsAlNum();
    bool CurISHexDigit();
    bool CurIsAlpha();
    void IncreaseLineCounter();
    int CountSep();
    void ReadLongString(Token*, int);
    bool CheckCurChar(char);
    bool CheckCurChar(char, char);
    void ReadShortString(Token*, int);
    int ReadDec();
    int GetNextHex();
    int ReadHex();
    int IsKeyWord(std::string&);
    void ReadUnicodeSaveAsUTF8();
    void ReadNumber(Token*);
    void LexError(const char*);
    void LexError(const char*, int);
    std::string* CreateString(const char*, int);
    static int ToHex(char);
    std::string Name2Str(int);// float/int/string/id to string for DEBUG
    
    int curChar_;        //current character in read
    int lineCounter_;    //input line counter
    int lastLineRead_;   //line of last token 'consumed'
    Token curToken_;     //current token
    Token nextToken_;    //look ahead token
    Buffer singleTokenBuffer_;//buffer for single token str
    IOReader io_;//input stream
    std::string source_; //current source file name
    // std::string *envn;  //environment variable name
    std::unordered_set<std::string*> strTable_;
    std::unordered_map<std::string, int> keyWordTable_;
};


void DumpToken(Token&);



#endif //LUACPP_LLEX_H_