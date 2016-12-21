#include "parser.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>

#define CR '\r'
#define LF '\n'

typedef struct _context_ _context_;
struct _context_
{
	const char* message;
};

void 
parser_init(_message_ *parser, void* data)
{
	_address_* addr;

	assert(parser->data == NULL); // 保证parser数据被清空
	memset(parser, 0, sizeof(*parser));
	
	addr = (_address_*)malloc(sizeof(_address_));

	parser->data = data;
	parser->addr = addr;
	
	parser_init_header(parser);
}

void
parser_init_header(_message_ *parser)
{
	_hnode_* hnode;
	_header_* header;

	hnode = (_hnode_*)malloc(sizeof(_hnode_));
	header = (_header_*)malloc(sizeof(_header_));

	assert(hnode != NULL);
	assert(header != NULL);

	memset(hnode, 0, sizeof(_hnode_));

	header->item = hnode;
	header->last = header->item;

	parser->header = header;
	parser->header->len = 0;
}

void
parser_destory_header(_header_ *header)
{
	_hnode_* cursor = header->item, *node;

	assert(cursor != NULL);

	while (cursor->next != NULL) {
		node = cursor->next;
		free(cursor);
		cursor = node;
	}

	free(cursor);

	header->len = 0;
	header->item = header->last = NULL;
}

void
parser_reset(_message_ *parser)
{
	memset(parser->addr, 0, sizeof(_address_));
	parser_destory_header(parser->header);
	parser_init_header(parser);

	parser->content_length = 0;
	parser->nread = 0;
	parser->status_code = 0;
	parser->method = 0;
	parser->upgraded = 0;
	parser->version = 0;
}

void*
parser_destory(_message_ *parser)
{
	free(parser->addr);
	parser_destory_header(parser->header);

	return parser->data;
}

int
header_add(_header_* header, const header_key hkey, const header_value hval)
{
	_hnode_* item, *end;
	assert(header->last != NULL);

	end = header->last;

	if (header->len == 0) {
		end->field = hkey;
		end->value = hval;

		header->len += 1;

		return 1;
	}

	item = (_hnode_*)malloc(sizeof(_hnode_));
	if (item == NULL) return 0;

	memset(item, 0, sizeof(_hnode_));

	item->field = hkey;
	item->value = hval;
	
	end->next = item;
	header->last = end->next;

	header->len += 1;

	return header->len;
}

