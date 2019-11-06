#if !defined(hisat2_mrnet_utils_h)
#define hisat2_mrnet_utils_h 1

#include <string>
#include <vector>

// Get the 'stem' of a given filename before the extension.
std::string filename_stem(const char *f_name);

// Get the extension of a given filename.
std::string filename_extension(const char *f_name);

// Get the directory of a given file.
std::string filename_dir(const char *f_name);

// Return true if a given file exists
bool fexists(const char *filename);

// Return paths to filename chunks given a filename
std::vector<std::string> get_chunk_filenames(std::string file_name, unsigned num_chunks);

#endif /* hisat2_mrnet_utils_h */
