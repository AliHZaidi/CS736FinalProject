TARGETS=PartitionFA
CXX_FLAGS = -std=c++11 \
            -Wall

.PHONY: all clean

all: $(TARGETS)

PartitionFA: PartitionFA.cpp
	$(CXX) $(CXX_FLAGS) -o $@ $<

clean:
	$(RM) $(TARGETS)