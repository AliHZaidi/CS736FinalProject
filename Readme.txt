THE CODE
    The code can be found through AFS in the directory /p/genome_mrnet/. Below is a list of important features of the code set and where to find them.
Main MRNet code - /p/genome_mrnet/CS736FinalProject/hisat2_server_src/
SFrontEnd.cpp - Code for the ‘Front End’ of the MRNet operation - this is in charge of loading in the initial packet filenames, sending data down, and managing control packets to backend nodes.
Hisat2.cpp - This is modified code from the original HISAT2 source to perform a MRNet Backend operation on filenames streamed to the aligner. Slight modifications to the original HISAT2 source have been made to convert this into a streaming version that caches relevant data structures and process packets.
SMergeFilter.cpp - Source code for the different filter and behaviors across the network.
Utils.cpp - File for common utility operations such as modifying filenames
Executables.cpp - Common executables that we use to reference filenames for the Filter Executable, Reference Executables, the Backend Executable, and the Samtools Binary. In a real system we may wish to refactor this into a config file
Compression and Truncation - /p/genome_mrnet/misc_scripts/...
Handle.cpp - Most of the compression and truncation functionality
ReadStructures.h - Data Structures and related functions backing the compression
Note: These files are in progress and so they’re are multiple versions in this directory. This is because we have been investigating many different types of data structures to find the ones that lend themselves best toward this application.
Data Sets -  /p/genome_mrnet/data/
Med_size_example.fa - A 10GB medium size example that we used for basic benchmarking of different topologies (we only used a subset of this file).
ERR184147_1.fa - The NRA12878 benchmarking dataset; this is pre-chunked into 1,000 chunks.
Topologies - We include topologies in the /p/genome_mrnet/CS736FinalProject/topologies file.
Partitioning File Source- /p/genome_mrnet/CS736FinalProject/hisat2_server_src/PartionFA.cpp
Readme - /p/genome_mrnet/CS736FinalProject/Readme.txt

Instructions for Running the Code

From the directory

/p/genome_mrnet/CS736FinalProject/hisat2_server_src/ ,

You can build all relevant files with ‘Make’
Running the partitioning utility can be done with
    ./PartitionFA <filepath> <number of chunks>
Running the entire alignment pipeline can be done with
./SFrontEnd <topology filepath> <Input filepath> <Output filepath> <Number of chunks>
For example, ./SFrontEnd /p/genome_mrnet/CS736FinalProject/topologies/localhost_one_be.to
/p/genome_mrnet/data/ERR184147_1.fa /p/genome_mrnet/output/aligned.sam 1000

