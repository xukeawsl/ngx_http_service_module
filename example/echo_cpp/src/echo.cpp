#include "ngx_http_service_interface.h"
#include "cJSON.h"

#include <string>
#include <cstring>

extern "C"
{

void srv_echo(const ngx_json_request_t *rqst, ngx_json_response_t *resp);

void str_free(void *str_c);

}

void str_free(void *str_c)
{
    delete [](char *)str_c;
}


void srv_echo(const ngx_json_request_t *rqst, ngx_json_response_t *resp)
{
    if (rqst->data) {
        std::string str(rqst->data);
        char *str_c = new char[str.length() + 1];
        std::memcpy(str_c, str.c_str(), str.length());

        resp->data = str_c;
        resp->release = str_free;
    }
}
