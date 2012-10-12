##
# Wineing Makefile
#
# Targets: debug [default], test, release
# The targets are briefly described below.
#
# The makefile is structured in sections. A section heading is
# prefixed with ###. The sections are:
#
# - Misc information: Contains information on many of the topic
#   encountered when writing Makefiles and on building, testing, and
#   debuggind C/C++ programs in general. It is not necessary to read
#   this section.
#
# - Basic settings: Include directory settings, source files, and
#   other globally relevant configuration options.
#
# - Toolchain settings: Compiler & linker flags, include paths,
#   ... Settings here depend on either the environment or the 'Basic
#   settings'. A user is not required to make changes in this section.
#
# - Build rules: Definition of build rules such as build targets,
#   dependencies, packaging, testing, etc.
#
# - Unused (exists for historical reasons only): a lot of crap from
#   previous versions.
#
#
# To successfully run the build the following dependencies must be met:
# - protobuf: for the definition of client interfaces
# - zmq: the client connection layer
# - check: for testing
# - wine: as a compatibility layer for nxcore (windows)
# - [ack]: only if 'todo' target is used
#
# On arch linux its a matter of running the command:
# $ yaourt check zmq protobuf wine ack
#
# Targets
#
# - debug [default] Build for debug. -ggdb and -DDEBUG are added to
#   gcc invocation. -DDEBUG will make DEBUG available as compiler
#   macro. Use it e.g. with #ifdef DEBUG ... to check if building in
#   debug mode. debug target is also required for valgrind. Wine has
#   to be compiled manually for valgrind to work, see
#   http://wiki.winehq.org/Wine_and_Valgrind?action=show&redirect=Valgrind
#
# - release Build for release. Invokes GCC suite without -g and
#   -DDEBUG flag.
#
# - test Currently under development.
#
# - todo Prints all the tu
#

### Misc information
#
# With automake/autoconf we could easily make the build multiplatform
# compatible. This is probably too much work as we target one platform
# only. Also mastering autotools is an endavour on its own.
#
# Make manual: http://www.gnu.org/software/make/manual/make.html
#
# Book on autoconf tools (not so bad):
# http://sources.redhat.com/autobook/
#
# http://www.winehq.org/docs/winedev-guide/x2800
#
# To list make database 'make -pn'. This prints a bunch of useful
# information.
#
# Recursive (nested) vs. non recursive Makefiles:
# Pros:
# -
#
# Cons:
# - http://stackoverflow.com/questions/559216/what-is-your-experience-with-non-recursive-make
#
# Implementation of non recursive Makefiles:
# - http://evbergen.home.xs4all.nl/nonrecursive-make.html (good)
#
# This seems to be relevant for big projects. Don't know if we really
# require this complexity.
#
# To print gcc search path invoke 'gcc -v'. See
# http://gcc.gnu.org/ml/gcc-help/2007-09/msg00206.html
#
# TODO: enhance make file to include profiling target to analyse for
# memory leaks and the like using valgrind...
#
# quick start: http://valgrind.org/docs/manual/QuickStart.html
# manual: http://valgrind.org/docs/manual/manual.html
#
# TODO: add target to display TODOs in project...
#
# TODO: Use automake/autoconf tools to check for libraries? A good
# introduction into GNU Build Tools:
# http://developers.sun.com/solaris/articles/gnu.html
#
# DONE: Turn on gcc's optimization options i.e. -O3.
# http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Optimize-Options.html

### Basic settings

# Either LOG_ALL, LOG_DEBUG, LOG_INFO, LOG_ERROR, LOG_NONE
LOG_LEVEL             = LOG_ALL

## Directories
VERSION               = 0.0.1
SRCDIR                = src/main/c
BINDIR                = target/wineing-$(VERSION)
RESDIR                = src/main/resources
LIBDIR                = ext/c
GENSRCDIR             = $(RESDIR)/protobuf
GENDIR                = src/gen/c

TESTSRCDIR            = src/test/c
TESTBINDIR            = target/wineing-$(VERSION)-test
TESTRESDIR            = src/test/resources

# Determine cache-line size. lazy.h expects this.
CACHE_LINE_SIZE       = $(shell cat /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size)

## Files
# *.proto files must be declared here in $(GENS). It is
# expected that they're placed in $(GENSRCDIR). The build will
# otherwise fail!
#
# TODO: If a *.proto file changes 'make all' will not recognize the
# file has changed. It is required to manually invoke 'make clean &&
# make all'.
EXES                  = $(exe_WI_NAME)
TEST_EXES             = $(exe_WI_TEST_NAME)
GENS                  = $(GENSRCDIR)/WineingCtrlProto.proto \
                        $(GENSRCDIR)/WineingMarketDataProto.proto

