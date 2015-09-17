#include "request.h"
#include "http_parser.h"

#include <vector>
#include <string>
#include <unordered_map>

using namespace std;

static http_parser_settings req_setting;
static bool req_openning = false;
static hydrus::Headers req_temp;



// ------------------------------------------------
//  Request Implementation class
// ------------------------------------------------
class hydrus::Request::RequestImpl
{
private:
    hydrus::Request * parent_;

public:
    http_parser parser;
    bool openning;
    size_t hcounter;
    Headers temp;

    RequestImpl(Request * parent) :parent_(parent), hcounter(0), openning(false)  
    {
        http_parser_init(&parser, HTTP_REQUEST);
    }
};



// ------------------------------------------------
//  http-parser functions
// ------------------------------------------------
#define REQ hydrus::Request

static int
_parser_on_msgbegin(http_parser * pa)   {
    return 0;
}


static int
_parser_on_url(http_parser * pa, const char * at, size_t n) {
    REQ * r = (REQ*)pa->data;
    r->url = string(at, n);
    return 0;
}


static int
_parser_on_headfield(http_parser * pa, const char * at, size_t n) {
    req_temp.name = string(at, n);
    req_openning = true;
    return 0;
}


static int
_parser_on_headvalue(http_parser * pa, const char * at, size_t n) {
    REQ * r = (REQ*)pa->data;

    req_temp.value = string(at, n);
    req_openning = false;
    r->headers.push_back(req_temp);

    return 0;
}


static int
_parser_on_headerdone(http_parser * pa) {
    if (req_openning)
        return -1;
    return 0;
}


static int
_parser_on_body(http_parser * pa, const char * at, size_t n) {
    REQ * r = (REQ*)pa->data;

    if (n == 0) return -1;
    r->body = new char[n];
    memcpy(r->body, at, n);
    return 0;
}


static int
_parser_on_msgdone(http_parser * pa)   {
    REQ * r = (REQ*)pa->data;
    r->method = http_method_str((http_method)pa->method);
    r->status_code = (size_t)pa->status_code;
    r->content_length = (size_t)pa->content_length;

    return 0;
}


static bool
_parser_ready() {
    req_setting.on_message_begin = _parser_on_msgbegin;
    req_setting.on_url = _parser_on_url;
    req_setting.on_header_field = _parser_on_headfield;
    req_setting.on_header_value = _parser_on_headvalue;
    req_setting.on_headers_complete = _parser_on_headerdone;
    req_setting.on_body = _parser_on_body;
    req_setting.on_message_complete = _parser_on_msgdone;

    return true;
}



// ------------------------------------------------
//  Request parser handler
// ------------------------------------------------
hydrus::Request::Request(const char * buffer, size_t nread) :
    already_parsed_(false),
    body(nullptr)
{
    impl_ = new hydrus::Request::RequestImpl(this);
    impl_->parser.data = (void*)this;
    http_parser_execute(&impl_->parser, &req_setting, buffer, nread);
}


hydrus::Request::~Request()
{
    if (body != nullptr)
        free(body);
    delete impl_;
}


bool
hydrus::Request::isParsed() const
{
    return already_parsed_;
}

//
//  This is a little hack to initialize the parser setting without any `init` function
//
bool hydrus::Request::parserReady = _parser_ready();
