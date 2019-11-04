/**
 * Partition a .fa (FASTA) file into multiple file 'chunks'.
 * Author: Matthew Crepea
 */
#include <iostream>
#include <fstream>
#include <sstream>

#include <string>

#include <stdexcept>
#include <bits/stdc++.h> 

// For realpath
#include <stdlib.h>

/**
 * Return true if the given file exists on this filesystem.
 * Taken from : http://www.cplusplus.com/forum/general/1796/
 */
bool fexists(const char *filename)
{
  std::ifstream ifile(filename);
  return ifile;
}

/**
 * Count number of lines in a file.
 * Taken from https://stackoverflow.com/questions/3482064/counting-the-number-of-lines-in-a-text-file
 */
int num_reads_in_f(const char *filename)
{
    std::ifstream myfile(filename);

    // new lines will be skipped unless we stop it from happening:    
    myfile.unsetf(std::ios_base::skipws);

    // count the newlines with an algorithm specialized for counting:
    int line_count = std::count(
        std::istream_iterator<char>(myfile),
        std::istream_iterator<char>(), 
        '>');

    return line_count;
}

/**
 * Get stem of a filename.
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

/**
 * Partition and write the given file at f_path into chunks.
 */
int partition_file(char *f_path, int num_chunks)
{
    // Resolve file name info - absolute path, stem, extension
    std::string path_str = f_path;
    std::string file_stem = filename_stem(f_path);
    std::string file_extension = filename_extension(f_path);

    // Get number of lines; number of reads; number of reads per chunk; and leftover reads.
    int number_of_reads = num_reads_in_f(f_path);
    int reads_per_chunk = (number_of_reads / num_chunks);
    int leftover_reads = number_of_reads % num_chunks;

    // Open the file
    std::ifstream ifile(path_str);
    std::string out_fname, line;
    std::stringstream  ss;
    int reads_for_this_chunk;
    
    for(int chunk_ind = 0; chunk_ind < num_chunks; ++chunk_ind)
    {
        reads_for_this_chunk = reads_per_chunk;
        if(chunk_ind < leftover_reads)
        {
            reads_for_this_chunk += 1;
        }

        // Create the output file.
        ss << file_stem << std::string(".") << chunk_ind << std::string(".") << file_extension;
        out_fname = ss.str();
        ss.str("");
        std::ofstream outfile (out_fname);
        
        for(int read_num = 0; read_num < reads_for_this_chunk; ++read_num)
        {
            std::getline(ifile, line);
            outfile << line << std::endl; // Name of read
            std::getline(ifile, line);
            outfile << line << std::endl; // Read contents.
        }

        outfile.close();
    }
    ifile.close();

    return 0;
}

/**
 * One argument - the location of the .fa file on disk.
 */
int main(int argc, char **argv)
{
    // Parse Arguments.
    if(argc < 3)
    {
        std::cerr << "Usage: PartitionFA <.fa File Path> <number of chunks>" << std::endl;
        return -1;
    }

    // Parse filename & resolve to absolute path.
    if(!fexists(argv[1]))
    {
        std::cerr << "ERROR PartitionFA : Unable to read file " << argv[1] << std::endl;
        return -1;
    }
    char *f_name = argv[1];
    char actual_path [PATH_MAX+1]; // Resolve this filename to an absolute filename
    char *ptr;
    ptr = realpath(f_name, actual_path);

    if(NULL == ptr)
    {
        std::cerr << "Unable to resolve file pathname." << std::endl;
        return -1;
    }

    // Parse number of chunks
    int num_chunks;
    try
    {
        num_chunks = std::stoi(argv[2]);
    }
    catch (const std::invalid_argument& ia)
	{
		std::cerr << "Invalid number of chunks given." << std::endl;
        return -1;
	}
    if(num_chunks < 2)
    {
        std::cerr << "Number of chunks must be >= 2." << std::endl;
        return -1;
    }

    // Partition and write files.
    return partition_file(actual_path, num_chunks);
}
