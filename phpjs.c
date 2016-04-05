#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "duktape.h"
#include "php_phpjs.h"

duk_ret_t duk_php_print(duk_context * ctx)
{
    int args = duk_get_top(ctx);
    int i;

    for (i = 0; i < args; i++) {
        php_printf(i == args - 1 ? "%s\n" : "%s ", duk_safe_to_string( ctx, i ));
    }

    duk_push_true(ctx);

    return 1;
}
int LAMBDA_CALLBACK_INDEX = -1000;

void duk_php_init(php_js_t* obj)
{
    duk_push_global_object(obj->ctx);
    duk_push_c_function(obj->ctx, duk_php_print, DUK_VARARGS);
    duk_put_prop_string(obj->ctx, -2, "print");
    duk_pop(obj->ctx);
}

void duk_php_throw(duk_context * ctx, duk_idx_t idx TSRMLS_DC)
{
    char * js_stack, * message;
    zval * tc_ex;
    MAKE_STD_ZVAL(tc_ex);
    object_init_ex(tc_ex, phpjs_JSException_ptr);


    duk_get_prop_string(ctx, idx, "stack");
    js_stack = estrdup(duk_safe_to_string(ctx, -1));
    duk_pop(ctx); 

    message = estrdup(duk_safe_to_string(ctx, idx));
    duk_pop(ctx); 

    SET_PROP(tc_ex, phpjs_JSException_ptr, "message", message);
    SET_PROP(tc_ex, phpjs_JSException_ptr, "js_stack", js_stack);

    zend_throw_exception_object(tc_ex TSRMLS_CC);

    efree(js_stack);
    efree(message);
}

void zval_to_duk(duk_context * ctx, char * name, zval * value)
{
    /*
    DONE : IS_NULL     0
    DONE : IS_LONG     1
    DONE : IS_DOUBLE   2
    DONE : IS_BOOL     3
    DONE : IS_ARRAY    4
    INPROGRESS : IS_OBJECT   5
    DONE : IS_STRING   6
    DONE : IS_RESOURCE 7
    TODO : IS_CONSTANT 8
    TODO : IS_CONSTANT_ARRAY   9
    TODO : IS_CALLABLE 10
    */
    switch (Z_TYPE_P(value)) {
    case IS_ARRAY: {
        zval ** data;
        HashTable *myht = Z_ARRVAL_P(value);
        char *str_index;
        uint str_length;
        ulong num_index;
        duk_idx_t arr_idx = duk_push_php_array_or_object(ctx, myht);

        for (zend_hash_internal_pointer_reset(myht);
               zend_hash_get_current_data(myht, (void **) &data) == SUCCESS;
               zend_hash_move_forward(myht)
           ) {
            zval_to_duk(ctx, NULL, *data);
            switch (zend_hash_get_current_key_ex(myht, &str_index, &str_length, &num_index, 0, NULL)) {
            case HASH_KEY_IS_LONG:
                duk_put_prop_index(ctx, arr_idx, num_index);
                break;
            case HASH_KEY_IS_STRING:
                duk_put_prop_string(ctx, arr_idx, str_index);
                break;
            }

        }
        break;
    }
    case IS_STRING:
        duk_push_string(ctx, Z_STRVAL_P(value));
        break;
    case IS_LONG:
        duk_push_int(ctx, Z_LVAL_P(value));
        break;
    case IS_DOUBLE:
        duk_push_number(ctx, Z_DVAL_P(value));
        break;
    case IS_BOOL:
        if (Z_BVAL_P(value)) {
            duk_push_true(ctx);
        } else {
            duk_push_false(ctx);
        }
        break;
    case IS_NULL:
        duk_push_null(ctx);
        break;
    case IS_RESOURCE:
        duk_push_pointer(ctx, Z_RESVAL_P(value));
        break;
    case IS_OBJECT: {
            zval *val;
            MAKE_STD_ZVAL(val);
            TSRMLS_FETCH();
            ZVAL_ZVAL(val,value,1,0);
            if(zend_is_callable(val, 0, NULL TSRMLS_CC)){
                duk_push_c_function(ctx, php_object_handler, DUK_VARARGS);
                duk_push_string(ctx, "__invoke");
                duk_put_prop_string(ctx, -2, "__function");
                duk_push_pointer(ctx, val);
                duk_put_prop_string(ctx, -2, "__zval__");
                // * /                
            }else{
                // create a proxy for handle value and method
                duk_int_t rc,args = 1;
                char * className ;
                zend_uint classNameLength;  
                duk_push_string(ctx, "(function(r,c) { return Duktape.initPHPObj(r,c); })");
                duk_peval(ctx); 
                duk_push_int(ctx, LAMBDA_CALLBACK_INDEX++);
                duk_push_pointer(ctx, val);
                // TODO : Get Class Name
                int ret =FAILURE;
                ret = zend_get_object_classname(value,&className,&classNameLength);
                if(ret == SUCCESS){
                    args++;
                    duk_push_string(ctx,className);
                }
                rc = duk_pcall_method(ctx, args);
                if (rc != DUK_EXEC_SUCCESS) {
                    duk_push_string(ctx, duk_to_string(ctx, -1));
                    duk_throw(ctx);
                }
            }
            break;
        }
    }

    if (name) {
        // if there is a name, then create a variable
        // otherwise just put the value to the stack (most likely to return;)
        duk_put_global_string(ctx, name);
    }
}

