
/*
 * Copyright (C) Hanada
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#define NGX_HTTP_ACCESS_CONTROL_RULE_ALLOW           0
#define NGX_HTTP_ACCESS_CONTROL_RULE_DENY            1


#define NGX_HTTP_ACCESS_CONTROL_INHERIT_ON           0
#define NGX_HTTP_ACCESS_CONTROL_INHERIT_OFF          1
#define NGX_HTTP_ACCESS_CONTROL_INHERIT_BEFORE       2
#define NGX_HTTP_ACCESS_CONTROL_INHERIT_AFTER        3


typedef struct {
    ngx_uint_t                  action;
    ngx_http_complex_value_t   *condition;
} ngx_http_access_control_rule_t;


typedef struct {
    ngx_array_t                *rules;
    ngx_uint_t                  status_code;
    ngx_uint_t                  inherit_mode;
} ngx_http_access_control_loc_conf_t;


static ngx_int_t ngx_http_access_control_handler(ngx_http_request_t *r);
static void *ngx_http_access_control_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_access_control_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);
static char *ngx_http_access_control(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static ngx_int_t ngx_http_access_control_init(ngx_conf_t *cf);


static ngx_conf_enum_t  ngx_http_access_control_inherit[] = {
    { ngx_string("on"), NGX_HTTP_ACCESS_CONTROL_INHERIT_ON },
    { ngx_string("off"), NGX_HTTP_ACCESS_CONTROL_INHERIT_OFF },
    { ngx_string("before"), NGX_HTTP_ACCESS_CONTROL_INHERIT_BEFORE },
    { ngx_string("after"), NGX_HTTP_ACCESS_CONTROL_INHERIT_AFTER },
    { ngx_null_string, 0 }
};


static ngx_command_t ngx_http_access_control_commands[] = {

    { ngx_string("access"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF
                        |NGX_CONF_TAKE2,
      ngx_http_access_control,
      NGX_HTTP_LOC_CONF_OFFSET,
      0, NULL },

    { ngx_string("access_rules_inherit"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_enum_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_access_control_loc_conf_t, inherit_mode),
      &ngx_http_access_control_inherit },

    { ngx_string("access_deny_status"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_access_control_loc_conf_t, status_code),
      NULL },

    ngx_null_command
};


static ngx_http_module_t ngx_http_access_control_module_ctx = {
    NULL,                                    /* preconfiguration */
    ngx_http_access_control_init,            /* postconfiguration */

    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */

    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */

    ngx_http_access_control_create_loc_conf, /* create location configuration */
    ngx_http_access_control_merge_loc_conf   /* merge location configuration */
};


ngx_module_t ngx_http_access_control_module = {
    NGX_MODULE_V1,
    &ngx_http_access_control_module_ctx,      /* module context */
    ngx_http_access_control_commands,         /* module directives */
    NGX_HTTP_MODULE,                          /* module type */
    NULL,                                     /* init master */
    NULL,                                     /* init module */
    NULL,                                     /* init process */
    NULL,                                     /* init thread */
    NULL,                                     /* exit thread */
    NULL,                                     /* exit process */
    NULL,                                     /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_access_control_handler(ngx_http_request_t *r)
{
    ngx_http_access_control_loc_conf_t *alcf;
    ngx_uint_t                          i;
    ngx_http_access_control_rule_t     *rules;
    ngx_str_t                           result;

    alcf = ngx_http_get_module_loc_conf(r, ngx_http_access_control_module);

    if (!alcf->rules) {
        return NGX_DECLINED;
    }

    rules = alcf->rules->elts;

    for (i = 0; i < alcf->rules->nelts; i++) {
        ngx_http_access_control_rule_t *rule = &rules[i];

        if (ngx_http_complex_value(r, rule->condition, &result) != NGX_OK) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        if (result.len == 0 || (result.len == 1 && result.data[0] == '0')) {
            continue;
        }

        if (rule->action == NGX_HTTP_ACCESS_CONTROL_RULE_DENY) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "access denied by access_control rules");
            return alcf->status_code;

        } else { /* NGX_HTTP_ACCESS_CONTROL_RULE_ALLOW */
            return NGX_DECLINED;
        }
    }

    return NGX_DECLINED;
}


static void *
ngx_http_access_control_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_access_control_loc_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_access_control_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->status_code = NGX_CONF_UNSET_UINT;
    conf->inherit_mode = NGX_CONF_UNSET_UINT;

    return conf;
}


