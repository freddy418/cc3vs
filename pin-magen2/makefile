include ../makefile.gnu.config
TARGET_COMPILER?=gnu
INCL?=./include
LINKER?=${CXX}
CXXFLAGS?=-g -static -I$(INCL)
EXTRA_LIBS=

OBJS_CACHE = debugtrace.o 

TOOL_ROOTS = debugtrace
TOOLS = $(TOOL_ROOTS:%=%$(PINTOOL_SUFFIX))

all : tools

tools: $(TOOLS)

%.o : %.cpp
	${CXX} ${COPT} $(CXXFLAGS) ${PIN_CXXFLAGS} ${OUTOPT}$@ $<

debugtrace$(PINTOOL_SUFFIX) : $(OBJS_CACHE) $(PIN_LIBNAMES)
	${LINKER} $(PIN_LDFLAGS) $(LINK_DEBUG) $(OBJS_CACHE) ${LINK_OUT}$@ ${PIN_LPATHS} $(PIN_LIBS) $(EXTRA_LIBS) $(DBG)
	mkdir obj-ia32;
	perl get_spec.pl --ref;
	cp debugtrace.so obj-ia32;
	cp run_spec.sh obj-ia32;

clean:
	-rm -rf *.o $(TOOLS) *out *.tested *.failed *.d *.lib *.exp *~ *.log obj-ia32