size_t
header_parse(_message_ *parser, const char* data, size_t len)
{
	int i, n;
	char *dp, ch;
	char *k, *v, *tmp;
	char url[MAXURL + 1];
	size_t used;
	// _context_ context;

	assert(data != NULL);

	dp = data;
	used = n = 0;
	// context.message = data;

	while (n < len) {
#ifdef DEBUG
		fprintf(stderr, "Starting...\n");
		fprintf(stderr, "Header length: %zu\n", len);
		fprintf(stderr, "loading: \n%s", dp);
#endif
		// 读取HTTP报文第一个字符
		switch(*dp) {
			// 解析请求报文
			case 'G':
				if (
					'E' == *(dp + 1) &&
					'T' == *(dp + 2)
				) {
					dp += 3;
					n += 3;
					parser->method = HTTP_GET;
				}
				break;
			case 'P':
				if (
					'O' == *(dp + 1) &&
					'S' == *(dp + 2) &&
					'T' == *(dp + 3)
				) {
					dp += 4;
					n += 4;
					parser->method = HTTP_POST;
				}
				break;
			case 'H':
				if (
					'T' == *(dp + 1) &&
					'T' == *(dp + 2) &&
					'P' == *(dp + 3)
				) // 解析响应报文
					break;
			default:
				THROW_ERROR(HTTP_UNDEFINED_ERROR, n);
		}

#ifdef DEBUG
		fprintf(stderr, "after parser method: \n%s n = %d\n", dp, n);
#endif
		if ((ch = *dp) == ' ' && dp++) n++; 
		else THROW_ERROR(HTTP_UNDEFINED_ERROR, n);

		if ((ch = *dp) == '/') {
			// 解析相对链接
			for (i = 0; ch != ' ' && i < MAXURL; dp++, i++, n++) {
				url[i] = ch; ch = *dp;
			}

			url[i + 1] = '\0';
		} else THROW_ERROR(HTTP_UNDEFINED_ERROR, n);

#ifdef DEBUG
		fprintf(stderr, "after parser pre-url: \n%s n = %d\n", dp, n);
#endif
		if (!(
			'H' == (ch = *dp) &&
			'T' == *(dp + 1) &&
			'T' == *(dp + 2) &&
			'P' == *(dp + 3)
		)) THROW_ERROR(HTTP_REQUIRE_VERSION, n + 4);

		dp += 4; n += 4;
#ifdef DEBUG
		fprintf(stderr, "before parser http version: \n%s n = %d\n", dp, n);
#endif
		if ((ch = *dp) == '/') {
			long version;
			char* end;

			dp++; n++;
			end = dp + 2;
			version = (long)(10 * strtod(dp, &end));
#ifdef DEBUG
		fprintf(stderr, "parser http version: %ld\n", version);
#endif
			switch(version) {
				case 10: 
					parser->version = 0; break;
				case 11:
					parser->version = 1; break;
				default:
					THROW_ERROR(HTTP_UNDEFINED_ERROR, n + 2);
			}
		} else THROW_ERROR(HTTP_REQUIRE_VERSION, n);

		dp += 3; n += 3;
#ifdef DEBUG
		fprintf(stderr, "after parser http version: \n%s n = %d\n", dp, n);
#endif
		if ((ch = *dp) != CR) THROW_ERROR(HTTP_UNEXCEPT_ENDLINE, n);
		if ((ch = *(dp + 1)) != LF) THROW_ERROR(HTTP_UNEXCEPT_ENDLINE, n + 1);

		dp += 2; n += 2; 
#ifdef DEBUG
		fprintf(stderr, "Starting parser header: \n%s n = %d\n", dp, n);
#endif
		// 解析报文首部
		if (*dp == '\0') THROW_ERROR(HTTP_REQUIRE_REQ_HEADER, n);

		while (1) {
			ch = *dp;

			tmp = dp;
			for (i = 0; (ch = *dp) != ':'; dp++, n++, i++)
				if (!isalnum(ch) && ch != '-')
					THROW_ERROR(HTTP_INVALID_HEADER, n);
			k = (char*)malloc(i);
			memcpy(k, tmp, i);
			k[i] = '\0';
#ifdef DEBUG
		fprintf(stderr, "parser header field: \n%s n = %d\n", k, n);
#endif
			dp += 2; n += 2; // 跳过": "

			tmp = dp;
			if (*dp != ' ' && *dp != '\r')
				for (i = 0; *dp != '\r'; dp++, n++, i++);
			v = (char *)malloc(i);
			memcpy(v, tmp, i);
			v[i] = '\0';
#ifdef DEBUG
		fprintf(stderr, "parser header value: \n%s n = %d\n", v, n);
#endif
			header_add(parser->header, k, v);

			switch(*k) {
				case 'c':
				case 'C':
					if (strncmp(k, "Content-Length", 14) == 0) {
						k = v + i;
						parser->content_length = strtol(v, &k, 10) + 1;
					}
					break;
				case 'h':
				case 'H':
					if (strncmp(k, "Host", 4) == 0) {}
					break;
				case 's':
				case 'S':
					break;
				case 'u':
				case 'U':
					if (strncmp(k, "Upgrade", 7) == 0 && strlen(k) == 7)
						parser->upgraded = 1; // 仅支持websocket
					break;
			}

			dp += 2; n += 2; // 跳过"\r\n"
#ifdef DEBUG
		fprintf(stderr, "after parser header: \n%s n = %d\n", dp, n);
#endif
			if (*dp == '\r') {
				if (*(dp + 1) != '\n')
					THROW_ERROR(HTTP_UNDEFINED_ERROR, n + 1);

				dp += 2; n += 2; // 跳过"\r\n"

				break;
			}
		}

#ifdef DEBUG
		fprintf(stderr, "looking for body: \n%s n = %d\n", dp, n);
		fprintf(stderr, "content_length: %d\n", parser->content_length);
#endif
		if (*dp != '\0' && parser->method == HTTP_POST) {
			parser->data = malloc(parser->content_length);
			memcpy(parser->data, dp, parser->content_length);
			dp += parser->content_length - 1;
			n += parser->content_length - 1;
		}

		ch = *dp;
	}

	assert(ch == '\0');

	parser->nread = n;

	THROW_ERROR(HTTP_PARSE_OK, parser->nread);
}