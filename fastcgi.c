/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_fastcgi.h"
#include "connection.h"

/* If you declare any globals in php_fastcgi.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(fastcgi)
*/

/* True global resources - no need for thread safety here */
static int le_fastcgi;

/* {{{ fastcgi_functions[]
 *
 * Every user visible function must have an entry in fastcgi_functions[].
 */
const zend_function_entry fastcgi_functions[] = {
//	PHP_FE(confirm_fastcgi_compiled,	NULL)		/* For testing, remove later. */
    PHP_FE(fcgi_set_domain, NULL)
    PHP_FE(fcgi_set_param, NULL)
    PHP_FE(fcgi_close, NULL)
    PHP_FE(fastcgi, NULL)
	PHP_FE_END	/* Must be the last line in fastcgi_functions[] */
};
/* }}} */

/* {{{ fastcgi_module_entry
 */
zend_module_entry fastcgi_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"fastcgi",
	fastcgi_functions,
	PHP_MINIT(fastcgi),
	PHP_MSHUTDOWN(fastcgi),
	PHP_RINIT(fastcgi),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(fastcgi),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(fastcgi),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_FASTCGI
ZEND_GET_MODULE(fastcgi)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("fastcgi.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_fastcgi_globals, fastcgi_globals)
    STD_PHP_INI_ENTRY("fastcgi.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_fastcgi_globals, fastcgi_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_fastcgi_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_fastcgi_init_globals(zend_fastcgi_globals *fastcgi_globals)
{
	fastcgi_globals->global_value = 0;
	fastcgi_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(fastcgi)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(fastcgi)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(fastcgi)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(fastcgi)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(fastcgi)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "fastcgi support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_fastcgi_compiled(string arg)
   Return a string to confirm that the module is compiled in */
/*
PHP_FUNCTION(confirm_fastcgi_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "fastcgi", arg);
	RETURN_STRINGL(strg, len, 0);
}
*/
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/

const unsigned long max_thread = 10;
char *host;
__uint16_t port;

char ***keys_dict;
char ***vals_dict;
char **content_dict;
int len_dict[10];
int count_request = 0;
char *outputs[10];

pthread_mutex_t mutex;

PHP_FUNCTION(fcgi_set_domain)
{
    char *host_param;
    int host_len_param;
    long port_param;
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &host_param, &host_len_param, &port_param)) {
        return;
    }
    host = (char *)emalloc(sizeof(char)*strlen(host_param));
    strcpy(host, host_param);
    port = port_param;
    keys_dict = (char ***)emalloc(sizeof(char **) * max_thread);
    vals_dict = (char ***)emalloc(sizeof(char **) * max_thread);
    content_dict = (char **)emalloc(sizeof(char *) * max_thread);
    return;
}

PHP_FUNCTION(fcgi_set_param)
{
    zval *z_array;
    char *content_param;
    int content_len_param;
    int count, i;
    zval **z_item;
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "as", &z_array, &content_param, &content_len_param)) {
        return;
    }
    count = zend_hash_num_elements(Z_ARRVAL_P(z_array));

    *(keys_dict+count_request) = (char **)emalloc(sizeof(char *)*count);
    *(vals_dict+count_request) = (char **)emalloc(sizeof(char *)*count);

    zend_hash_internal_pointer_reset(Z_ARRVAL_P(z_array));
    for (i = 0; i < count; i ++) {
        char *key;
        ulong idx;
        zend_hash_get_current_data(Z_ARRVAL_P(z_array), (void**) &z_item);
        convert_to_string_ex(z_item);
        char *val_tmp = Z_STRVAL_PP(z_item);
        
        if (zend_hash_get_current_key(Z_ARRVAL_P(z_array), &key, &idx, 0) == HASH_KEY_IS_STRING) 
        {
            *(*(keys_dict+count_request)+i) = (char *)emalloc(sizeof(char)*strlen(key));
            memset(*(*(keys_dict+count_request)+i), '\0', sizeof(char)*strlen(key));
            strcpy(*(*(keys_dict+count_request)+i), key);
        }
        else
        {
            *(*(keys_dict+count_request)+i) = (char *)emalloc(sizeof(char)*strlen("unkown"));
            memset(*(*(keys_dict+count_request)+i), '\0', sizeof(char)*strlen("unkown"));
            strcpy(*(*(keys_dict+count_request)+i), "unkown");
        }
        *(*(vals_dict+count_request)+i) = (char *)emalloc(sizeof(char)*strlen(val_tmp)+1);
        memset(*(*(vals_dict+count_request)+i), '\0', sizeof(char)*strlen(val_tmp)+1);
        strcpy(*(*(vals_dict+count_request)+i), val_tmp);
        zend_hash_move_forward(Z_ARRVAL_P(z_array));
    }

    if (count_request >= 10) {
        return;
    }
    *(content_dict+count_request) = (char *)emalloc(sizeof(char)*content_len_param);
    strcpy(*(content_dict+count_request), content_param);
    len_dict[count_request] = count;
    count_request++;
    return;
}


