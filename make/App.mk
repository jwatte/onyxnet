ifeq ($(APPNAME),)
$(error "You must specify APPNAME")
endif
ifneq ($(LIBNAME),)
$(error "LIBNAME cannot be speficied for an app")
endif
ifneq ($(TESTNAME),)
$(error "TESTNAME cannot be speficied for an app")
endif
CPP_app_$(APPNAME):=$(wildcard app/$(APPNAME)/*.cpp)
OBJ_app_$(APPNAME):=$(patsubst %.cpp,obj/%.o,$(CPP_app_$(APPNAME)))
CFLAGS_app_$(APPNAME):=$(CFLAGS)
LFLAGS_app_$(APPNAME):=$(LFLAGS)
LIBS_app_$(APPNAME):=$(LIBS)
TARGETS:=$(TARGETS) obj/$(APPNAME)

obj/$(APPNAME):	$(OBJ_app_$(APPNAME)) $(foreach l,$(LIBS),obj/lib$l.a)
	g++ $(LFLAGS_app_$(basename $@)) $(OBJ_app_$(basename $@)) $(foreach l,$(LIBS_app_$(basename $@)),-l$l) -Lobj -MMD -o $@

obj/app/$(APPNAME)/%.o:	app/$(APPNAME)/%.cpp make/Reset.mk make/App.mk
	-mkdir -p $(dir $@)
	g++ $(CFLAGS_app_$(patsubst obj/app/%/,%,$(dir $@))) -Ilib -MMD -c -o $@ $<

-include $(patsubst %.o,%.d,$(OBJ_app_$(APPNAME))
-include make/Reset.mk
