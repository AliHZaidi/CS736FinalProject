# Set up paths, flags needed for building.
XPLAT_CONFIG_PATH=/usr/local/lib/xplat-5.0.1/include/
LIB_PATH=/usr/local/lib
MRNET_LIB=mrnet
XPLAT_LIB=xplat
LDFLAGS =  -Wl,-E
SOFLAGS =  -fPIC -shared -rdynamic 

# Includes required for building files which include "mrnet/MRNet.h"
MRNET_INCLUDES=-I $(XPLAT_CONFIG_PATH) -L $(LIB_PATH) -l $(MRNET_LIB) -l $(XPLAT_LIB) -l pthread -ldl

TARGETS=front_end back_end
SO_TARGETS=filter.so


.PHONY: all clean

all: $(TARGETS) $(SO_TARGETS)

front_end: front_end.cpp
	$(CXX) -o $@ $< $(MRNET_INCLUDES) $(LDFLAGS)

back_end: back_end.cpp
	$(CXX) -o $@ $< $(MRNET_INCLUDES) $(LDFLAGS)

filter.so: filter.cpp
	$(CXX) -o $@ $< $(MRNET_INCLUDES) $(SOFLAGS)

clean:
	$(RM) $(TARGETS) $(SO_TARGETS)