## Executables (targets)
# wineing.exe
exe_WI_NAME            = $(BINDIR)/wineing.exe
exe_WI_CC_SRCS         =
exe_WI_CXX_SRCS        = $(SRCDIR)/impl/wine/nx/nxinf.win.cc \
                         $(SRCDIR)/impl/wine/nx/nxtape.win.cc \
                         $(SRCDIR)/impl/wine/core/wineing.win.cc \
                         $(SRCDIR)/impl/all/net/chan.cc \
                         $(SRCDIR)/impl/all/conc/conc.cc \
                         $(SRCDIR)/main.win.cc
exe_WI_LDFLAGS         =
exe_WI_WIN_LDFLAGS     = -mconsole \
                         $(exe_WI_LDFLAGS)
exe_WI_LIBRARY_PATH    =
exe_WI_LIBRARIES       = -luuid \
                         -lzmq \
                         -lprotobuf \
                         -lpthread
exe_WI_DLL_PATH        =
exe_WI_DLLS            = #-lodbc32 \
                         -lole32 \
                         -lwinspool \
                         -lodbccp32
exe_WI_OBJS            = $(subst .c,.c.o,$(exe_WI_CC_SRCS)) \
                         $(subst .cc,.cc.o,$(exe_WI_CXX_SRCS)) \
                         $(gen_PB_OBJS)

# wineing.test
exe_WI_TEST_NAME       = $(TESTBINDIR)/wineing.test
exe_WI_TEST_CC_SRCS    =
exe_WI_TEST_CXX_SRCS   = $(SRCDIR)/impl/all/net/chan.cc \
                         $(SRCDIR)/impl/all/conc/conc.cc \
                         $(SRCDIR)/impl/all/core/wineing.cc \
                         $(SRCDIR)/impl/linux/nx/nxinf.cc \
                         $(SRCDIR)/impl/linux/nx/nxtape.cc \
                         $(TESTSRCDIR)/main_test.cc

exe_WI_TEST_OBJS       = $(subst .c,.c.o,$(exe_WI_TEST_CC_SRCS)) \
                         $(subst .cc,.cc.o,$(exe_WI_TEST_CXX_SRCS)) \
                         $(gen_PB_OBJS)


## Protobuf
# Don't touch!
gen_PB_SRCS    = $(patsubst $(GENSRCDIR)/%.proto,\
                            $(GENDIR)/%.pb.cc,\
                            $(GENS))
gen_PB_OBJS    = $(patsubst $(GENSRCDIR)/%.proto,\
                            $(GENDIR)/%.pb.cc.o,\
                            $(GENS))
# <<< end 'Basic settings'


### Toolchain settings
# There's usually no changes needed below this line.

# Exports DEBUG compiler macro
ALL_OPTIONS           = -DLOG_LEVEL=$(LOG_LEVEL) \
			-DCACHE_LINE_SIZE=$(CACHE_LINE_SIZE)
DEBUG                 = -ggdb -DDEBUG
OPTIONS               = -O3
TEST_OPTIONS          = -lcheck -ftest-coverage
TEST_INCLUDE_PATH     = -I$(GENDIR) \
                        -I$(SRCDIR)/inc \
                        -I$(TESTSRCDIR)/inc
LIBRARY_PATH          = # -DMYSYMBOL=VAL
LIBRARIES             = # -lzmq
DLL_PATH              =
DLL_IMPORTS           = # -lole32

DEFINES               =
INCLUDE_PATH          = -I$(GENDIR) \
                        -I$(SRCDIR)/inc \
                        -I$(RESDIR)/NxCoreAPI
CFLAGS                = -Wall
CXXFLAGS              = -Wall
LFLAGS                = -Wall

# See http://www.delorie.com/howto/cygwin/mno-cygwin-howto.html
# for what no-cygwin does. Its an incompatibility with -lpthread
CEXTRA               = #-mno-cygwin
CXXEXTRA             = #-mno-cygwin
#RCEXTRA               =


# Concat includes
ALL_INCL              = $(DEFINES) \
                        $(OPTIONS) \
                        $(ALL_OPTIONS) \
                        $(INCLUDE_PATH)

ALL_TEST_INCL         = $(DEFINES) \
                        $(ALL_OPTIONS) \
                        $(TEST_OPTIONS) \
                        $(TEST_INCLUDE_PATH)

# Concat library paths
ALL_LIBS              = $(LIBRARY_PATH) \
                        $(LIBRARIES) \
                        $(DLL_PATH) \
                        $(DLL_IMPORTS)

## Tools
CC = gcc
CXX = g++
WCC = winegcc
WCXX = wineg++
#WRC = wrc
PROTOC = protoc
#AR = ar
# <<< end 'Tool chain settings'


### Build rules
# Useful inforamtion on implicit rules/variables and the like
# http://www.gnu.org/savannah-checkouts/gnu/make/manual/html_node/Implicit-Variables.html#Implicit-Variables
.PHONY: release clean

