#include "ngx_http_service_interface.h"
#include "cJSON.h"

void srv_test1(const ngx_json_request_t *rqst, ngx_json_response_t *resp)
{
    cJSON *object;

    object = cJSON_CreateObject();
    if (object == NULL) {
        goto end;
    }

    cJSON_AddStringToObject(object, "data", "test func one");

    resp->data = cJSON_Print(object);

end:
    cJSON_Delete(object);
}


void srv_test2(const ngx_json_request_t *rqst, ngx_json_response_t *resp)
{
    cJSON *object;

    object = cJSON_CreateObject();
    if (object == NULL) {
        goto end;
    }

    cJSON_AddStringToObject(object, "data", "test func two");

    resp->data = cJSON_Print(object);

end:
    cJSON_Delete(object);
}