ifeq ($(TESTNAME),)
$(error "You must specify TESTNAME")
endif
ifneq ($(LIBNAME),)
$(error "LIBNAME cannot be speficied for a test")
endif
ifneq ($(APPNAME),)
$(error "APPNAME cannot be speficied for a test")
endif
CPP_test_$(TESTNAME):=$(wildcard test/$(TESTNAME)/*.cpp)
OBJ_test_$(TESTNAME):=$(patsubst %.cpp,obj/%.o,$(CPP_test_$(TESTNAME)))
CFLAGS_test_$(TESTNAME):=$(CFLAGS)
LFLAGS_test_$(TESTNAME):=$(LFLAGS)
LIBS_test_$(TESTNAME):=$(LIBS)
TARGETS:=$(TARGETS) obj/test_$(TESTNAME)

obj/test_$(TESTNAME):	$(OBJ_test_$(TESTNAME)) $(foreach l,$(LIBS),obj/lib$l.a)
	g++ $(LFLAGS_test_$(patsubst obj/test_%,%,$@)) $(OBJ_test_$(patsubst obj/test_%,%,$@)) $(foreach l,$(LIBS_test_$(patsubst obj/test_%,%,$@)),-l$l) -Lobj -MMD -o $@

obj/test/$(TESTNAME)/%.o:	test/$(TESTNAME)/%.cpp make/Reset.mk make/Test.mk
	-mkdir -p $(dir $@)
	echo "LIBS_test_$(patsubst obj/test/%/,%,$(dir $@)) is" $(LIBS_test_$(patsubst obj/test/%/,%,$(dir $@)))
	g++ $(CFLAGS_test_$(patsubst obj/test/%/,%,$(dir $@))) -Ilib -MMD -c -o $@ $<

-include $(patsubst %.o,%.d,$(OBJ_test_$(TESTNAME)))
-include make/Reset.mk
