ifeq ($(LIBNAME),)
$(error "You must specify LIBNAME")
endif
ifneq ($(TESTNAME),)
$(error "TESTNAME cannot be speficied for a lib")
endif
ifneq ($(APPNAME),)
$(error "APPNAME cannot be speficied for a lib")
endif
ifneq ($(LIBS),)
$(error "LIBS cannot be speficied for a lib")
endif
CPP_lib_$(LIBNAME):=$(wildcard lib/$(LIBNAME)/*.cpp)
OBJ_lib_$(LIBNAME):=$(patsubst %.cpp,obj/%.o,$(CPP_lib_$(LIBNAME)))
CFLAGS_lib_$(LIBNAME):=$(CFLAGS)
TARGETS:=$(TARGETS) obj/lib$(LIBNAME).a

obj/lib$(LIBNAME).a:	$(OBJ_lib_$(LIBNAME))
	rm -f $@
	ar crs $@ $(OBJ_lib_$(patsubst obj/lib%.a,%,$@))

obj/lib/$(LIBNAME)/%.o:	lib/$(LIBNAME)/%.cpp make/Reset.mk make/Lib.mk
	-mkdir -p $(dir $@)
	g++ $(CFLAGS_lib_$(basename $(dir $@))) -Ilib -MMD -c -o $@ $<

-include $(patsubst %.o,%.d,$(OBJ_lib_$(LIBNAME)))
-include make/Reset.mk
