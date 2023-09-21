
/*
 * Copyright (C) xukeawsl
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_http_service_interface.h"


typedef struct ngx_http_service_main_conf_s  ngx_http_service_main_conf_t;
typedef struct ngx_http_service_loc_conf_s  ngx_http_service_loc_conf_t;


typedef struct {
    ngx_int_t    len;
    u_char      *data;
} ngx_http_service_ctx_t;


struct ngx_http_service_main_conf_s {
    ngx_array_t *service;
    ngx_hash_t   service_hash;

    ngx_array_t *module_path;
    ngx_hash_t   module_path_hash;

    ngx_array_t *module_dependency;

    ngx_array_t *module;
    ngx_hash_t   module_hash;
    ngx_uint_t   module_count;

    ngx_str_t   *module_name_map;

    ngx_array_t *module_symbol;
    ngx_hash_t   module_symbol_hash;
};


struct ngx_http_service_loc_conf_s {
    ngx_flag_t   service_mode;
};


static char *ngx_http_service_parse(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_service_parse_handler(ngx_conf_t *cf, ngx_command_t *dummy, void *conf);
static char *ngx_http_service_parse_module_path(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_service_parse_module_path_handler(ngx_conf_t *cf, ngx_command_t *dummy, void *conf);
static char *ngx_http_service_parse_module_dependency(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_service_parse_module_dependency_handler(ngx_conf_t *cf, ngx_command_t *dummy, void *conf);
static void *ngx_http_service_create_main_conf(ngx_conf_t *cf);
static void *ngx_http_service_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_service_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char *ngx_http_service_add_module(ngx_conf_t *cf, ngx_str_t *module_name, ngx_http_service_main_conf_t *smcf);
static ngx_int_t ngx_http_service_load_service(ngx_conf_t *cf);
static ngx_int_t ngx_http_service_module_hash_init(ngx_conf_t *cf, ngx_http_service_main_conf_t *smcf, ngx_pool_t *temp_pool);
static ngx_int_t ngx_http_service_module_load(ngx_conf_t *cf, ngx_http_service_main_conf_t *smcf, ngx_uint_t *module_list);
static void ngx_unload_module(void *data);
static ngx_int_t ngx_http_service_mode_handler(ngx_http_request_t *r);
static void ngx_http_service_mode_post_handler(ngx_http_request_t *r);


static ngx_command_t ngx_http_service_commands[] = {
    { ngx_string("service"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS,
      ngx_http_service_parse,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("module_path"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS,
      ngx_http_service_parse_module_path,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("module_dependency"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS,
      ngx_http_service_parse_module_dependency,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("service_mode"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_service_loc_conf_t, service_mode),
      NULL },

    ngx_null_command
};


static ngx_http_module_t ngx_http_service_module_ctx = {
    NULL,                                /* preconfiguration */
    ngx_http_service_load_service,       /* postconfiguration */

    ngx_http_service_create_main_conf,   /* create main configuration */
    NULL,                                /* init main configuration */

    NULL,                                /* create server configuration */
    NULL,                                /* merge server configuration */

    ngx_http_service_create_loc_conf,    /* create location configuration */
    ngx_http_service_merge_loc_conf      /* merge location configuration */
};