# In case debug target is invoked, lazily prepend debug arguments to
# gcc
debug: CC += $(DUBUG)
debug: CXX += $(DEBUG)
debug: WCC += $(DEBUG)
debug: WCXX += $(DEBUG)
debug: OPTIONS = -O0
debug: release run-tests

# Shall we define our own SUFFIXES? Would result in shorter
# target/prerequisite rules. Personally I prefer being explicit.
#
#.SUFFIXES:
#.SUFFIXES: .cpp .c .o .h

# Tell make not to delete intermediary files. See
# http://www.gnu.org/software/make/manual/make.html#index-preserving-with-_0040code_007b_002eSECONDARY_007d-230
#
#.SECONDARY: %.pb.cc
#.PRECIOUS: %.pb.cc

release: dirs $(EXES) libs

test: dirs $(TEST_EXES)

todo:
	@ack TODO **

run-tests: test
	cd $(TESTBINDIR)
	./$(exe_WI_TEST_NAME)

cache_line:
	@echo "Setting CACHE_LINE_SIZE to $(CACHE_LINE_SIZE)"


$(exe_WI_TEST_NAME): gen cache_line $(exe_WI_TEST_OBJS)
	$(CXX) $(ALL_LIBS) $(ALL_TEST_INCL) $(exe_WI_LDFLAGS) $(exe_WI_TEST_OBJS) $(exe_WI_LIBRARY_PATH) $(exe_WI_LIBRARIES) -o $@

$(exe_WI_NAME): gen cache_line $(exe_WI_OBJS)
	$(WCXX) $(ALL_LIBS) $(ALL_INCL) $(exe_WI_WIN_LDFLAGS) $(exe_WI_OBJS) $(exe_WI_DLL_PATH) $(exe_WI_DLLS) $(exe_WI_LIBRARY_PATH) $(exe_WI_LIBRARIES) -o $@ 

gen: $(gen_PB_SRCS)

libs:
	mkdir -p $(BINDIR)/lib
	cp $(LIBDIR)/* $(BINDIR)/lib

dirs:
	mkdir -p $(BINDIR)
	mkdir -p $(TESTBINDIR)
	mkdir -p $(GENDIR)

# Default c/cc targets
%.c.o: %.c
	$(CC) $(CFLAGS) $(CEXTRA) $(ALL_INCL) -o $@ -c $<

%.cc.o: %.cc
	$(CXX) $(CXXFLAGS) $(CXXEXTRA) $(ALL_INCL) -fPIC -o $@ -c $<

# We introduce windows specific targets (by convention files ending in
# .win.(c|cc) will be compiled with winegcc or wineg++ respectively.
# Following this convention circumvents writing specific targets for
# each file that needs to be compiled withe either winegcc or wineg++.
%.win.c.o: %.win.c
	$(WCC) $(CFLAGS) $(CEXTRA) $(ALL_INCL) -o $@ -c $<

%.win.cc.o: %.win.cc
	$(WCXX) $(CXXFLAGS) $(CXXEXTRA) $(ALL_INCL) -o $@ -c $<

# It is not possible to use wildcards (automatic variables in make
# jargon) but in the recipe. They're illegal in the target and
# prerequisite definition. See
# http://lists.gnu.org/archive/html/help-make/2012-03/msg00052.html
#
# This is where we need to change things so that 'make' recognizes a
# *.proto file has changed and automatically triggers a full rebuild.
# The problem is easily solved with explicit targets or by placing the
# *.proto files in the same directory as the generation. I'm against
# using explicit targets because it requires manually adding the
# target whenever a) a new *.proto file is added or b) a proto file is
# renamed. I'm also against putting the *.proto files in the same
# directory as the generation. The cleaner solution is to invoke 'make
# clean && make all'.
$(GENDIR)/%.cc.o: $(GENDIR)/%.cc
	$(CXX) $(ALL_INCL) -fPIC -o $@ -c $<

$(GENDIR)/%.cc:
	$(PROTOC) -I=$(GENSRCDIR) --cpp_out=$(GENDIR) $(patsubst $(GENDIR)/%.pb.cc,$(GENSRCDIR)/%.proto,$@)
	mkdir -p $(GENDIR)/gen
	cp $(GENDIR)/*.pb.h $(GENDIR)/gen/

# Special target for protobuf files
#%.pb.cc.o: %.pb.cc
#	$(CXX) $(ALL_INCL) -fPIC -o $@ -c $<

#%.proto:

# Rules for cleaning
clean:
	$(RM) $(gen_PB_SRCS) $(gen_PB_OBJS)
	$(RM) -rf $(GENDIR)/gen
	$(RM) $(exe_WI_OBJS)
	$(RM) -rf $(BINDIR)/
	$(RM) $(exe_WI_TEST_OBJS)
	$(RM) -rf $(TESTBINDIR)/
# <<< end 'Build rules'
