#include "binarg_check.h"

#define HEADOF      1

static void print_str(char *str,int length)
{
    uint8_t *p = (uint8_t *)str;
    
    for (int i = 0;i < length;i++) {
        printf("%c",p[i]);
    }

    printf("\n");
}

static uint8_t read_uint8(struct binary_chunk *chunk)
{
    uint8_t ret = *((uint8_t *)chunk->pos);
    chunk->pos++;
    return ret;
}

static uint16_t read_uint16(struct binary_chunk *chunk)
{
    uint16_t ret = *((uint16_t *)chunk->pos);
    chunk->pos+=2;
    return ret;
}

static uint32_t read_uint32(struct binary_chunk *chunk)
{
    uint32_t ret = *((uint32_t *)chunk->pos);
    chunk->pos+=4;
    return ret;
}

static uint64_t read_uint64(struct binary_chunk *chunk)
{
    uint64_t ret = *((uint64_t *)chunk->pos);
    chunk->pos+=8;
    return ret;
}

static int read_str(struct binary_chunk *chunk,ngx_str_t *str)
{
    uint8_t *p = chunk->pos;
    long_string_t  *l_str;
    short_string_t *s_str;
    int length = 0;

    if (*p == 0xff) {
        l_str = (long_string_t *)p;
        length = (*((size_t *)(p+1))) - 1;
        str->data = l_str->data;
    } else if (*p <= 0xfe) {
        s_str = (short_string_t *)p;
        length = (*p) - 1;
        str->data = s_str->data;
    } else {
        return -1;
    }

    str->len = length;
    chunk->pos += (length + 1);
    return 0;
}

static int read_code(struct binary_chunk *chunk,prototype_t *pro)
{
    int i = 0;
    uint32_t *val = NULL;
    uint32_t n = read_uint32(chunk);
    if (n == 0) {
        return 0;
    }

    pro->code = ngx_array_create(8,sizeof(uint32_t));

    for (i = 0;i < n;i++) {
        val = ngx_array_push(pro->code);
        *val = read_uint32(chunk);
    }

    return 0;
}

static int read_constants(struct binary_chunk *chunk,prototype_t *pro)
{
    int i = 0;
    uint8_t type;
    constant_t *val = NULL;
    uint32_t n = read_uint32(chunk);
    if (n == 0) {
        return 0;
    }

    pro->constants = ngx_array_create(8,sizeof(constant_t));

    for (i = 0;i < n;i++) {
        val = ngx_array_push(pro->constants);
        type = read_uint8(chunk);
        val->type = type;
        switch (type)
        {
            case TAG_NIL: break;
            case TAG_BOOLEAN:   boolvalue(val) = read_uint8(chunk); break;
            case TAG_NUMBER:    doublevalue(val) = read_uint64(chunk); break;
            case TAG_INTEGER:   intvalue(val) = read_uint32(chunk); break;
            case TAG_SHORT_STR: 
                sstring(val) = (struct short_string *)chunk->pos;
                chunk->pos += sstring(val)->length;
                break;
            case TAG_LONG_STR:
                lstring(val) = (struct long_string *)chunk->pos;
                chunk->pos += lstring(val)->length + 8;
                break;
            default: break;
        }
    }

    return 0;
}

static int read_upvalues(struct binary_chunk *chunk,prototype_t *pro)
{
    int i = 0;
    upvalue_t *val = NULL;
    uint32_t n = read_uint32(chunk);
    if (n == 0) {
        return 0;
    }

    pro->upvalues = ngx_array_create(8,sizeof(upvalue_t));

    for (i = 0;i < n;i++) {
        val = ngx_array_push(pro->upvalues);
        val->instack = read_uint8(chunk);
        val->idx = read_uint8(chunk);
    }

    return 0;
}

static int read_lineinfo(struct binary_chunk *chunk,prototype_t *pro)
{
    int i = 0;
    uint32_t *val = NULL;
    uint32_t n = read_uint32(chunk);
    if (n == 0) {
        return 0;
    }

    pro->lineinfo = ngx_array_create(8,sizeof(uint32_t));

    for (i = 0;i < n;i++) {
        val = ngx_array_push(pro->lineinfo);
        *val = read_uint32(chunk);
    }

    return 0;
}

static int read_locvars(struct binary_chunk *chunk,prototype_t *pro)
{
    int i = 0;
    locvar_t *val = NULL;
    uint32_t n = read_uint32(chunk);
    if (n == 0) {
        return 0;
    }

    pro->locvars = ngx_array_create(8,sizeof(locvar_t));

    for (i = 0;i < n;i++) {
        val = ngx_array_push(pro->locvars);
        read_str(chunk,&val->str);
        val->start_pc = read_uint32(chunk);
        val->end_pc = read_uint32(chunk);
    }

    return 0; 
}

static int read_upvalue_names(struct binary_chunk *chunk,prototype_t *pro)
{
    int i = 0;
    ngx_str_t *val = NULL;
    uint32_t n = read_uint32(chunk);
    if (n == 0) {
        return 0;
    }

    //printf("names %d\n",n);
    pro->upvalue_names = ngx_array_create(8,sizeof(ngx_str_t));

    for (i = 0;i < n;i++) {
        val = ngx_array_push(pro->upvalue_names);
        read_str(chunk,val);
        //print_str(val->data,val->len);
    }

    return 0; 
}

