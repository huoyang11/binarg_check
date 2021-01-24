#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <binarg_check.h>

int main(int argc,char *argv[])
{
    struct binary_chunk chunk = {0};
    //printf("size = %lu\n",sizeof(struct binary_chunk));
    load_binary_file("./luac.out",&chunk);
    protofun_parser(&chunk);
    unload_binarg_file(&chunk);

    return 0;
}