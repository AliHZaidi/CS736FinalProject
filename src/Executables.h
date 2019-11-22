#if !defined(hisat2_mrnet_exes_h)
#define hisat2_mrnet_exes_h 1

#include <string>

// Hack-ey way of holding const values for all of our executables that we will be loading in / referencing.
// But hey, it is what it is.

const char *HISAT2_SMALL_PATH  = "/p/genome_mrnet/bin/hisat2-align-s";
const char *HISAT2_LARGE_PATH  = "/p/genome_mrnet/bin/hisat2-align-l";
const char *SAMTOOOLS_PATH     = "/p/genome_mrnet/bin/samtools/bin/samtools"; // lol @ this path

const char *BASIC_BACKEND_EXEC = "/u/c/r/crepea/736/CS736FinalProject/src/BackEnd";
const char *APPEND_FILTER_EXEC = "/u/c/r/crepea/736/CS736FinalProject/src/AppendFilter.so";

const char *STREAMING_BACKEND_EXEC = "/u/c/r/crepea/736/CS736FinalProject/src/SBackEnd";
const char *STREAMING_APPEND_FILTER_EXEC = "/u/c/r/crepea/736/CS736FinalProject/src/SAppendFilter.so";

const char *HG38_INDEX_PATH    = "/p/genome_mrnet/hisat2_reference/h38_ind";
const char *EXAMPLE_INDEX_PATH = "/p/genome_mrnet/hisat2_example/index/22_20-21M_snp";

#endif /* hisat2_mrnet_utils_h */