duk_idx_t duk_push_php_array_or_object(duk_context * ctx, HashTable * myht)
{
    char *str_index;
    uint str_length;
    ulong num_index;
    zval ** data;

    for (zend_hash_internal_pointer_reset(myht);
            zend_hash_get_current_data(myht, (void **) &data) == SUCCESS;
            zend_hash_move_forward(myht)
        ) {
        switch (zend_hash_get_current_key_ex(myht, &str_index, &str_length, &num_index, 0, NULL)) {
        case HASH_KEY_IS_STRING:
            return duk_push_object(ctx);
        }
    }

    return duk_push_array(ctx);
}

static int duk_is_php_object(duk_context * ctx, duk_idx_t idx)
{
    if (duk_is_array(ctx, idx)) {
        return 0;
    }

    int cmp;

    duk_get_prop_string(ctx, idx, "constructor");
    duk_get_prop_string(ctx, -1, "name");
    cmp = strcmp("Object", duk_safe_to_string(ctx, -1));
    duk_pop_2(ctx);

    return cmp != 0;
}

void duk_to_zval(zval ** var, duk_context * ctx, duk_idx_t idx)
{
    duk_size_t len;
    char * str;
    switch (duk_get_type(ctx, idx)) {
    case DUK_TYPE_UNDEFINED:
    case DUK_TYPE_NULL:
    case DUK_TYPE_NONE:
        ZVAL_NULL(*var);
        break;

    case DUK_TYPE_OBJECT: {
        if (duk_is_function(ctx, idx)) {
            TSRMLS_FETCH();
            object_init_ex(*var, phpjs_JSFunctionWrapper_ptr);
            phpjs_add_duk_context(*var, ctx, idx TSRMLS_CC);
            break;
        } else if (duk_is_php_object(ctx, idx)) {
            TSRMLS_FETCH();
            object_init_ex(*var, phpjs_JSObjectWrapper_ptr);
            phpjs_add_duk_context(*var, ctx, idx TSRMLS_CC);
            break;
        }

        // It's a hash or an array (AKA a PHP array)
        duk_idx_t idx1;
        duk_enum(ctx, idx, DUK_ENUM_OWN_PROPERTIES_ONLY);
        idx1 = duk_normalize_index(ctx, -1);
        array_init(*var);

        while (duk_next(ctx, idx1, 1 /*get_value*/)) {
            zval * value;
            MAKE_STD_ZVAL(value);
            duk_to_zval(&value, ctx, -1);

            if(duk_get_type(ctx, -2) == DUK_TYPE_NUMBER) {
                add_index_zval(*var, duk_get_number(ctx,-2), value);
            } else {
                add_assoc_zval(*var, duk_get_string(ctx, -2), value);
            }

            duk_pop_2(ctx);  /* pop_key */
        }
        duk_pop(ctx);
        break;
    }

    case DUK_TYPE_NUMBER:
        ZVAL_DOUBLE(*var, duk_get_number(ctx, idx));
        break;

    case DUK_TYPE_BOOLEAN:
        if (duk_get_number(ctx, idx)) {
            ZVAL_TRUE(*var);
        } else {
            ZVAL_FALSE(*var);
        }
        break;

    case DUK_TYPE_STRING:
        str = duk_get_lstring(ctx, idx, &len);
        ZVAL_STRINGL(*var, str, len,  1);
        break;
    case DUK_TYPE_POINTER:
        ZVAL_RESOURCE(*var,duk_get_pointer(ctx, idx));
        break;
    }
}

