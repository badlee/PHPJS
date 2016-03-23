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
duk_ret_t phpjs_empty_function(duk_context *ctx) {
    if (duk_is_constructor_call(ctx)) {
        duk_push_string(ctx, "Can not call this function as constructor");
        duk_throw(ctx);
        return DUK_RET_ERROR;
    }

    return 0;
}

duk_ret_t phpjs_obj_get_function(duk_context *ctx) {
    zval * value = duk_get_pointer(ctx, 0);
    char * name = duk_get_string(ctx, 1);
    {
        zval * prop;
        MAKE_STD_ZVAL(prop);
        TSRMLS_FETCH();
        prop = zend_read_property(zend_get_class_entry(value TSRMLS_DC),value,name,strlen(name),1 TSRMLS_DC);
        if (EG(exception) != NULL) {
            // There was an exception in the PHP side, let's catch it and throw as a JS exception
            duk_push_string(ctx, Z_EXCEPTION_PROP("message"));
            zend_clear_exception(TSRMLS_C);
            duk_throw(ctx);
            return DUK_RET_INTERNAL_ERROR;
        }
        if(Z_TYPE_P(prop) == 0){
            if(Z_OBJ_HT_P(value)->get_method(&value,name,strlen(name), NULL TSRMLS_DC) != 0){    
                duk_push_c_function(ctx, php_object_handler, DUK_VARARGS);
                duk_push_string(ctx, name);
                duk_put_prop_string(ctx, -2, "__function");
                duk_push_pointer(ctx, value);
                duk_put_prop_string(ctx, -2, "__zval__");
            }else{
                duk_push_null(ctx);
            }
        }else{
            zval_to_duk(ctx, NULL, prop);
        }
        //zval_ptr_dtor(&prop); // bug
        return 1;
    }
    return 0;
}
duk_ret_t phpjs_obj_keys_function(duk_context *ctx) {
    zval * value = duk_get_pointer(ctx, 0);
    HashTable *ht       = NULL;
    zval    **ppzval    = NULL;
    int arr_idx         = 0;

    ht = Z_OBJ_HT_P(value)->get_properties((value) TSRMLS_CC);
    
    duk_idx_t obj_idx = duk_push_array(ctx); // lua_newtable(L);
    for(zend_hash_internal_pointer_reset(ht);
            zend_hash_get_current_data(ht, (void **)&ppzval) == SUCCESS;
            zend_hash_move_forward(ht)) {
        // lookup key
        char *key = NULL;
        uint len  = 0;
        ulong idx  = 0;
        zval *zkey= NULL;
        switch(zend_hash_get_current_key_ex(ht, &key, &len, &idx, 0, NULL)) {
            case HASH_KEY_IS_STRING :
                MAKE_STD_ZVAL(zkey);
                ZVAL_STRINGL(zkey, key, len - 1, 1);
                duk_push_string(ctx,Z_STRVAL_P(zkey));
                break;
            case HASH_KEY_IS_LONG:
                MAKE_STD_ZVAL(zkey);
                ZVAL_LONG(zkey, idx);
                duk_push_string(ctx,Z_LVAL_P(zkey));
                break;
        }
        zval_ptr_dtor(&zkey);
        // write value
        duk_put_prop_index(ctx, obj_idx, arr_idx++);
    }

    /* end from php-lua*/
    return 1;
}

duk_ret_t phpjs_obj_has_function(duk_context *ctx) {
    zval * value = duk_get_pointer(ctx, 0);
    char * name = duk_get_string(ctx, 1);
    zval * key;
    MAKE_STD_ZVAL(key);

    ZVAL_STRINGL(key, name, strlen(name),  1);
    if(Z_OBJ_HT_P(value)->has_property(value, key,2, NULL TSRMLS_CC))
        duk_push_true(ctx);
    else {
        // check if method exist return NULL pointer if not exist
        if(Z_OBJ_HT_P(value)->get_method(&value,name,strlen(name), NULL TSRMLS_DC) != NULL){
            duk_push_true(ctx);
        }else{
            duk_push_false(ctx);
        }
    }
    zval_ptr_dtor(&key);
    return 1;
}

duk_ret_t phpjs_obj_set_function(duk_context *ctx) {
    zval * value = duk_get_pointer(ctx, 0);
    char * name = duk_get_string(ctx, 1);
    zval * key;
    zval *val;
    int returnVal = 1;
    MAKE_STD_ZVAL(key);
    MAKE_STD_ZVAL(val);

    duk_to_zval(&val,ctx, 2);
    ZVAL_STRINGL(key, name, strlen(name),  1);
    
    Z_OBJ_HT_P(value)->write_property(value, key,val, NULL TSRMLS_CC);
    if (EG(exception) != NULL) {
            returnVal = DUK_RET_INTERNAL_ERROR;
            // There was an exception in the PHP side, let's catch it and throw as a JS exception
            duk_push_string(ctx, Z_EXCEPTION_PROP("message"));
            zend_clear_exception(TSRMLS_C);
            duk_throw(ctx);
    }else{
        duk_push_true(ctx);
    }
    zval_ptr_dtor(&key);
    return returnVal;
}

