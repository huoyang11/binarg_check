#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <binarg_check.h>

int main(int argc,char *argv[])
{
    struct binary_chunk chunk = {0};
    //printf("size = %lu\n",sizeof(struct binary_chunk));
    load_binary_file(&chunk,"./luac.out");
    protofun_parser(&chunk);
    luaobj_info(chunk.protofun);
    unload_binarg_file(&chunk);

    return 0;
}