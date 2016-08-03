#include <ngx_http.h>

#define ONE_DAY 86400
#define SEC_PER_HOUR 3600
#define SEC_PER_MIN 60

typedef struct {
	int expire;
} ngx_http_time_loc_conf_t;

static ngx_int_t ngx_http_time_preconf(ngx_conf_t *);

static ngx_int_t time_add_variables(ngx_conf_t *);
static ngx_int_t time_second_get (ngx_http_request_t *, ngx_http_variable_value_t *, uintptr_t);
static ngx_int_t time_unix_get (ngx_http_request_t *, ngx_http_variable_value_t *, uintptr_t);
static char * ngx_http_time_expire(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void * ngx_http_time_create_loc_conf(ngx_conf_t *cf);
static char * ngx_http_time_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);


static ngx_http_variable_t time_variables[] = {
	{ ngx_string("time_second"),
		NULL, time_second_get,
		0,
		0,
		0
	},
	{ ngx_string("time_unix"),
		NULL, time_unix_get,
		0,
		0,
		0
	},
	{ ngx_null_string, NULL, NULL, 0, 0, 0 }
};

static ngx_command_t ngx_http_time_commands[] = {
	{ ngx_string("time_expire"),
		NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
		ngx_http_time_expire,
		NGX_HTTP_LOC_CONF_OFFSET,
		0,
		NULL },

	ngx_null_command
};

static ngx_http_module_t  ngx_http_time_module_ctx = {
	ngx_http_time_preconf,         /* preconfiguration */
	NULL,                          /* postconfiguration */

	NULL,                          /* create main configuration */
	NULL,                          /* init main configuration */

	NULL,                          /* create server configuration */
	NULL,                          /* merge server configuration */

	ngx_http_time_create_loc_conf, /* create location configuration */
	ngx_http_time_merge_loc_conf   /* merge location configuration */
};

ngx_module_t  ngx_http_time_module = {
	NGX_MODULE_V1,
	&ngx_http_time_module_ctx, /* module context */
	ngx_http_time_commands,    /* module directives */
	NGX_HTTP_MODULE,           /* module type */
	NULL,                      /* init master */
	NULL,                      /* init module */
	NULL,                      /* init process */
	NULL,                      /* init thread */
	NULL,                      /* exit thread */
	NULL,                      /* exit process */
	NULL,                      /* exit master */
	NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_time_preconf(ngx_conf_t *cf)
{
    if (time_add_variables(cf) == NGX_OK) {
        return NGX_OK;
    }
    return NGX_ERROR;
}

static ngx_int_t time_add_variables(ngx_conf_t *cf)
{
	int i;
	ngx_http_variable_t *var;

	for (i = 0; time_variables[i].name.len > 0; ++i) {
		var = ngx_http_add_variable(cf, &time_variables[i].name, time_variables[i].flags);
		if (var == NULL) {
			ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "time add variable '%s' failed.", time_variables[i].name.data);
			return NGX_ERROR;
		}

		var->set_handler = time_variables[i].set_handler;
		var->get_handler = time_variables[i].get_handler;
		var->data = time_variables[i].data;
	}

	return NGX_OK;
}

static ngx_int_t time_second_get (ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
	u_char *p;
	ngx_http_time_loc_conf_t *ltmcf;
	int expire, sec;
	time_t timer;
	struct tm *now;

	timer = time(NULL);
	now = localtime(&timer);

	ltmcf = ngx_http_get_module_loc_conf(r, ngx_http_time_module);

	sec = now->tm_hour * SEC_PER_HOUR + now->tm_min * SEC_PER_MIN + now->tm_sec;
	if (sec > ONE_DAY - ltmcf->expire) {
		expire = ONE_DAY - sec;
	} else {
		expire = ltmcf->expire;
	}

	p = ngx_pnalloc(r->pool, NGX_TIME_T_LEN);
	if (p == NULL) {
		return NGX_ERROR;
	}

	v->len = ngx_sprintf(p, "%d", expire) - p;
	v->valid = 1;
	v->no_cacheable = 0;
	v->not_found = 0;
	v->data = p;

	return NGX_OK;
}

static ngx_int_t time_unix_get (ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
	u_char *p;
	ngx_http_time_loc_conf_t *ltmcf;
	int expire, sec;
	time_t timer;
	struct tm *now;

	timer = time(NULL);
	now = localtime(&timer);

	ltmcf = ngx_http_get_module_loc_conf(r, ngx_http_time_module);

	sec = now->tm_hour * SEC_PER_HOUR + now->tm_min * SEC_PER_MIN + now->tm_sec;
	if (sec > ONE_DAY - ltmcf->expire) {
		expire = ONE_DAY - sec;
	} else {
		expire = ltmcf->expire;
	}

	p = ngx_pnalloc(r->pool, NGX_TIME_T_LEN);
	if (p == NULL) {
		return NGX_ERROR;
	}

	v->len = ngx_sprintf(p, "%T", ngx_time() + expire) - p;
	v->valid = 1;
	v->no_cacheable = 0;
	v->not_found = 0;
	v->data = p;

	return NGX_OK;
}

static char * ngx_http_time_expire(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_str_t *args;
	ngx_http_time_loc_conf_t *ltmcf;
	int expire;

	ltmcf = conf;
	args = cf->args->elts;

	expire = atoi((char *)args[1].data);
	if (expire >= ONE_DAY) {
		ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "time_expire is too large.");
		return NGX_CONF_ERROR;
	}

	ltmcf->expire = expire;

	return NGX_CONF_OK;
}

static void *
ngx_http_time_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_time_loc_conf_t  *conf;

	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_time_loc_conf_t));
	if (conf == NULL) {
		return NGX_CONF_ERROR;
	}

	conf->expire = 0;

	return conf;
}

static char *
ngx_http_time_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_http_time_loc_conf_t *prev = parent;
	ngx_http_time_loc_conf_t *conf = child;

	conf->expire = prev->expire;

	return NGX_CONF_OK;
}