ngx_module_t  ngx_http_service_module = {
    NGX_MODULE_V1,
    &ngx_http_service_module_ctx,          /* module context */
    ngx_http_service_commands,             /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static char *
ngx_http_service_parse(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_service_main_conf_t *smcf = conf;

    char        *rv;
    ngx_conf_t   save;

    if (smcf->service == NULL) {
        smcf->service = ngx_array_create(cf->pool, 64, sizeof(ngx_hash_key_t));
        if (smcf->service == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    if (smcf->module == NULL) {
        smcf->module = ngx_array_create(cf->pool, 64, sizeof(ngx_hash_key_t));
        if (smcf->module == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    save = *cf;
    cf->handler = ngx_http_service_parse_handler;
    cf->handler_conf = conf;

    rv = ngx_conf_parse(cf, NULL);

    *cf = save;

    return rv;
}


static char *
ngx_http_service_parse_handler(ngx_conf_t *cf, ngx_command_t *dummy, void *conf)
{
    ngx_http_service_main_conf_t *smcf = conf;

    ngx_str_t      *value, *module_name, *old;
    ngx_uint_t      i, n, hash;
    ngx_hash_key_t *service;

    value = cf->args->elts;

    if (ngx_strcmp(value[0].data, "include") == 0) {
        if (cf->args->nelts != 2) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid number of arguments"
                               " in \"include\" directive");
            return NGX_CONF_ERROR;
        }

        return ngx_conf_include(cf, dummy, conf);
    }

    module_name = ngx_palloc(cf->pool, sizeof(ngx_str_t));
    if (module_name == NULL) {
        return NGX_CONF_ERROR;
    }

    *module_name = value[0];

    if (ngx_http_service_add_module(cf, module_name, smcf) != NGX_CONF_OK) {
        return NGX_CONF_ERROR;
    }

    for (i = 1; i < cf->args->nelts; i++) {

        hash = ngx_hash_key(value[i].data, value[i].len);

        service = smcf->service->elts;
        for (n = 0; n < smcf->service->nelts; n++) {
            if (ngx_strcmp(value[i].data, service[n].key.data) == 0) {
                old = service[n].value;
                service[n].value = module_name;

                ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                                   "duplicate extension \"%V\", "
                                   "module name: \"%V\", "
                                   "previous module name: \"%V\"",
                                   &value[i], module_name, old);

                goto next;
            }
        }

        service = ngx_array_push(smcf->service);
        if (service == NULL) {
            return NGX_CONF_ERROR;
        }

        service->key = value[i];
        service->key_hash = hash;
        service->value = module_name;

        ngx_conf_log_error(NGX_LOG_DEBUG, cf, 0, 
                           "module_name: %V, service name: %V",
                           module_name, &value[i]);

    next:
        continue;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_service_parse_module_path(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_service_main_conf_t *smcf = conf;

    char        *rv;
    ngx_conf_t   save;

    if (smcf->module_path == NULL) {
        smcf->module_path = ngx_array_create(cf->pool, 64, sizeof(ngx_hash_key_t));
        if (smcf->module_path == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    if (smcf->module == NULL) {
        smcf->module = ngx_array_create(cf->pool, 64, sizeof(ngx_hash_key_t));
        if (smcf->module == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    save = *cf;
    cf->handler = ngx_http_service_parse_module_path_handler;
    cf->handler_conf = conf;

    rv = ngx_conf_parse(cf, NULL);

    *cf = save;

    return rv;
}


static char *
ngx_http_service_parse_module_path_handler(ngx_conf_t *cf, ngx_command_t *dummy, void *conf)
{
    ngx_http_service_main_conf_t *smcf = conf;

    ngx_str_t      *value, *module_path, *old;
    ngx_uint_t      i, n, hash;
    ngx_hash_key_t *module_name;

    value = cf->args->elts;

    if (ngx_strcmp(value[0].data, "include") == 0) {
        if (cf->args->nelts != 2) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid number of arguments"
                               " in \"include\" directive");
            return NGX_CONF_ERROR;
        }

        return ngx_conf_include(cf, dummy, conf);
    }

    module_path = ngx_palloc(cf->pool, sizeof(ngx_str_t));
    if (module_path == NULL) {
        return NGX_CONF_ERROR;
    }

    *module_path = value[0];

    for (i = 1; i < cf->args->nelts; i++) {

        hash = ngx_hash_key(value[i].data, value[i].len);

        if (ngx_http_service_add_module(cf, &value[i], smcf) != NGX_CONF_OK) {
            return NGX_CONF_ERROR;
        }

        module_name = smcf->module_path->elts;
        for (n = 0; n < smcf->module_path->nelts; n++) {
            if (ngx_strcmp(value[i].data, module_name[n].key.data) == 0) {
                old = module_name[n].value;
                module_name[n].value = module_path;

                ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                                   "duplicate extension \"%V\", "
                                   "module name: \"%V\", "
                                   "previous module name: \"%V\"",
                                   &value[i], &module_name[n].key, old);

                goto next;
            }
        }

        module_name = ngx_array_push(smcf->module_path);
        if (module_name == NULL) {
            return NGX_CONF_ERROR;
        }

        module_name->key = value[i];
        module_name->key_hash = hash;
        module_name->value = module_path;

        ngx_conf_log_error(NGX_LOG_DEBUG, cf, 0, 
                           "module path: %V, module name: %V",
                           module_path, &value[i]);

    next:
        continue;
    }

    return NGX_CONF_OK;
}


static char *ngx_http_service_parse_module_dependency(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_service_main_conf_t *smcf = conf;

    char        *rv;
    ngx_conf_t   save;

    if (smcf->module_dependency == NULL) {
        smcf->module_dependency = ngx_array_create(cf->pool, 64, sizeof(ngx_array_t));
        if (smcf->module_dependency == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    if (smcf->module == NULL) {
        smcf->module = ngx_array_create(cf->pool, 64, sizeof(ngx_hash_key_t));
        if (smcf->module == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    save = *cf;
    cf->handler = ngx_http_service_parse_module_dependency_handler;
    cf->handler_conf = conf;

    rv = ngx_conf_parse(cf, NULL);

    *cf = save;

    return rv;
}


static char *
ngx_http_service_parse_module_dependency_handler(ngx_conf_t *cf, ngx_command_t *dummy, void *conf)
{
    ngx_http_service_main_conf_t *smcf = conf;

    ngx_str_t   *module_name, *value;
    ngx_uint_t   i;
    ngx_array_t *module_list;

    module_list = ngx_array_push(smcf->module_dependency);
    if (module_list == NULL) {
        return NGX_CONF_ERROR;
    }

    if (ngx_array_init(module_list, cf->pool, 64, sizeof(ngx_str_t)) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;

    for (i = 0; i < cf->args->nelts; i++) {
        module_name = ngx_palloc(cf->pool, sizeof(ngx_str_t));
        if (module_name == NULL) {
            return NGX_CONF_ERROR;
        }

        module_name = ngx_array_push(module_list);
        if (module_name == NULL) {
            return NGX_CONF_ERROR;
        }
        *module_name = value[i];

        if (ngx_http_service_add_module(cf, module_name, smcf) != NGX_CONF_OK) {
            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
}


static void *ngx_http_service_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_service_main_conf_t *smcf;

    smcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_service_main_conf_t));
    if (smcf == NULL) {
        return NULL;
    }

    return smcf;
}


static void *
ngx_http_service_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_service_loc_conf_t *slcf;

    slcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_service_loc_conf_t));
    if (slcf == NULL) {
        return NULL;
    }

    slcf->service_mode = NGX_CONF_UNSET;

    return slcf;
}


static char *
ngx_http_service_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_service_loc_conf_t *prev = parent;
    ngx_http_service_loc_conf_t *conf = child;

    ngx_conf_merge_value(conf->service_mode, prev->service_mode, 0);

    return NGX_CONF_OK;
}


static char *
ngx_http_service_add_module(ngx_conf_t *cf, ngx_str_t *module_name, ngx_http_service_main_conf_t *smcf)
{
    ngx_uint_t      i, hash, *value;
    ngx_hash_key_t *module_key;

    if (smcf->module == NULL || module_name == NULL) {
        return NGX_CONF_ERROR;
    }

    module_key = smcf->module->elts;

    for (i = 0; i < smcf->module->nelts; i++) {
        if (ngx_strcmp(module_name->data, module_key[i].key.data) == 0) {
            goto found;
        }
    }

    module_key = ngx_array_push(smcf->module);
    if (module_key == NULL) {
        return NGX_CONF_ERROR;
    }

    value = ngx_palloc(cf->pool, sizeof(ngx_uint_t));
    if (value == NULL) {
        return NGX_CONF_ERROR;
    }

    *value = smcf->module_count++;

    hash = ngx_hash_key(module_name->data, module_name->len);

    module_key->key = *module_name;
    module_key->key_hash = hash;
    module_key->value = value;

found:
    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_service_load_service(ngx_conf_t *cf)
{
    ngx_pool_t                   *temp_pool;
    ngx_uint_t                    i, *module_id;
    ngx_hash_key_t               *module_key;
    ngx_hash_init_t               hash_init;
    ngx_http_handler_pt          *h;
    ngx_http_core_main_conf_t    *cmcf;
    ngx_http_service_main_conf_t *smcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_service_mode_handler;

    smcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_service_module);

    /*no module need to load*/
    if (smcf->module_count == 0 || smcf->module == NULL) {
        return NGX_OK;
    }

    /*find module name by symbol name*/
    if (smcf->service && smcf->service_hash.buckets == NULL) {
        hash_init.hash = &smcf->service_hash;
        hash_init.key = ngx_hash_key;
        hash_init.max_size = 1024;
        hash_init.bucket_size = 64;
        hash_init.name = "service_hash";
        hash_init.pool = cf->pool;
        hash_init.temp_pool = NULL;

        if (ngx_hash_init(&hash_init, smcf->service->elts, smcf->service->nelts) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    /*find module path by module name*/
    if (smcf->module_path && smcf->module_path_hash.buckets == NULL) {
        hash_init.hash = &smcf->module_path_hash;
        hash_init.key = ngx_hash_key;
        hash_init.max_size = 1024;
        hash_init.bucket_size = 64;
        hash_init.name = "module_path_hash";
        hash_init.pool = cf->pool;
        hash_init.temp_pool = NULL;

        if (ngx_hash_init(&hash_init, smcf->module_path->elts, smcf->module_path->nelts) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    /*find module id by module name*/
    if (smcf->module && smcf->module_hash.buckets == NULL) {
        hash_init.hash = &smcf->module_hash;
        hash_init.key = ngx_hash_key;
        hash_init.max_size = 1024;
        hash_init.bucket_size = 64;
        hash_init.name = "module_hash";
        hash_init.pool = cf->pool;
        hash_init.temp_pool = NULL;

        if (ngx_hash_init(&hash_init, smcf->module->elts, smcf->module->nelts) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    /*find module name by module id*/
    smcf->module_name_map = ngx_pcalloc(cf->pool, smcf->module_count * sizeof(ngx_str_t));
    if (smcf->module_name_map == NULL) {
        return NGX_ERROR;
    }

    module_key = smcf->module->elts;
    
    for (i = 0; i < smcf->module->nelts; i++) {
        module_id = module_key[i].value;
        smcf->module_name_map[*module_id] = module_key[i].key;
    }


    temp_pool = ngx_create_pool(1024, cf->log);
    if (temp_pool == NULL) {
        return NGX_ERROR;
    }

    if (ngx_http_service_module_hash_init(cf, smcf, temp_pool) != NGX_OK) {
        ngx_destroy_pool(temp_pool);
        return NGX_ERROR;
    }
    ngx_destroy_pool(temp_pool);

    return NGX_OK;
}


static ngx_int_t 
ngx_http_service_module_hash_init(ngx_conf_t *cf, ngx_http_service_main_conf_t *smcf, ngx_pool_t *temp_pool)
{
    ngx_str_t     module_name, *value;
    ngx_uint_t    i, n, key, u, v, front, rear, topo_size;
    ngx_uint_t   *topo_order, *module_id, *indegree, *queue;
    ngx_array_t **module_graph, *module_list;
    

    topo_order = ngx_pcalloc(temp_pool, smcf->module_count * sizeof(ngx_uint_t));
    if (topo_order == NULL) {
        return NGX_ERROR;
    }

    /*No dependencies exist*/
    if (smcf->module_dependency == NULL) {
        for (i = 0; i < smcf->module_count; i++) {
            topo_order[i] = i;
        }

        if (ngx_http_service_module_load(cf, smcf, topo_order) != NGX_OK) {
            return NGX_ERROR;
        }
        return NGX_OK;
    }

    module_graph = ngx_pcalloc(temp_pool, smcf->module_count * sizeof(ngx_array_t *));
    if (module_graph == NULL) {
        return NGX_ERROR;
    }

    for (i = 0; i < smcf->module_count; i++) {
        module_graph[i] = ngx_array_create(temp_pool, 1, sizeof(ngx_uint_t));
        if (module_graph[i] == NULL) {
            return NGX_ERROR;
        }
    }

    indegree = ngx_pcalloc(temp_pool, smcf->module_count * sizeof(ngx_uint_t));
    if (indegree == NULL) {
        return NGX_ERROR;
    }

    queue = ngx_pcalloc(temp_pool, smcf->module_count * sizeof(ngx_uint_t));
    if (queue == NULL) {
        return NGX_ERROR;
    }

    module_list = smcf->module_dependency->elts;
    for (i = 0; i < smcf->module_dependency->nelts; i++) {
        value = module_list->elts;
        module_name = value[0];

        key = ngx_hash_key(module_name.data, module_name.len);
        ngx_strlow(module_name.data, module_name.data, module_name.len);

        module_id = ngx_hash_find(&smcf->module_hash, key, module_name.data, module_name.len);
        if (module_id == NULL) {
            return NGX_ERROR;
        }
        u = *module_id;

        for (n = 1; n < module_list->nelts; n++) {
            key = ngx_hash_key(value[n].data, value[n].len);
            ngx_strlow(value[n].data, value[n].data, value[n].len);

            module_id = ngx_hash_find(&smcf->module_hash, key, value[n].data, value[n].len);
            if (module_id == NULL) {
                return NGX_ERROR;
            }
            v = *module_id;

            module_id = ngx_array_push(module_graph[v]);
            if (module_id == NULL) {
                return NGX_ERROR;
            }
            *module_id = u;

            indegree[u]++;
        }
    }

    front = rear = topo_size = 0;
    for (i = 0; i < smcf->module_count; i++) {
        if (indegree[i] == 0) {
            queue[rear++] = i;
        }
    }

    while (front < rear) {
        u = queue[front++];
        topo_order[topo_size++] = u;

        module_id = module_graph[u]->elts;
        for (i = 0; i < module_graph[u]->nelts; i++) {
            v = module_id[i];

            if (--indegree[v] == 0) {
                queue[rear++] = v;
            }
        }
    }

    if (topo_size != smcf->module_count) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "There is a circular dependency between modules");
        return NGX_ERROR;
    }

    if (ngx_http_service_module_load(cf, smcf, topo_order) != NGX_OK) {
        return NGX_ERROR;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_service_module_load(ngx_conf_t *cf, ngx_http_service_main_conf_t *smcf, ngx_uint_t *module_list)
{
    void                          *handle;
    u_char                        *buf;
    ngx_str_t                     *value, module_name, module_name_lc, module_path;
    ngx_uint_t                     i, n, len, module_id, module_key, symbol_key;
    ngx_hash_key_t                *symbol_list, *symbol;
    ngx_hash_init_t                hash_init;
    ngx_pool_cleanup_t            *cln;
    ngx_http_service_interface_pt  symbol_pt;


    /*initial module symbol hash*/
    hash_init.hash = &smcf->module_symbol_hash;
    hash_init.key = ngx_hash_key;
    hash_init.max_size = 1024;
    hash_init.bucket_size = 64;
    hash_init.name = "module_symbol_hash";
    hash_init.pool = cf->pool;
    hash_init.temp_pool = NULL;

    /*initial module symbol array*/
    smcf->module_symbol = ngx_array_create(cf->pool, 64, sizeof(ngx_hash_key_t));
    if (smcf->module_symbol == NULL) {
        return NGX_ERROR;
    }

    for (i = 0; i < smcf->module_count; i++) {
        module_id = module_list[i];
        module_name = smcf->module_name_map[module_id];

        module_key = ngx_hash_key(module_name.data, module_name.len);
        module_name_lc.len = module_name.len;
        module_name_lc.data = ngx_pstrdup(cf->pool, &module_name);
        ngx_strlow(module_name_lc.data, module_name_lc.data, module_name_lc.len);

        value = ngx_hash_find(&smcf->module_path_hash, module_key, module_name_lc.data, module_name_lc.len);
        if (value == NULL) {
            ngx_str_null(&module_path);
        } else {
            module_path = *value;
        }

        /*maybe need to add '/' after prefix*/
        buf = ngx_pcalloc(cf->pool, module_path.len + module_name.len + 2);
        if (buf == NULL) {
            return NGX_ERROR;
        }

        len = module_path.len;

        if (len > 0 && module_path.data[len - 1] != '/') {
            len = 1;
            ngx_snprintf(buf, module_path.len + module_name.len + 2, "%V/%V", &module_path, &module_name);
        } else {
            len = 0;
            ngx_snprintf(buf, module_path.len + module_name.len + 1, "%V%V", &module_path, &module_name);
        }
        

        module_path.data = buf;
        module_path.len += module_name.len + len;

        ngx_conf_log_error(NGX_LOG_DEBUG, cf, 0, "module id (%ui) module name (%V) module path (%V)",
                           module_id, &module_name, &module_path);

        cln = ngx_pool_cleanup_add(cf->cycle->pool, 0);
        if (cln == NULL) {
            return NGX_ERROR;
        }

        handle = ngx_dlopen(module_path.data);
        if (handle == NULL) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           ngx_dlopen_n " \"%s\" failed (%s)",
                           module_path.data, ngx_dlerror());
            return NGX_ERROR;
        }

        cln->handler = ngx_unload_module;
        cln->data = handle;

        symbol_list = smcf->service->elts;
        for (n = 0; n < smcf->service->nelts; n++) {
            value = symbol_list[n].value;
            if (ngx_strcmp(value->data, module_name.data) != 0)
                continue;

            symbol_pt = ngx_dlsym(handle, (const char *) symbol_list[n].key.data);
            if (symbol_pt == NULL) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           ngx_dlsym_n " \"%V\", \"%s\" failed (%s)",
                           &symbol_list[n].key, "ngx_http_service_module", ngx_dlerror());
                return NGX_ERROR;
            }
            
            symbol_key = ngx_hash_key(symbol_list[n].key.data, symbol_list[n].key.len);

            symbol = ngx_array_push(smcf->module_symbol);
            if (symbol == NULL) {
                return NGX_ERROR;
            }

            symbol->key = symbol_list[n].key;
            symbol->key_hash = symbol_key;
            symbol->value = symbol_pt;
        }
    }

    /*find symbol by module name*/
    if (ngx_hash_init(&hash_init, smcf->module_symbol->elts, smcf->module_symbol->nelts) != NGX_OK) {
        return NGX_ERROR;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_service_mode_handler(ngx_http_request_t *r)
{
    ngx_buf_t                     *buf;
    ngx_int_t                      rc;
    ngx_uint_t                     i, key;
    ngx_chain_t                    out;
    ngx_list_part_t               *part;
    ngx_table_elt_t               *h;
    ngx_json_request_t            *rqst;
    ngx_json_response_t           *resp;
    ngx_http_service_ctx_t        *s;
    ngx_http_service_loc_conf_t   *slcf;
    ngx_http_service_main_conf_t  *smcf;
    ngx_http_service_interface_pt  service_handler_pt;
    

    slcf = ngx_http_get_module_loc_conf(r, ngx_http_service_module);

    if (slcf->service_mode == 0) {
        return NGX_DECLINED;
    }

    smcf = ngx_http_get_module_main_conf(r, ngx_http_service_module);

    s = ngx_http_get_module_ctx(r, ngx_http_service_module);
    if (s == NULL) {
        s = ngx_pcalloc(r->pool, sizeof(ngx_http_service_ctx_t));
        if (s == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        ngx_http_set_ctx(r, s, ngx_http_service_module);
    }

    /* we response to 'POST' requests only */
    if (!(r->method & NGX_HTTP_POST)) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    if (r->headers_in.content_type == NULL) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    /* we response to 'application/json' request header value */
    if (ngx_strncasecmp(r->headers_in.content_type->value.data, (u_char *) "application/json", 16) != 0)
    {
        return NGX_HTTP_NOT_ALLOWED;
    }

    part = &r->headers_in.headers.part;
    h = part->elts;

    /* we response to 'service-name' request header key */
    for (i = 0; /* void */; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            h = part->elts;

            i = 0;
        }

        if (ngx_strncasecmp(h[i].key.data, (u_char *) "service-name", 12) == 0) {
            goto found;
        }
    }

    return NGX_HTTP_NOT_ALLOWED;

found:
    key = ngx_hash_key(h[i].value.data, h[i].value.len);
    ngx_strlow(h[i].value.data, h[i].value.data, h[i].value.len);

    service_handler_pt = ngx_hash_find(&smcf->module_symbol_hash, key, h[i].value.data, h[i].value.len);
    if (service_handler_pt == NULL) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    rc = ngx_http_read_client_request_body(r, ngx_http_service_mode_post_handler);
    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        return rc;
    }

    if (s->len == NGX_ERROR) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    rqst = ngx_pnalloc(r->pool, sizeof(ngx_json_request_t));
    if (rqst == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    rqst->data = (const char *) s->data;

    resp = ngx_pcalloc(r->pool, sizeof(ngx_json_response_t));
    if (resp == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    service_handler_pt(rqst, resp);

    r->headers_out.status = NGX_HTTP_OK;
    ngx_str_set(&r->headers_out.content_type, "application/json");
    
    if (resp->data == NULL) {
        r->header_only = 1;

    } else {
        r->headers_out.content_length_n = ngx_strlen(resp->data);
    }

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    buf = ngx_create_temp_buf(r->pool, r->headers_out.content_length_n);
    if (buf == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_memcpy(buf->pos, resp->data, r->headers_out.content_length_n);

    if (resp->release == NULL) {
        ngx_free(resp->data);

    } else {
        resp->release(resp->data);
    }

    buf->last = buf->pos + r->headers_out.content_length_n;
    buf->last_buf = 1;

    out.buf = buf;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}


static void
ngx_http_service_mode_post_handler(ngx_http_request_t *r)
{
    u_char *sbuf;
    ssize_t n, size;
    ngx_chain_t *in;
    ngx_http_service_ctx_t *s;


    s = ngx_http_get_module_ctx(r, ngx_http_service_module);

    if (r->request_body == NULL || r->request_body->bufs == NULL) {
        return;
    }

    s->len = r->headers_in.content_length_n;

    s->data = ngx_pcalloc(r->pool, s->len * sizeof(char) + 1);
    if (s->data == NULL) {
        s->len = NGX_ERROR;
        return;
    }

    sbuf = s->data;

    for (in = r->request_body->bufs; in; in = in->next) {
        if (in->buf->in_file) {
            size = in->buf->file_last - in->buf->file_pos;
            n = ngx_read_file(in->buf->file, sbuf, size, in->buf->file_pos);
            if (n == NGX_ERROR) {
                s->len = NGX_ERROR;
                return;
            }

            if (n != size) {
                ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0,
                              ngx_read_file_n " returned "
                                   "only %z bytes instead of %z",
                                   n, size);
                s->len = NGX_ERROR;
                return;
            }

        } else {
            size = in->buf->last - in->buf->pos;
            ngx_memcpy(sbuf, in->buf->pos, size);
        }
        sbuf += size;
    }
}


static void
ngx_unload_module(void *data)
{
    void  *handle = data;

    if (ngx_dlclose(handle) != 0) {
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0,
                      ngx_dlclose_n " failed (%s)", ngx_dlerror());
    }
}