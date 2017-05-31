# reset variables specified by individual targets
LIBNAME:=
APPNAME:=
TESTNAME:=
LIBS:=

# reset compile options to defaults
LFLAGS:=-g -Wall -Werror -std=gnu++11 -lpthread
CFLAGS:=-D_DEBUG -O0 -g -pipe -Wall -Werror -std=gnu++11

# reset the make file names (nobody should change these, but this is a convenient place for defaults)
LIBMK:=make/Lib.mk
TESTMK:=make/Test.mk
APPMK:=make/App.mk
