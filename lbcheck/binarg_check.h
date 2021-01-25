#ifndef __BINARG_CHECK_H__
#define __BINARG_CHECK_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <ngx_array.h>

#define LUA_SIGNATURE       "\x1bLua"
#define LUAC_VERSION        0x53
#define LUAC_FORMAT         0
#define LUAC_DATA           "\x19\x93\r\n\x1a\n"
#define CINT_SIZE           4
#define CSIZET_SIZE         8
#define INSTRUCRION_SIZE    4
#define LUA_INTEGER_SIZE    8
#define LUA_NUMBER_SZIE     8
#define LUAC_INT            0x5678
#define LUAC_NUM            370.5

#define ISBIG(M)            (*((uint8_t *)(&((M)->luac_int))))

struct header
{
    uint32_t signature;            //模数
    uint8_t  version;              //版本
    uint8_t  format;               //格式
    uint8_t  luac_data[6];
    uint8_t  cint_size;
    uint8_t  sizet_size;
    uint8_t  instruction_size;
    uint8_t  luaint_size;
    uint8_t  luanumber_size;
    uint64_t luac_int;             //0x5678 确定大小端
    double   luac_num;             //370.5 确定浮点数格式
    uint8_t  protodata[0];
}__attribute__ ((packed));

typedef struct header header_t;

#define TAG_NIL             0x00
#define TAG_BOOLEAN         0x01
#define TAG_NUMBER          0x03
#define TAG_INTEGER         0x13
#define TAG_SHORT_STR       0x04
#define TAG_LONG_STR        0x14

struct short_string
{
    uint8_t length;
    char data[0];
}__attribute__ ((packed));
typedef struct short_string short_string_t;

struct long_string
{
    uint8_t mod;
    size_t length;
    char data[0];
}__attribute__ ((packed));
typedef struct long_string long_string_t;

#define isnil(V)                ((V)->type == TAG_NIL)
#define isbool(V)               ((V)->type == TAG_BOOLEAN)
#define isnumber(V)             ((V)->type == TAG_NUMBER)
#define isinteger(V)            ((V)->type == TAG_INTEGER)
#define isshortstr(V)           ((V)->type == TAG_SHORT_STR)
#define islonhstr(V)            ((V)->type == TAG_LONG_STR)

#define boolvalue(V)            ((V)->value.bool_value)
#define doublevalue(V)          ((V)->value.double_value)
#define intvalue(V)             ((V)->value.int_value)
#define sstring(V)              ((V)->value.string_value.s_str)
#define lstring(V)              ((V)->value.string_value.l_str)

struct constant
{
    uint8_t type;
    union {
        uint8_t  bool_value;
        double   double_value;
        uint64_t int_value;
        union
        {
            struct short_string *s_str;
            struct long_string  *l_str;
        }string_value;
    }value;
}__attribute__ ((packed));
typedef struct constant constant_t;

struct upvalue
{
    uint8_t instack;
    uint8_t idx;
};
typedef struct upvalue upvalue_t;

struct locvar
{
    ngx_str_t str;
    uint32_t  start_pc;
    uint32_t  end_pc;
};
typedef struct locvar locvar_t;

typedef struct prototype prototype_t;
struct prototype
{
    ngx_str_t   source;             //源文件名
    uint32_t    line_defined;       //起止行号
    uint32_t    last_line_defined;
    uint8_t     argc;               //参数个数
    uint8_t     isvararg;           //是否是vararg函数
    uint8_t     maxstack_size;      //寄存器个数
    ngx_array_t *code;              //指令表  uint32_t
    ngx_array_t *constants;         //常量表  constant_t
    ngx_array_t *upvalues;          //upvalue表 upvalue_t
    ngx_array_t *protos;            //子函数表 prototype_t *
    ngx_array_t *lineinfo;          //行号表 uint32
    ngx_array_t *locvars;           //局部变量表
    ngx_array_t *upvalue_names;     //upvalue名表 ngx_str_t
};

struct binary_chunk
{
    char        *file_data;
    int         file_size;
    header_t    *head;
    prototype_t *protofun;
    uint8_t     *pos;
};

int load_binary_file(struct binary_chunk *chunk,const char *file_path);
int protofun_parser(struct binary_chunk *chunk);
int luaobj_info(prototype_t *pro);
int unload_binarg_file(struct binary_chunk *chunk);

#endif