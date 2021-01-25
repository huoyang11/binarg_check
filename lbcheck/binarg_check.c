#include "binarg_check.h"

#define HEADOF      1

static int read_prototypes(struct binary_chunk *chunk,prototype_t *pro);

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

    pro->code = ngx_array_create(8,sizeof(uint32_t));
    if (n == 0) {
        return 0;
    }

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

    pro->upvalues = ngx_array_create(8,sizeof(upvalue_t));
    if (n == 0) {
        return 0;
    }

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

    pro->lineinfo = ngx_array_create(8,sizeof(uint32_t));
    if (n == 0) {
        return 0;
    }

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
   
    pro->locvars = ngx_array_create(8,sizeof(locvar_t));
    if (n == 0) {
        return 0;
    }

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

    pro->upvalue_names = ngx_array_create(8,sizeof(ngx_str_t));
    if (n == 0) {
        return 0;
    }

    for (i = 0;i < n;i++) {
        val = ngx_array_push(pro->upvalue_names);
        read_str(chunk,val);
        //print_str(val->data,val->len);
    }

    return 0; 
}

int check_head(struct binary_chunk *chunk)
{
    header_t *head = chunk->head;
    if (!head) {
        return -1;
    }

    if (memcmp(&head->signature,LUA_SIGNATURE,4)) {
        return -1;
    }

    if (head->version != LUAC_VERSION) {
        return -1;
    }

    if (memcmp(&head->luac_data,LUAC_DATA,6)) {
        return -1;
    }

    if (head->luac_int != LUAC_INT) {
        return -1;
    }

    if (head->luac_num != LUAC_NUM) {
        return -1;
    }

    return 0;
}

int load_binary_file(struct binary_chunk *chunk,const char *file_path)
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

    head = (struct header*)file_data;

    chunk->file_data = file_data;
    chunk->file_size = length;
    chunk->head = head;
    chunk->protofun = calloc(1,sizeof(prototype_t));
    chunk->pos = head->protodata;

    return 0;
}

static int read_proto(struct binary_chunk *chunk,prototype_t *pro)
{
    uint8_t si = read_uint8(chunk);
    if (si) read_str(chunk,&pro->source);
    else memcpy(&pro->source,&chunk->protofun->source,sizeof(ngx_str_t));
    pro->line_defined = read_uint32(chunk);
    pro->last_line_defined = read_uint32(chunk);
    pro->argc = read_uint8(chunk);
    pro->isvararg = read_uint8(chunk);
    pro->maxstack_size = read_uint8(chunk);
    read_code(chunk,pro);
    read_constants(chunk,pro);
    read_upvalues(chunk,pro);
    read_prototypes(chunk,pro);
    read_lineinfo(chunk,pro);
    read_locvars(chunk,pro);
    read_upvalue_names(chunk,pro);

    return 0;
}

static int read_prototypes(struct binary_chunk *chunk,prototype_t *pro)
{
    int i = 0;
    uint8_t si;
    prototype_t *val = NULL;
    uint32_t n = read_uint32(chunk);

    pro->protos = ngx_array_create(8,sizeof(prototype_t));
    if (n == 0) {
        return 0;
    }

    for (i = 0;i < n;i++) {
        val = ngx_array_push(pro->protos);
        read_proto(chunk,val);
    }

    return 0; 
}

int protofun_parser(struct binary_chunk *chunk)
{ 
    prototype_t *pro = chunk->protofun;

    read_proto(chunk,pro);

    return 0;
}

static int print_line(prototype_t *pro)
{
    if (!pro) {
        return -1;
    }

    uint32_t *lines = pro->lineinfo->elts;
    uint32_t *codes = pro->code->elts;

    for (int i = 0;i < pro->lineinfo->nelts;i++) {
        printf("\t%d\t[%d]\t0x%x\n",i + 1,lines[i],codes[i]);
    }

    return 0;
}

static int print_constants(prototype_t *pro)
{
    if (!pro) {
        return -1;
    }

    printf("constants (%d)\n",pro->constants->nelts);
    constant_t *ant = pro->constants->elts;
    for (int i = 0;i < pro->constants->nelts;i++) {
        
        printf("\t%d\t",i+1);
        switch (ant->type)
        {
            case TAG_NIL:       break;
            case TAG_BOOLEAN:   
                printf("%d\n",isbool(ant + i));
                break;
            case TAG_NUMBER:
                printf("%lf\n",isnumber(ant + i));
                break;
            case TAG_INTEGER:   
                printf("%d\n",isinteger(ant + i));
                break;
            case TAG_SHORT_STR:                 
                print_str(sstring(ant + i)->data,sstring(ant + i)->length - 1);
                break;
            case TAG_LONG_STR: break;
            default: break;
        }        
    }

    return 0;
}

static int print_locals(prototype_t *pro)
{
    if (!pro) {
        return -1;
    }

    printf("locals (%d)\n",pro->locvars->nelts);
    locvar_t *ant = pro->locvars->elts;
    for (int i = 0;i < pro->locvars->nelts;i++) {
        printf("%d %d ",ant->start_pc,ant->end_pc);
        print_str(ant->str.data,ant->str.len - 1);
    }

    return 0;
}

static int print_upvalue_names(prototype_t *pro)
{
    if (!pro) {
        return -1;
    }

    printf("upvalues (%d)\n",pro->upvalue_names->nelts);
    ngx_str_t *ant = pro->upvalue_names->elts;
    for (int i = 0;i < pro->upvalue_names->nelts;i++) {
        printf("\t");
        print_str(ant->data,ant->len);
    }

    return 0;
}

int luaobj_info(prototype_t *pro)
{
    if (!pro) {
        return -1;
    }

    char buf[1024] = {0};
    memcpy(buf,pro->source.data,pro->source.len);
    printf("function <%s:%d,%d>\n", buf, pro->line_defined,pro->last_line_defined, pro->code->nelts);

    char c = 0;
    if (pro->isvararg) c = '+';
    printf("%d%c params,%d slots ,%d upvalues,", pro->argc,c, pro->maxstack_size, pro->upvalues->nelts);
    printf("%d locals ,%d constants ,%d functions\n", pro->locvars->nelts, pro->constants->nelts, pro->protos->nelts);

    print_line(pro);
    print_constants(pro);
    print_locals(pro);
    print_upvalue_names(pro);

    int n = pro->protos->nelts;
    prototype_t *pros = pro->protos->elts;
    for (int i = 0;i < n;i++) {
        luaobj_info(pros + i);
    }

    return 0;
}

int unload_binarg_file(struct binary_chunk *chunk)
{
    free(chunk->file_data);
    memset(chunk,0,sizeof(struct binary_chunk));

    return 0;
}