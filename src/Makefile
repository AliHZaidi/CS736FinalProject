TARGETS=PartitionFA
TARGETS_BASIC_NET=FrontEnd BackEnd AppendFilter.so
CXX_FLAGS = -std=c++17 \
            -Wall

# Setup Flags/Options for compiling files with MRNet
XPLAT_CONFIG_PATH=/usr/local/lib/xplat-5.0.1/include/
LIB_PATH=/usr/local/lib
MRNET_LIB=mrnet
XPLAT_LIB=xplat
LDFLAGS =  -Wl,-E
SOFLAGS =  -fPIC -shared -rdynamic 

# Includes required for building files which include "mrnet/MRNet.h"
MRNET_INCLUDES=-I $(XPLAT_CONFIG_PATH) -L $(LIB_PATH) -l $(MRNET_LIB) -l $(XPLAT_LIB) -l pthread -ldl -Wall

.PHONY: all all_basic_net clean

all: $(TARGETS)

all_basic_net: $(TARGETS_BASIC_NET)

# Below: Rules for Basic Network

FrontEnd: FrontEnd.o Utils.o
	$(CXX) $(CXX_FLAGS) -o $@ $< Utils.o $(MRNET_INCLUDES) $(LDFLAGS)

FrontEnd.o: basic_partition_hisat2/FrontEnd.cpp basic_partition_hisat2/BasicHISAT2MRNet.h Utils.h Executables.h
	$(CXX) -c $< -o $@  $(MRNET_INCLUDES) $(LDFLAGS)

BackEnd: BackEnd.o Utils.o 
	$(CXX) $(CXX_FLAGS) -o $@ $< Utils.o $(MRNET_INCLUDES) $(LDFLAGS)

BackEnd.o: basic_partition_hisat2/BackEnd.cpp basic_partition_hisat2/BasicHISAT2MRNet.h Utils.h Executables.h
	$(CXX) -c $< -o $@  $(MRNET_INCLUDES) $(LDFLAGS)

AppendFilter.so: basic_partition_hisat2/AppendFilter.cpp Executables.h
	$(CXX) -o $@ $< $(MRNET_INCLUDES) $(SOFLAGS)

# End Section


# Below : Basic Utils Binaries & Programs

PartitionFA: PartitionFA.o Utils.o
	$(CXX) $(CXX_FLAGS) -o $@ $< Utils.o

PartitionFA.o: PartitionFA.cpp Utils.h
	$(CXX) $(CXX_FLAGS) -c $< -o $@

Utils.o: Utils.cpp Utils.h
	$(CXX) $(CXX_FLAGS) -c $< -o $@

# End Section

clean:
	$(RM) $(TARGETS) $(TARGETS_BASIC_NET)
	$(RM) *.o