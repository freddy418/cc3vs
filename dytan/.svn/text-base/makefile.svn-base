include ../makefile.gnu.config
TARGET_COMPILER?=gnu
INCL?=./include
LINKER ?=${CXX}
CXXFLAGS ?= -g -static -I$(INCL) -I/usr/include/libxml2 $(DBG) $(OPT) -MMD

EXTRA_LIBS = -L/usr/lib32

OBJS_DYTAN = bitset.o dytan.o monitor_path.o monitor_network.o \
	   taint_source_path.o taint_source_network.o taint_source_func.o \
	   replace_functions.o syscall_functions.o taint_func_args.o

OBJS_PENUMBRA = penumbra.o replace_functions.o syscall_functions.o   

TOOL_ROOTS = dytan
TOOLS = $(TOOL_ROOTS:%=%$(PINTOOL_SUFFIX))

all: tools

tools: $(TOOLS)

penumbra.o : instrument_functions.c
dytan.o : instrument_functions.c

#%.o : %.cpp
#	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<

## build rules

%.o : %.cpp
	${CXX} ${COPT} $(CXXFLAGS) ${PIN_CXXFLAGS} ${OUTOPT}$@ $<

penumbra$(PINTOOL_SUFFIX) : $(OBJS_PENUMBRA) $(PIN_LIBNAMES)
	${LINKER} $(PIN_LDFLAGS) $(LINK_DEBUG) $(OBJS_PENUMBRA) ${LINK_OUT}$@ ${PIN_LPATHS} $(PIN_LIBS) $(EXTRA_LIBS) $(DBG)

dytan$(PINTOOL_SUFFIX) : $(OBJS_DYTAN) $(PIN_LIBNAMES)
	${LINKER} $(PIN_LDFLAGS) $(LINK_DEBUG) $(OBJS_DYTAN) ${LINK_OUT}$@ ${PIN_LPATHS} $(PIN_LIBS) $(EXTRA_LIBS) $(DBG)
	mkdir obj-ia32;
	cp dytan.so obj-ia32/
	perl get_spec.pl --ref
	cp run_spec.sh obj-ia32/
	cp run_temporal.sh obj-ia32/
	cp config.xml obj-ia32/
	cp analyze_st.pl obj-ia32/
	cp analyze_td.pl obj-ia32/
	cc -m32 -g -static test.c -o test

## cleaning
clean:
	-rm -rf *.o $(TOOLS) *out *.tested *.failed *.d *.lib *.exp *~ *.log obj-ia32