static char *
ngx_http_access_control_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child)
{
    ngx_http_access_control_loc_conf_t *prev = parent;
    ngx_http_access_control_loc_conf_t *conf = child;

    ngx_array_t                        *new_rules;
    ngx_http_access_control_rule_t     *pr, *cr, *nr;
    ngx_uint_t                          i;

    ngx_conf_merge_uint_value(conf->inherit_mode, prev->inherit_mode,
                              NGX_HTTP_ACCESS_CONTROL_INHERIT_ON);

    ngx_conf_merge_uint_value(conf->status_code, prev->status_code,
                              NGX_HTTP_FORBIDDEN);

    if (conf->inherit_mode == NGX_HTTP_ACCESS_CONTROL_INHERIT_OFF) {
        if (conf->rules == NGX_CONF_UNSET_PTR) {
            conf->rules = NULL;
        }

        return NGX_CONF_OK;
    }

    if (conf->rules == NGX_CONF_UNSET_PTR) {
        conf->rules = (prev->rules == NGX_CONF_UNSET_PTR) ? NULL : prev->rules;
        return NGX_CONF_OK;
    }

    if (prev->rules == NGX_CONF_UNSET_PTR || prev->rules == NULL) {
        return NGX_CONF_OK;
    }

    if (conf->inherit_mode == NGX_HTTP_ACCESS_CONTROL_INHERIT_ON) {
        return NGX_CONF_OK;
    }

    if (conf->inherit_mode == NGX_HTTP_ACCESS_CONTROL_INHERIT_AFTER) {
        pr = prev->rules->elts;
        for (i = 0; i < prev->rules->nelts; i++) {
            cr = ngx_array_push(conf->rules);
            if (cr == NULL) {
                return NGX_CONF_ERROR;
            }

            *cr = pr[i];
        }

        return NGX_CONF_OK;
    }

    /* NGX_HTTP_ACCESS_CONTROL_INHERIT_BEFORE */
    new_rules = ngx_array_create(cf->pool,
        prev->rules->nelts + conf->rules->nelts,
        sizeof(ngx_http_access_control_rule_t));

    if (new_rules == NULL) {
        return NGX_CONF_ERROR;
    }

    pr = prev->rules->elts;
    for (i = 0; i < prev->rules->nelts; i++) {
        nr = ngx_array_push(new_rules);
        if (nr == NULL) {
            return NGX_CONF_ERROR;
        }

        *nr = pr[i];
    }

    cr = conf->rules->elts;
    for (i = 0; i < conf->rules->nelts; i++) {
        nr = ngx_array_push(new_rules);
        if (nr == NULL) {
            return NGX_CONF_ERROR;
        }

        *nr = cr[i];
    }

    conf->rules = new_rules;

    return NGX_CONF_OK;
}


static char *
ngx_http_access_control(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_access_control_loc_conf_t *alcf = conf;
    ngx_str_t                          *value;
    ngx_http_access_control_rule_t     *rule;
    ngx_http_compile_complex_value_t    ccv;
    ngx_http_complex_value_t           *cv;

    value = cf->args->elts;

    if (cf->args->nelts < 3) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
            "invalid number of arguments in \"access\" directive");
        return NGX_CONF_ERROR;
    }

    if (alcf->rules == NULL) {
        alcf->rules = ngx_array_create(cf->pool, 4,
            sizeof(ngx_http_access_control_rule_t));
        if (alcf->rules == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    rule = ngx_array_push(alcf->rules);
    if (rule == NULL) {
        return NGX_CONF_ERROR;
    }

    if (ngx_strcmp(value[1].data, "allow") == 0) {
        rule->action = NGX_HTTP_ACCESS_CONTROL_RULE_ALLOW;
    } else if (ngx_strcmp(value[1].data, "deny") == 0) {
        rule->action = NGX_HTTP_ACCESS_CONTROL_RULE_DENY;
    } else {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
            "invalid action \"%V\" in \"access\" directive", &value[1]);
        return NGX_CONF_ERROR;
    }

    cv = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
    if (cv == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));
    ccv.cf = cf;
    ccv.value = &value[2];
    ccv.complex_value = cv;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    rule->condition = cv;

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_access_control_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_access_control_handler;

    return NGX_OK;
}
