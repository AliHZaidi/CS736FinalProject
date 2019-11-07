To run this current simple build,

- make all - will build binaries on the CSL machines.
- First partition the input fasta 
    PartitionFA <.fa File Path> <number of chunks>
        This will chunk it out to the directory with the original reads
- ./Frontend <topology .to path> <input filenames eg/ reads_1.fa > <output filename - eg/ out.sam>

# NOTE - currnetly, the frontend doesn't rename output files back to what was initially requested; something like
# output.0_1_2_3_4.bam will be printed.

# Also, the output rightnow is a .bam (binary encoded sam file) - needed for <samtools cat>

Some constants in code that you may want to change:
    In Executables.h,
        const char *BASIC_BACKEND_EXEC = "/u/c/r/crepea/736/CS736FinalProject/src/BackEnd";
        const char *APPEND_FILTER_EXEC = "/u/c/r/crepea/736/CS736FinalProject/src/AppendFilter.so";

    In the Makefile, there are some path to local MRNET includes
        # Setup Flags/Options for compiling files with MRNet
        XPLAT_CONFIG_PATH=/usr/local/lib/xplat-5.0.1/include/
        LIB_PATH=/usr/local/lib

        Make sure these exist on your /usr/local/

    In /basic_partition_hisat2/BasicHISAT2MRNet.h,
        # NUM_CHUNKS is hardcoded to 5. We may we to change this dependent on the topology.

    Generally, you may have to mess around with the topology files to indicate what the nodes are; you have to explicitly name hosts to
    work in a distributed setting (no using localhost)