#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <binarg_check.h>

int main(int argc,char *argv[])
{
    struct binary_chunk chunk = {0};
 
    if (argc < 2) {
        printf("argc != 2\n");
        return -1;
    }

    load_binary_file(&chunk,argv[1]);
    if (check_head(&chunk)) {
        printf("is not luabin\n");
        return -1;
    }

    protofun_parser(&chunk);
    luaobj_info(chunk.protofun);
    unload_binarg_file(&chunk);

    return 0;
}