duk_ret_t phpjs_obj_delete_function(duk_context *ctx) {
    zval * value = duk_get_pointer(ctx, 0);
    char * name = duk_get_string(ctx, 1);
    zval * key;
    int returnVal = 1;
    MAKE_STD_ZVAL(key);
    ZVAL_STRINGL(key, name, strlen(name),  1);
    
    Z_OBJ_HT_P(value)->unset_property(value, key, NULL TSRMLS_CC);
    if (EG(exception) != NULL) {
            returnVal = DUK_RET_INTERNAL_ERROR;
            // There was an exception in the PHP side, let's catch it and throw as a JS exception
            duk_push_string(ctx, Z_EXCEPTION_PROP("message"));
            zend_clear_exception(TSRMLS_C);
            duk_throw(ctx);
    }else{
        duk_push_true(ctx);
    }
    zval_ptr_dtor(&key);
    return returnVal;
}


duk_ret_t php_object_handler(duk_context *ctx) {
    if (duk_is_constructor_call(ctx)) {
        duk_push_string(ctx, "Can not call this function as constructor");
        duk_throw(ctx);
        return DUK_RET_ERROR;
    }
    int returnVal = DUK_EXEC_SUCCESS,i,args = duk_get_top(ctx); /* function args */
    zval *func, *value, *retval, *params[args];

    MAKE_STD_ZVAL(func);
    MAKE_STD_ZVAL(retval);
    MAKE_STD_ZVAL(value);

    /* get Resource Value (zval *) */
    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, "__zval__");
    value = (zval *) duk_get_pointer(ctx, -1);
    duk_pop(ctx);

    /* get function name  */
    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, "__function");
    duk_to_zval(&func,ctx,-1);
    duk_pop(ctx);
    
    /* get arguments */
    for(i=0; i<args;i++){
        zval *val;
        MAKE_STD_ZVAL(val);
        duk_to_zval(&val,ctx,i);
        params[i] = val;
    }
    /* end get */

    /* EXEC Object Handler */
    {
        
        // exec PHP function : __invoke
        TSRMLS_FETCH();
        if(call_user_function(NULL, &value, func, retval, args, params TSRMLS_CC) != SUCCESS){
            returnVal = DUK_ERR_ERROR;
            duk_push_error_object(ctx, DUK_ERR_ERROR, "Unknown function: \"%s\"", Z_STRVAL_P(func));
            duk_throw(ctx);
        };

        if (EG(exception) != NULL) {
            returnVal = DUK_RET_INTERNAL_ERROR;
            // There was an exception in the PHP side, let's catch it and throw as a JS exception
            duk_push_string(ctx, Z_EXCEPTION_PROP("message"));
            zend_clear_exception(TSRMLS_C);
            duk_throw(ctx);
        }
        if(returnVal == DUK_EXEC_SUCCESS){
            returnVal = Z_TYPE_P(retval) == 0 ? 0 : 1;
            zval_to_duk(ctx, NULL, retval);
        }
    }

    zval_ptr_dtor(&func);
    zval_ptr_dtor(&retval);
    /* END EXEC */

    /* free args */
    for(i=0; i<args;i++){
        if(Z_TYPE_P(params[i]) != IS_RESOURCE)
            zval_ptr_dtor(&params[i]);
    }
    return returnVal;
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
    zval *val,*udata,*retval, *func, *params[1];
    //php_js_t* obj;

    MAKE_STD_ZVAL(func);
    MAKE_STD_ZVAL(retval);
    MAKE_STD_ZVAL(udata);
    MAKE_STD_ZVAL(val);

    duk_to_zval(&val,ctx,0); // get ID
    params[0] = val;



    /* set function name */
    char * str = "JSModSearch";
    ZVAL_STRINGL(func, str, strlen(str),  1);

    /* found zval object JS */
    duk_push_global_object(ctx);
    duk_memory_functions mem;
    duk_get_memory_functions(ctx, &mem);
    udata = mem.udata;
    duk_pop(ctx);
    
    /* exec PHP function : JSModSearch */
    TSRMLS_FETCH();
    int ret = FAILURE;

    ret = call_user_function(NULL, &udata, func, retval, 1, params TSRMLS_CC);
    if (EG(exception) != NULL){
        // ignore error if JSModSearch not found in global stack
        zend_clear_exception(TSRMLS_C);
        ret = FAILURE;
    }

    if(ret != SUCCESS && call_user_function(CG(function_table), NULL, func, retval, 1, params TSRMLS_CC) != SUCCESS) {
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