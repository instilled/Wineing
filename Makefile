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
LIBDIR                = ext/c
RESDIR                = src/main/resources
GENSRCDIR             = $(RESDIR)/protobuf
GENDIR                = src/gen/c
## Files
# *.proto files must be declared here in $(GENS). It is
# expected that they're placed in $(GENSRCDIR). The build will
# otherwise fail!
#
# TODO: If a *.proto file changes 'make all' will not recognize the
# file has changed. It is required to manually invoke 'make clean &&
# make all'.
GENS                  = $(GENSRCDIR)/WineingCtrlProto.proto \
                        $(GENSRCDIR)/WineingMarketDataProto.proto
EXES                  = $(exe_WI_NAME)

## Executables (targets)
# wineing.exe
exe_WI_NAME            = $(BINDIR)/wineing.exe
exe_WI_CC_SRCS         =
exe_WI_CXX_SRCS        = $(SRCDIR)/core/inxcore.win.cc \
                         $(SRCDIR)/core/wchan.cc \
                         $(SRCDIR)/wineing.win.cc
exe_WI_LDFLAGS         = -mwindows
exe_WI_LIBRARY_PATH    =
exe_WI_LIBRARIES       = -luuid \
                         -lzmq \
                         -lprotobuf \
                         -lpthread
exe_WI_DLL_PATH        =
exe_WI_DLLS            = -lodbc32 \
                         -lole32 \
                         -lwinspool \
                         -lodbccp32
exe_WI_OBJS            = $(subst .c,.c.o,$(exe_WI_CC_SRCS)) \
                         $(subst .cc,.cc.o,$(exe_WI_CXX_SRCS)) \
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
DEBUG                 = -ggdb -DDEBUG -DLOG_LEVEL=$(LOG_LEVEL)
LIBRARY_PATH          = # -DMYSYMBOL=VAL
LIBRARIES             = # -lzmq
DLL_PATH              =
DLL_IMPORTS           = # -lole32

DEFINES               =
OPTIONS               = -O3 -DLOG_LEVEL=$(LOG_LEVEL) # excluded from DEBUG target
INCLUDE_PATH          = -I$(GENDIR) -I$(SRCDIR) -I$(RESDIR)/NxCoreAPI 

CFLAGS                = -Wall
CXXFLAGS              = -Wall -std=c++11
LFLAGS                = -Wall

# See http://www.delorie.com/howto/cygwin/mno-cygwin-howto.html
# for what no-cygwin does. Its an incompatibility with -lpthread
CEXTRA               = #-mno-cygwin
CXXEXTRA             = #-mno-cygwin
#RCEXTRA               =


# Concat includes
ALL_INCL              = $(DEFINES) \
                        $(OPTIONS) \
                        $(INCLUDE_PATH)
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

# In case debug target is invoked, lazily prepend debug arguments to
# gcc
debug: CC += $(DUBUG)
debug: CXX += $(DEBUG)
debug: WCC += $(DEBUG)
debug: WCXX += $(DEBUG)
debug: OPTIONS = -O0
debug: release

release: dirs $(EXES) libs

// TODO: test target not yet implemented
test: debug
	@echo "Implement me..."

todo:
	@ack TODO **

$(exe_WI_NAME): gen $(exe_WI_OBJS)
	$(WCXX) $(ALL_LIBS) $(ALL_INCL) $(exe_WI_LDFLAGS) $(exe_WI_OBJS) $(exe_WI_DLL_PATH) $(exe_WI_DLLS) $(exe_WI_LIBRARY_PATH) $(exe_WI_LIBRARIES) -o $@ 

gen: $(gen_PB_SRCS)

