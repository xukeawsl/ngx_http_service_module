#include <jsoncpp/json/json.h>
#include <cstring>
#include "ngx_http_service_interface.h"


extern "C"
{

void srv_jsoncpp(const ngx_json_request_t *rqst, ngx_json_response_t *resp);

}


void srv_jsoncpp(const ngx_json_request_t *rqst, ngx_json_response_t *resp)
{
    Json::Value root;

    root["data"] = Json::Value("use jsoncpp");

    std::string result = root.toStyledString();

    resp->data = (char *) calloc(1, result.length() + 1);
    if (resp->data == NULL) {
        return;
    }

    memcpy(resp->data, result.c_str(), result.length());
}