PHP_FUNCTION(fcgi_close)
{
    efree(host);

    int loop;
    for(loop=0; loop<count_request; loop++)
    {
        int array_count;
        for(array_count=0;array_count<len_dict[loop];array_count++)
        {
            efree(*(*(keys_dict+loop)+array_count));
            efree(*(*(vals_dict+loop)+array_count));
        }
        efree(*(keys_dict+loop));
        efree(*(vals_dict+loop));
        efree(*(content_dict+loop));
    }

    efree(keys_dict);
    efree(vals_dict);
    efree(content_dict);
    count_request=0;
}

void
fcgi_thread_run(int *id)
{

    unsigned short request_id = *id;
    int sock = INVALID_SOCKET;
    int connect_ret = connect_to_fpm(host, port, &sock);
    if (connect_ret) {
        pthread_exit((void *)1);
    }
    ssize_t begin_ret = begin_send(&sock, request_id);
    if (!begin_ret) {
        close(sock);
        pthread_exit((void *)2);
    }
    ssize_t send_env_ret = send_env(&sock, *(keys_dict+(*id)), *(vals_dict+(*id)), len_dict[(*id)], request_id);
    if (!send_env_ret) {
        close(sock);
        pthread_exit((void *)3);
    }
    ssize_t send_content_ret = send_content(&sock, *(content_dict+(*id)), request_id);
    if (!send_content_ret) {
        pthread_exit((void *)4);
    }
    
    unsigned short content_len;
    ssize_t recv_header_ret = recv_header(&sock, request_id, &content_len);
    if (!recv_header_ret) {
        pthread_exit((void *)5);
    }
    char *content = (char *)emalloc(sizeof(char)*content_len);
    memset(content, '\0', sizeof(char)*content_len);
    ssize_t recv_ret = recv_content(&sock, request_id, content_len, content);
    if (!recv_ret) {
        efree(content);
        pthread_exit((void *)6);
    }
    outputs[*id] = (char *)emalloc(sizeof(char)*content_len+1);
    memset(outputs[*id], '\0', sizeof(char)*content_len+1);
    
    pthread_mutex_lock(&mutex);
    strncpy(outputs[*id], content, content_len);
    pthread_mutex_unlock(&mutex);
    efree(content);
    ssize_t send_end_ret = send_end(&sock, request_id);
    if (!send_end_ret) {
        pthread_exit((void *)7);
    }
    close(sock);
    
    pthread_exit((void *)0);
}

PHP_FUNCTION(fastcgi)
{
    pthread_mutex_init(&mutex, NULL);
    
    pthread_t p_list[max_thread];
    memset(&p_list, 0, sizeof(p_list));
    void *p_ret[max_thread];
    int p_param[max_thread];
    int loop;

    for (loop = 0; loop < count_request; loop++) {
        p_param[loop] = loop;
        pthread_create(&p_list[loop], NULL, (void *)fcgi_thread_run, &p_param[loop]);
        
    }

    for (loop=0; loop<count_request; loop++) {
      if(p_list[loop] != 0)
       {
            pthread_join(p_list[loop], &p_ret[loop]);
       }
    }
    array_init(return_value);
    for(loop=0; loop<count_request; loop++)
    {
        add_next_index_string(return_value, outputs[loop], 1);
        efree(outputs[loop]);
    }
    pthread_mutex_destroy(&mutex);
    return;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
