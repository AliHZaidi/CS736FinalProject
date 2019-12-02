#if !defined(hisat2_mrnet_exes_h)
#define hisat2_mrnet_exes_h 1

// Hack-ey way of holding const values for all of our executables that we will be loading in / referencing.
// But hey, it is what it is.

extern const char *HISAT2_SMALL_PATH;
extern const char *HISAT2_LARGE_PATH;
extern const char *SAMTOOLS_PATH; // lol @ this path

extern const char *BASIC_BACKEND_EXEC;
extern const char *APPEND_FILTER_EXEC;

extern const char *STREAMING_BACKEND_EXEC;
extern const char *STREAMING_MERGE_FILTER_SO;

extern const char *HG38_INDEX_PATH;
extern const char *EXAMPLE_INDEX_PATH;


#endif /* hisat2_mrnet_utils_h */