int load_binary_file(const char *file_path,struct binary_chunk *chunk)
{
    FILE *fp = fopen(file_path,"rb");
    char *file_data = NULL;
    header_t *head;
    int length = 0;
    int n = 0;

    if (!chunk || !fp) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

    file_data = calloc(1,length);
    n = fread(file_data,1,length,fp);
    if (n != length) {
        return -1;
    }
    fclose(fp);
#if 0
    printf("%ld\n",offsetof(struct header,signature));
    printf("%ld\n",offsetof(struct header,version));
    printf("%ld\n",offsetof(struct header,format));
    printf("%ld\n",offsetof(struct header,luac_data));
    printf("%ld\n",offsetof(struct header,cint_size));
    printf("%ld\n",offsetof(struct header,sizet_size));
    printf("%ld\n",offsetof(struct header,instruction_size));
    printf("%ld\n",offsetof(struct header,luaint_size));
    printf("%ld\n",offsetof(struct header,luanumber_size));
    printf("%ld\n",offsetof(struct header,luac_int));
    printf("%ld\n",offsetof(struct header,luac_num));
#endif
    head = (struct header*)file_data;

    if (memcmp(&head->signature,LUA_SIGNATURE,4)) {
        printf("is not lua bin!!\n");
        return -1;
    }
    printf("is lua bin!!\n");

    if (head->version != LUAC_VERSION) {
        printf("is lua version not 5.3\n");
        return -1;
    }
    printf("lua version is 5.3\n");
    printf("lua format = %d\n",head->format);

    if (memcmp(&head->luac_data,LUAC_DATA,6)) {
        printf("is not lua bin\n");
        return -1;
    }
    printf("year = %x%x\n",head->luac_data[0],head->luac_data[1]);

    printf("cint size = %u\n",head->cint_size);
    printf("sizet size = %u\n",head->sizet_size);
    printf("instruction size = %u\n",head->instruction_size);
    printf("luaint size = %u\n",head->luaint_size);
    printf("luanumber size = %u\n",head->luanumber_size);

    if (ISBIG(head)) {
        printf("is big\n");
    } else {
        printf("is little\n");
    }

    printf("0x%lx\n",head->luac_int);
    printf("%lf\n",head->luac_num);

    chunk->file_data = file_data;
    chunk->file_size = length;
    chunk->head = head;
    chunk->protofun = calloc(1,sizeof(prototype_t));
    chunk->pos = head->protodata;

    return 0;
}

static int read_prototype(struct binary_chunk *chunk,prototype_t *pro)
{
    int i = 0;
    uint8_t si;
    prototype_t *val = NULL;
    uint32_t n = read_uint32(chunk);
    if (n == 0) {
        return 0;
    }

    pro->protos = ngx_array_create(8,sizeof(prototype_t));

    for (i = 0;i < n;i++) {
        val = ngx_array_push(pro->protos);
        si = read_uint8(chunk);
        if (si) read_str(chunk,&val->source);
        val->line_defined = read_uint32(chunk);
        val->last_line_defined = read_uint32(chunk);
        val->argc = read_uint8(chunk);
        val->isvararg = read_uint8(chunk);
        val->maxstack_size = read_uint8(chunk);
        read_code(chunk,val);
        read_constants(chunk,val);
        
        printf("\n");
        constant_t *ant = val->constants->elts;
        for (int i = 0;i < val->constants->nelts;i++) {
            printf("[%d] ",i);
            print_str(sstring(ant + i)->data,sstring(ant + i)->length - 1);
        }

        read_upvalues(chunk,val);
        read_prototype(chunk,val);
        read_lineinfo(chunk,val);
        read_locvars(chunk,val);
        read_upvalue_names(chunk,val);

        ngx_str_t *str = val->upvalue_names->elts;
        for (int i = 0;i < val->upvalue_names->nelts;i++) {
            printf("[%d] ",i);
            print_str(str->data,str->len);
        }
    }

    return 0; 
}

int protofun_parser(struct binary_chunk *chunk)
{
    int i = 0;
    uint8_t si = read_uint8(chunk);
    printf("%d\n",si);
    prototype_t *pro = chunk->protofun;

    //printf("%lu\n",((unsigned long)chunk->pos - (unsigned long)chunk->file_data));
    if (si) {
        read_str(chunk,&pro->source);
        print_str(pro->source.data,pro->source.len);
    }
    pro->line_defined = read_uint32(chunk);
    pro->last_line_defined = read_uint32(chunk);
    pro->argc = read_uint8(chunk);
    pro->isvararg = read_uint8(chunk);
    pro->maxstack_size = read_uint8(chunk);
    read_code(chunk,pro);
    read_constants(chunk,pro);

    printf("\n");
    constant_t *ant = chunk->protofun->constants->elts;
    for (i = 0;i < chunk->protofun->constants->nelts;i++) {
        printf("[%d] ",i);
        print_str(sstring(ant + i)->data,sstring(ant + i)->length - 1);
    }

    read_upvalues(chunk,pro);
    read_prototype(chunk,pro);
    read_lineinfo(chunk,pro);
    read_locvars(chunk,pro);
    read_upvalue_names(chunk,pro);
    ngx_str_t *str = chunk->protofun->upvalue_names->elts;
    for (i = 0;i < chunk->protofun->upvalue_names->nelts;i++) {
        printf("[%d] ",i);
        print_str(str->data,str->len);
    }

    return 0;
}

int unload_binarg_file(struct binary_chunk *chunk)
{
    free(chunk->file_data);
    memset(chunk,0,sizeof(struct binary_chunk));

    return 0;
}