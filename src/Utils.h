#if !defined(hisat2_mrnet_utils_h)
#define hisat2_mrnet_utils_h 1

#include <string>
#include <vector>

// Given a filename .sam , eg /foo/bar/reads.sam,
// Return a filename for a corresponding .bam file
// Eg /foo/bar/reads.bam
std::string filename_sam_to_bam(const char *filename)

// Get the 'stem' of a given filename before the extension.
std::string filename_stem(const char *f_name);

// Get the extension of a given filename.
std::string filename_extension(const char *f_name);

// Get the directory of a given file.
std::string filename_dir(const char *f_name);

std::string filename_chunk_str(const char *f_name);

// Return true if a given file exists
bool fexists(const char *filename);

// Return paths to filename chunks given a filename
std::vector<std::string> get_chunk_filenames(std::string file_name, unsigned num_chunks);

// Given a vector of chunks, reduce their filename to one for a 'super chunk' of all the smaller chunks.
std::string concat_chunk_filenames(std::vector<std::string> filenames);


#endif /* hisat2_mrnet_utils_h */
