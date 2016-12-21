#include "test.h"
#include "../parser.h"

static void
test_init_parser()
{
	void* data;
	_message_* parser;
	parser = (_message_*)malloc(sizeof(_message_));
	parser->data = NULL;

	parser_init(parser, NULL);
	assert(parser != NULL);
	EXPECT_NULL(parser->data);
	assert(parser->addr != NULL);
	EXPECT_NULL(parser->header->item->next);

	data = parser_destory(parser);
	free(parser);

	EXPECT_NULL(data);
}

static void
test_init_header()
{
	int i, len;
	void* data;
	_message_* parser;
	_header_* header;

	parser = (_message_*)malloc(sizeof(_message_));
	parser_init(parser, NULL);

	header = parser->header;
	assert(header->item == header->last);

	len = header_add(header, "Accept", "text/html, image/gif, image/jpeg");
	EXPECT_INT(1, len);
	len = header_add(header, "Accept-language", "en");
	EXPECT_INT(2, len);
	len = header_add(header, "User-agent", "Mozilla/4.75 [en] (Win98; U)");
	EXPECT_INT(3, len);

	data = parser_destory(parser);

	EXPECT_INT(0, header->len);
	EXPECT_NULL(header->last);
	EXPECT_NULL(header->item);

	free(parser);
}

static void
test_parse_request_command()
{
	char* request;
	size_t req_len, parse_len;
	_message_* parser;
	_header_* header;

	parser = (_message_*)malloc(sizeof(_message_));
	parser_init(parser, NULL);

	request = "GET /tools.html HTTP/1.1\r\n";
	req_len = strlen(request);
	parse_len = header_parse(parser, request, req_len);

	EXPECT_INT(1, parser->version);
	EXPECT_INT(req_len, parse_len);
	EXPECT_INT(HTTP_REQUIRE_REQ_HEADER, parser_errno);

	parser_reset(parser);

	request = "XXX /tools.html HTTP/1.1\r\n";
	req_len = strlen(request);
	parse_len = header_parse(parser, request, req_len);

#if 1
	EXPECT_INT(0, parser->version);
#endif
	EXPECT_INT(0, parse_len);
	EXPECT_INT(HTTP_UNDEFINED_ERROR, parser_errno);

	parser_reset(parser);

	request = "GET /tools.html HTTP/1.1\r";
	req_len = strlen(request);
	parse_len = header_parse(parser, request, req_len);

	EXPECT_INT(1, parser->version);
	EXPECT_INT(req_len, parse_len);
	EXPECT_INT(HTTP_UNEXCEPT_ENDLINE, parser_errno);

	parser_destory(parser);
	free(parser);
}

static void
test_parse_full_request()
{
	char* request;
	size_t req_len, parse_len;
	_message_* parser;
	_header_* header;
	_hnode_* node;

	parser = (_message_*)malloc(sizeof(_message_));
	parser_init(parser, NULL);

	request = 
		"GET / HTTP/1.1\r\n"
		"Host: localhost:9877\r\n"
		"Connection: keep-alive\r\n"
		"Cache-Control: max-age=0\r\n"
		"Upgrade-Insecure-Requests: 1\r\n"
		"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2950.4 Safari/537.36\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
		"DNT: 1\r\n"
		"Accept-Encoding: gzip, deflate, sdch, br\r\n"
		"Accept-Language: zh-CN,zh;q=0.8,en;q=0.6,zh-TW;q=0.4\r\n"
		"\r\n";
	req_len = strlen(request);
	parse_len = header_parse(parser, request, req_len);

	EXPECT_INT(1, parser->version);
	EXPECT_INT(req_len, parse_len);
	EXPECT_INT(HTTP_PARSE_OK, parser_errno);
	EXPECT_INT(9, parser->header->len);

	parser_reset(parser);

	request = 
		"POST / HTTP/1.1\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Host: localhost:9877\r\n"
		"Connection: close\r\n"
		"User-Agent: Paw/3.0.12 (Macintosh; OS X/10.12.2) GCDHTTPRequest\r\n"
		"Content-Length: 48\r\n"
		"\r\n"
		"{\"operator\":\"qiege\",\"song_name\":\"woaiwodezhuguo\"}";
	req_len = strlen(request);
	parse_len = header_parse(parser, request, req_len);

	EXPECT_INT(1, parser->version);
	EXPECT_INT(req_len, parse_len);
	EXPECT_INT(HTTP_PARSE_OK, parser_errno);
	EXPECT_INT(48, parser->content_length);
	EXPECT_HEADER_STRING("{\"operator\":\"qiege\",\"song_name\":\"woaiwodezhuguo\"}", parser->data, 49);
	EXPECT_INT(5, parser->header->len);
}

static void
test_parse_header()
{
	// test_parse_request_command();
#if 1
	test_parse_full_request();
#endif
}

static void
test_init()
{
	test_init_parser();
	test_init_header();
}

static void 
test_parse()
{
	test_parse_header();
}

int 
main() 
{
#ifdef _WINDOWS
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    test_init();
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
}