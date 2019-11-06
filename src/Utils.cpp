#include <fstream>
#include <sstream>

#include <limits.h>
#include <stdlib.h>

#include "Utils.h"

/**
 * Get stem of a filename (absolute path). Include the path up to the filename.
 * Code taken from https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s14.html
 */
std::string filename_stem(const char *f_name)
{
    std::string path_str = f_name;
    size_t delimiter_ind = path_str.rfind('.', path_str.length());
    if (delimiter_ind != std::string::npos) {
        return path_str.substr(0, delimiter_ind);
    }

    return NULL;
}

/**
 * Get stem of a filename.
 * Code taken from https://www.oreilly.com/library/view/c-cookbook/0596007612/ch10s14.html
 */
std::string filename_extension(const char *f_name)
{
    std::string path_str = f_name;
    size_t delimiter_ind = path_str.rfind('.', path_str.length());
    if (delimiter_ind != std::string::npos) {
        return path_str.substr(delimiter_ind+1, path_str.length() - delimiter_ind);
    }

    return NULL;
}

std::string filename_dir(const char *f_name)
{
    std::string path_str = f_name;
    size_t delimiter_ind = path_str.rfind('/', path_str.length());
    if (delimiter_ind != std::string::npos) {
        return path_str.substr(0, delimiter_ind + 1);
        // return fs::absolute(path_str);
    }

    return NULL;    
}

/**
 * For a given filename of format
 * <stem>.<chunk_index>.extension>,
 * Recover the chunk index
 * eg for local/reads_1.0.fa, split into:
 * steam       -> local/reads_1
 * chunk_index -> 0
 * extension   -> .fa
 * Return the chunk index "0"
 */
std::string filename_chunk_str(const char *f_name)
{
    std::string path_str = f_name;
    std::string first_stem;
    size_t delimiter_ind = path_str.rfind('.', path_str.length());
    if (delimiter_ind != std::string::npos)
    {
        first_stem = path_str.substr(0, delimiter_ind);
    }
    delimiter_ind = first_stem.rfind('.', first_stem.length());
    if (delimiter_ind != std::string::npos)
    {
        return first_stem.substr(delimiter_ind + 1, first_stem.length() - delimiter_ind);
    }

    return "";
}

/**
 * Return true if the given file exists on this filesystem.
 * Taken from : http://www.cplusplus.com/forum/general/1796/
 */
bool fexists(const char *filename)
{
  std::ifstream ifile(filename);
  return ifile.good();
}

/**
 * Get list of file paths to all 'chunks' for a given file.
 * Uses convention that, for given file /path/to/file/reads.fa ,
 * The chunks will be labeled as /path/to/file/reads.0.fa, /path/to/file/reads.1.fa , ...
 */
std::vector<std::string> get_chunk_filenames(std::string file_name, unsigned num_chunks)
{
    const char *f_name = file_name.c_str();
    char actual_path [PATH_MAX+1]; // Resolve this filename to an absolute filename
    realpath(f_name, actual_path);
    
    std::string f_stem = filename_stem(actual_path);
    std::string extension = filename_extension(actual_path);

    std::vector<std::string> chunk_paths;
    std::string curr_str;
    std::stringstream ss;

    for(unsigned i = 0; i < num_chunks; ++i)
    {
        ss << f_stem << std::string(".") << i << std::string(".") << extension;
        curr_str = ss.str();
        ss.str("");

        chunk_paths.push_back(curr_str); 
    }

    return chunk_paths;
}

std::string concat_chunk_filenames(std::vector<std::string> filenames)
{
    if(filenames.size() == 0)
    {
        return "";
    }
    if(filenames.size() == 1)
    {
        return filenames[0];
    }

    std::string f_stem = filename_stem(filename_stem(filenames[0].c_str()).c_str());
    std::string f_ext = filename_extension(filenames[0].c_str());

    std::stringstream ss;
    ss << f_stem << "." << filename_chunk_str(filenames[0].c_str());
    for(std::vector<std::string>::const_iterator i = filenames.begin(); i != filenames.end(); ++i) {
        // Hackey way to skip first element
	if(i == filenames.begin())
	{
	    continue;
	}
	ss << "_" << filename_chunk_str((*i).c_str());
    }
    ss << "." << f_ext;

    return ss.str();
}
std::string filename_sam_to_bam(const char *filename)
{
    std::stringstream ss;

    ss << filename_stem(filename);
    ss << ".bam";

    return ss.str();
}
