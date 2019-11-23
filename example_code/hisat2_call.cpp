#include <unistd.h>
#include <sys/wait.h>

char *HISAT2_EXEC = "/p/genome_mrnet/bin/hisat2-align-s";
char *EXAMPLE_IND = "/p/genome_mrnet/hisat2_example/index/22_20-21M_snp";
char *EXAMPLE_READS = "/p/genome_mrnet/hisat2_example/reads/reads_1.fa";
char *OUT = "ex_out.sam";

char *F_FLAG = "-f";
char *X_FLAG = "-x";
char *U_FLAG = "-U";
char *S_FLAG = "-S";

int main()
{
    int pid = fork();
    if(pid == 0)
    {
        // Child
        char *args[9];

        args[0] = HISAT2_EXEC;
        args[1] = F_FLAG;
        args[2] = X_FLAG;
        args[3] = EXAMPLE_IND;
        args[4] = U_FLAG;
        args[5] = EXAMPLE_READS;
        args[6] = S_FLAG;
        args[7] = OUT;
        args[8] = NULL;

        execvp(HISAT2_EXEC, args);
    }
    else
    {
        int status;
        wait(&status);
    }
    
    return 0;
}