duk_ret_t php_get_function_wrapper(duk_context * ctx)
{
    char * fnc = "";
    int catch = 0,
        returnVal = 1,
        i=0,
        args = duk_get_top(ctx); /* function args */
    zval *retval, *func, *params[args];

    /* add params */
    for(i=0; i<args;i++){
        zval *val;
        MAKE_STD_ZVAL(val);
        duk_to_zval(&val,ctx,i);
        params[i] = val;
    }
    /* end add */
    MAKE_STD_ZVAL(func);
    MAKE_STD_ZVAL(retval);


    /* get function name (it's a __function property) */
    duk_push_current_function(ctx);
    duk_get_prop_string(ctx, -1, "__function");
    duk_to_zval(&func, ctx, -1);
    duk_pop(ctx);
    /* we got already the function name */
    //php_printf("Exec ZEND_FNC %s\n",Z_STRVAL_P(func));
    TSRMLS_FETCH();
    if(call_user_function(CG(function_table), NULL, func, retval, args, params TSRMLS_CC) != SUCCESS) {
        returnVal = 0;
        duk_push_error_object(ctx, DUK_ERR_ERROR, "Unknown PHP function: \"%s\"", Z_STRVAL_P(func));
        duk_throw(ctx);
    }

    if (EG(exception) != NULL) {
        catch = 1;
        returnVal = 0;
        /* There was an exception in the PHP side, let's catch it and throw as a JS exception */
        duk_push_string(ctx, Z_EXCEPTION_PROP("message"));
        zend_clear_exception(TSRMLS_C);
    }
    if (catch)
        duk_throw(ctx);
    //php_printf("End Exec ZEND_FNC WITH %u\n",Z_TYPE_P(retval));
    
    if(!catch && returnVal){
        returnVal = Z_TYPE_P(retval) == 0 ? 0 : 1;
        zval_to_duk(ctx, NULL, retval);
    }
    zval_ptr_dtor(&func);
    zval_ptr_dtor(&retval);
    /* free args */
    for(i=0; i<args;i++){
        if(Z_TYPE_P(params[i]) != IS_RESOURCE)
            zval_ptr_dtor(&params[i]);
    }

    return !catch ? returnVal : 1;
}

duk_ret_t duk_set_into_php(duk_context * ctx)
{
    zval * value;
    char * name;

    MAKE_STD_ZVAL(value);
    duk_to_zval(&value, ctx, 2);

    name = estrdup(duk_get_string(ctx, 1) + 1);

    TSRMLS_FETCH();
    zend_hash_update(EG(active_symbol_table), name, strlen(name)+1, &value, sizeof(zval*), NULL);
    duk_push_true(ctx);

    return 1;
}


duk_ret_t duk_get_from_php(duk_context * ctx)
{
    char * name = duk_get_string(ctx, 1);

    if (name[0] == '$') {
        zval ** value;
        TSRMLS_FETCH();
        if(zend_hash_find(EG(active_symbol_table), name+1, strlen(name), (void **) &value) == SUCCESS) {
            zval_to_duk(ctx, NULL, *value);
        } else {
            duk_push_undefined(ctx);
        }
    } else {
        // they expect a function wrapper
        duk_push_c_function(ctx, php_get_function_wrapper, DUK_VARARGS);
        duk_push_string(ctx, name);
        duk_put_prop_string(ctx, -2, "__function");
    }


    return 1;
}

