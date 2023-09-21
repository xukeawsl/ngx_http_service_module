
/*
 * Copyright (C) xukeawsl
 */


#ifndef _NGX_HTTP_SERVICE_INTERFACE_H_INCLUDED_
#define _NGX_HTTP_SERVICE_INTERFACE_H_INCLUDED_


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct ngx_json_request_s   ngx_json_request_t;
typedef struct ngx_json_response_s  ngx_json_response_t;
typedef void (*ngx_http_service_interface_pt)(const ngx_json_request_t *rqst, ngx_json_response_t *resp);


struct ngx_json_request_s {
    const char *data;
};


struct ngx_json_response_s {
    char *data;
    void (*release)(void *);
};


#ifdef __cplusplus
}
#endif


#endif