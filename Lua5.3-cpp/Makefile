CC = g++ 
CPPFLAGS = -std=c++11 -g
CORE_O = llex.o lzio.o
LIB_O =
BASE_O = $(CORE_O) $(LIB_O)

LUA_T = lua
LUA_O = lua.o

ALL_O = $(BASE_O) $(LUA_O)
ALL_T = $(LUA_T)


all: $(LUA_T)

o: $(ALL_O)

$(LUA_T): $(LUA_O) $(BASE_O)
	$(CC) $(CPPFLAGS) -o $@ $(LUA_O) $(BASE_O)

lzio.o: lzio.cpp lzio.h luaconf.h
llex.o: llex.cpp llex.h lzio.h 