libs:
	mkdir -p $(BINDIR)/lib
	cp $(LIBDIR)/* $(BINDIR)/lib

dirs:
	mkdir -p $(BINDIR)
	mkdir -p $(GENDIR)

#%cchan.cc.o: %cchan.cc
#	$(CXX) $(CXXFLAGS) $(CXXEXTRA) $(ALL_INCL) -fPIC -o $@ -c $<

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

# Special target for protobuf files
#%.pb.cc.o: %.pb.cc
#	$(CXX) $(ALL_INCL) -fPIC -o $@ -c $<

#%.proto:

# Rules for cleaning
clean:
	$(RM) $(SRCDIR)/*.o
	$(RM) $(BINDIR)/*.exe $(BINDIR)/*.so
	$(RM) $(GENDIR)/*.cc $(GENDIR)/*.h $(GENDIR)/*.o
	$(RM) $(exe_WI_OBJS)
	$(RM) -rf $(BINDIR)/lib

# <<< end 'Build rules'

### Unused:
# ## nxCoreProtocol buffer compilation
# nxCoreProto_o_TRGT         = NxCoreProto.o
# nxCoreProto_o_SRCS         = $(GENDIR)/*.cc
# nxCoreProto_o_CXXFLAGS     =
#
# gen_NxCoreConfigProto_TRGT = nxCoreConfig
# gen_NxCoreConfigProto_NAME = NxCoreConfigProto
# gen_NxCoreConfigProto_SRCS = $(GENSRCDIR)/$(gen_NxCoreConfigProto_NAME).proto
#
# gen_NxCoreMsgProto_TRGT    = gen_nxCoreMsg
# gen_NxCoreMsgProto_NAME    = NxCoreMsgProto
# gen_NxCoreMsgProto_SRCS    = $(GENSRCDIR)/$(gen_NxCoreMsgProto_NAME).proto
#
# gen_NxCoreConfig_SRC  = $(GENSRCDIR)/NxCoreConfigProto.proto
# gen_NxCoreConfig_TRGT = $(GENDIR)/NxCoreConfigProto.pb.cc
#
# # ## Rules for gen_* targets
# # $(gen_NxCoreConfigProto_TRGT):
# #     $(call fn_genNxCore,$(gen_NxCoreConfigProto_NAME), \
# #                         $(gen_NxCoreConfigProto_SRCS))
#
# # $(gen_NxCoreMsgProto_TRGT):
# #     $(call fn_genNxCore,$(gen_NxCoreMsgProto_NAME), \
# #                         $(gen_NxCoreMsgProto_SRCS))
#
# # ## fn_genNxCore
# # # TODO: Does not work with multiple source files!!! Inkokes
# # # $(PROTOC). $(PROTOC) is only invoked if the source's modification
# # # time is greater that the output file.  Expects two arguments:
# # # - basename of the file to be generated (without .proto ending)
# # # - (relative) path to the file(s)
# # fn_genNxCore =      @if [ $(2) -nt $(GENDIR)/$(1).pb.cc ]; then \
# #                     echo "Generating '$(GENDIR)/$(1).pb.cc'..."; \
# #                     $(PROTOC) -I=$(GENSRCDIR) --cpp_out=$(GENDIR) \
# #                             $(2); \
# #             fi
#
# ## Rules for compingin protoc generated sources
# # TODO: Shall we extract -fPIC and -c to something like
# # $(nxCoreProto_o_CXXFLAGS)? ...I guess not because this is
# # specific to $@
# $(nxCoreProto_o_TRGT): $(gent_NxCoreConfigProto_TRGT)
#       $(CXX) -fPIC -c $(nxCoreProto_o_SRCS)
# # Try to move binaries to target... we would need to copy headers as well
# #     for f in $(nxCoreProto_o_SRCS) ; do \
# #             $(CXX) -fPIC -c -o $(BINDIR)/$$f.pb.cc.o $(GENDIR)/$$f.pb.cc; \
# #     done
#
# foo: foo.o
#       $(CC) -o $@ $^
#
# foo.o: foo.c
#       $(CC) -o $@ -c $<
#
# NxCoreMsgProto: NxCoreMsgProto.pb.o
#       $(CXX) -shared -o $@ -lprotobuf $^
#
# NxCoreMsgProto.pb.o: $(GENDIR)/NxCoreMsgProto.pb.cc
#       $(CXX) -fPIC -c $<
#
# $(GENDIR)/NxCoreMsgProto.pb.cc:
# #$(gen_NxCoreMsgProto_TRGT)
