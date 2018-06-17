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

Buffered streams
*/

#ifndef LUACPP_LZIO_H_
#define LUACPP_LZIO_H_

#include "luaconf.h"
#include <istream>

#define EOZ -1

class Buffer{
public://constant
    static const int BUFFER_MAX_SIZE = MAX_INT;
    static const int BUFFER_MIN_SIZE = 32;
public:
    explicit Buffer(int);
    inline const char * GetCurBuffer() { return buff_; }
    void SaveChar(char);
    inline void ResetBuffer() { curLen_ = 0; }
    inline int GetCurLen() { return curLen_; }
    inline void RemoveBuffer(int n) { curLen_ -= n; }
private:
    char *buff_;
    int capacity_;
    int curLen_;
};



class IOReader{
public:
    explicit IOReader(char*);
    ~IOReader();
    int GetNextChar();
    inline const char *GetFilePath() { return filePath_; }
    inline int GetCurLine() { return curLine_; }
private:
    char *filePath_;// nullptr for stdin, otherwise file path
    int curLine_;
    std::istream* inputStream_;
};

#endif //LUACPP_LZIO_H_