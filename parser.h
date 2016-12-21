#ifndef _H_PARSER
#define _H_PARSER
#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>

#define MAXURL 1024

typedef struct _message_ _message_;
typedef struct _address_ _address_;
typedef struct _header_ _header_;

// HTTP传输方式
enum http_method
{
	HTTP_GET,
	HTTP_POST,
};

// 解析错误提示
enum parse_error
{
	HTTP_PARSE_OK,
	HTTP_UNDEFINED_ERROR = 1, // 未定义的解析时错误
	HTTP_REQUIRE_VERSION, // 缺少HTTP版本号
	HTTP_UNEXCEPT_ENDLINE, // 行尾不以\r\n结尾
	HTTP_REQUIRE_REQ_HEADER, // 请求报文缺少请求头部
	HTTP_INVALID_HEADER, // 不合法的首部报文
	HTTP_POST_NEED_ENTITY, // POST请求报文需要请求实体
};

int parser_errno;

// 抛出错误并退出
#define THROW_ERROR(e, len) \
	do { \
		parser_errno = e; \
		return (len); \
	} while (0)

struct _address_
{
	uint16_t ip;
	uint16_t port;
};

typedef char* header_key; // read-only
typedef char* header_value;
typedef struct _hnode_ _hnode_;

struct _hnode_
{
	header_key field;
	header_value value;
	struct _hnode_* next;
};

struct _header_
{
	size_t len;
	struct _hnode_* item;
	struct _hnode_* last;
};

struct _message_
{
	uint32_t nread;
	uint64_t content_length;

	struct _address_* addr;
	struct _header_* header;

	uint8_t status_code : 7;
	uint8_t method : 3;

	uint8_t upgraded : 1;
	uint8_t version : 1;

	void* data;
};

/* 初始化parser的内存空间，同时会调用init_header来初始化header的内存空间 */
void 
parser_init(_message_ *parser, void* data);

/* _header_内存 */
void
parser_init_header(_message_ *parser);
void
parser_destory_header(_header_ *header);

// TODO
void
parser_reset(_message_ *parser);

/* 回收parser的内存空间，返回内部存储的data
 */
void*
parser_destory(_message_ *parser);

/* 添加一个新的hnode并赋值，成功后节点后移并返回首部个数；失败时会返回0
 */
int
header_add(_header_* header, const header_key hkey, const header_value hval);

/**
 * 执行parse, 错误时将置入错误代码到parse_errno
 * @param  parser 用来存储请求头信息
 * @param  data   接受到的字符信息
 * @param  len    字符信息长度
 * @return        返回已被parse的字节数
 */
extern size_t 
header_parse(_message_ *parser, const char* data, size_t len);

#ifdef __cplusplus	
}
#endif
#endif