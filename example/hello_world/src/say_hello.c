#include "ngx_http_service_interface.h"
#include "cJSON.h"


void srv_sayHello(const ngx_json_request_t *rqst, ngx_json_response_t *resp)
{
    cJSON *object;

    object = cJSON_CreateObject();
    if (object == NULL) {
        goto end;
    }

    cJSON_AddStringToObject(object, "data", "hello world");

    resp->data = cJSON_Print(object);

end:
    cJSON_Delete(object);
}
