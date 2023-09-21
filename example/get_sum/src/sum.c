#include "ngx_http_service_interface.h"
#include "cJSON.h"

extern int add(int, int);

/*
* {"arr": [1, 2, 3, 4]}
*/
void srv_getSum(const ngx_json_request_t *rqst, ngx_json_response_t *resp)
{
    cJSON *input = cJSON_Parse(rqst->data);
    if (input == NULL) {
        return;
    }

    cJSON *arr = cJSON_GetObjectItem(input, "arr");
    if (cJSON_IsArray(arr)) {
        cJSON *item;
        int size = cJSON_GetArraySize(arr);
        int sum = 0;
        int i;
        for (i = 0; i < size; i++) {
            item = cJSON_GetArrayItem(arr, i);
            sum = add(sum, cJSON_GetNumberValue(item));
        }

        cJSON *output = cJSON_CreateObject();
        if (output == NULL) {
            return;
        }

        cJSON_AddNumberToObject(output, "sum", sum);

        resp->data = cJSON_Print(output);
        cJSON_Delete(output);
    }
}