# reset variables specified by individual targets
LIBNAME:=
APPNAME:=
TESTNAME:=
LIBS:=

# reset compile options to defaults
LFLAGS:=-g -Wall -Werror
CFLAGS:=-O1 -g -pipe -Wall -Werror

# reset the make file names (nobody should change these, but this is a convenient place for defaults)
LIBMK:=make/Lib.mk
TESTMK:=make/Test.mk
APPMK:=make/App.mk
