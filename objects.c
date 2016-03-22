#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "duktape.h"
#include "php_phpjs.h"

PHPAPI zend_class_entry *phpjs_JSObjectWrapper_ptr;

static zend_object_value phpjs_function_new(zend_class_entry * ce TSRMLS_DC)
{
    zend_object_value retval;
    phpjs_wrap_duk_t * obj;

    obj = (phpjs_wrap_duk_t *) emalloc(sizeof(phpjs_wrap_duk_t));
    memset(obj, 0, sizeof(phpjs_wrap_duk_t));
    zend_object_std_init(&obj->zo, ce TSRMLS_CC);
#if PHP_VERSION_ID < 50399
    zval * tmp;
    zend_hash_copy(obj->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
#else
    object_properties_init((zend_object*) &(obj->zo), ce);
#endif

    retval.handle = zend_objects_store_put(obj, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)phpjs_wrapped_free, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}

ZEND_METHOD(JSObjectWrapper, __call)
{
    FETCH_THIS_WRAPPER
    zval* a_args;
    char * fnc;
    int lfnc;
    int ind;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &fnc, &lfnc, &a_args) == FAILURE)
        return;

    duk_dup(ctx, obj->idx);
    phpjs_php__call(ctx, fnc, a_args, return_value TSRMLS_CC); 
    duk_pop(ctx);
}

ZEND_METHOD(JSObjectWrapper, __get)
{
    FETCH_THIS_WRAPPER
    char * varname;
    int varname_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &varname, &varname_len) == FAILURE)
        return;

    duk_dup(ctx, obj->idx);
    duk_get_prop_string(ctx, -1, varname);
    duk_to_zval(&return_value, ctx, -1);
    php_duk_free_return(ctx);
}

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS___get, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS___call, 0, 0, 2)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()

static zend_function_entry phpjs_JSObjectWrapper_functions[] = {
    ZEND_ME(JSObjectWrapper, __get, ai_phpjs_JS___get, ZEND_ACC_PUBLIC)
    ZEND_ME(JSObjectWrapper, __call, ai_phpjs_JS___call, ZEND_ACC_PUBLIC)
    ZEND_FE_END
};

void php_register_object_handler(TSRMLS_D)
{
    zend_class_entry _ce;
    INIT_CLASS_ENTRY(_ce, "JSObjectWrapper", phpjs_JSObjectWrapper_functions);
    phpjs_JSObjectWrapper_ptr = zend_register_internal_class(&_ce TSRMLS_CC);
    phpjs_JSObjectWrapper_ptr->create_object = phpjs_function_new;
}

duk_ret_t php_object_handler(duk_context *ctx) {
    if (duk_is_constructor_call(ctx)) {
        duk_push_string(ctx, "Can not call this function as constructor");
        duk_throw(ctx);
        return 1;
    }
    zval *func;
    phpjs_object_hanler * obj;
    MAKE_STD_ZVAL(func);
    obj = (phpjs_object_hanler *) emalloc(sizeof(phpjs_object_hanler));
    memset(obj, 0, sizeof(phpjs_object_hanler));
    /* get Function Name */
    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, "__function");
    duk_to_zval(&func, ctx, -1);
    duk_pop(ctx);
    
    /* get Resource Value ({vm:zval, ctx:duk_context*, idx:duk_idx_t}) */
    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, "__res");
    obj = duk_get_pointer(ctx, -1);
    duk_pop(ctx);
    /* TODO EXEC Object Handler */

    duk_push_undefined(ctx);
    return 1;
}

duk_ret_t php_mod_search_handler(duk_context *ctx){
    /* Nargs was given as 4 and we get the following stack arguments:
     *   index 0: id
     *   index 1: require
     *   index 2: exports
     *   index 3: module
     */
    const char *buf;
    int returnVal = DUK_EXEC_SUCCESS,i;
    zval *retval, *func, *params[1];

    MAKE_STD_ZVAL(func);
    MAKE_STD_ZVAL(retval);
    
    zval *val;
    MAKE_STD_ZVAL(val);
    duk_to_zval(&val,ctx,0); // get ID
    params[0] = val;



    /* set function name */
    char * str = "JSModSearch";
    ZVAL_STRINGL(func, str, strlen(str),  1);
    /* exec PHP function : JSModSearch */
    TSRMLS_FETCH();
    if(call_user_function(CG(function_table), NULL, func, retval, 1, params TSRMLS_CC) != SUCCESS) {
        returnVal = DUK_RET_ERROR;
        duk_push_error_object(ctx, DUK_ERR_ERROR, "module not found: \"%s\"", Z_STRVAL_P(val));
        duk_throw(ctx);
    }
    if (EG(exception) != NULL) {
        returnVal = DUK_RET_INTERNAL_ERROR;
        /* There was an exception in the PHP side, let's catch it and throw as a JS exception */
        duk_push_string(ctx, Z_EXCEPTION_PROP("message"));
        zend_clear_exception(TSRMLS_C);
        duk_throw(ctx);
    }

    if(returnVal == DUK_EXEC_SUCCESS && Z_TYPE_P(retval) == IS_STRING) {
        returnVal = 1;
        duk_push_string(ctx, Z_STRVAL_P(retval));
    }else if(Z_TYPE_P(retval) != IS_NULL){
        duk_push_error_object(ctx, DUK_ERR_TYPE_ERROR, "module not loadable : \"%s\"", Z_STRVAL_P(val));
        duk_throw(ctx);
        returnVal = DUK_RET_TYPE_ERROR;
    }else{
        returnVal = DUK_EXEC_SUCCESS;
    }

    zval_ptr_dtor(&func);
    zval_ptr_dtor(&retval);
    zval_ptr_dtor(&params[0]);

    return returnVal ;
}