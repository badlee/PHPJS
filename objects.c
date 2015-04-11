#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "duktape.h"
#include "phpjs.h"

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

ZEND_METHOD(JSObjectWrapper, __get)
{
    FETCH_THIS_WRAPPER
    char * varname;
    int varname_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &varname, &varname_len) == FAILURE)
        return;

    duk_push_heapptr(ctx, obj->function);
    duk_get_prop_string(ctx, -1, varname);
    duk_to_zval(&return_value, ctx, -1);
    php_duk_free_return(ctx);
}

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS___get, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

static zend_function_entry phpjs_JSObjectWrapper_functions[] = {
    ZEND_ME(JSObjectWrapper, __get, ai_phpjs_JS___get, ZEND_ACC_PUBLIC)
    ZEND_FE_END
};

void php_register_object_handler(TSRMLS_D)
{
    zend_class_entry _ce;
    INIT_CLASS_ENTRY(_ce, "JSObjectWrapper", phpjs_JSObjectWrapper_functions);
    phpjs_JSObjectWrapper_ptr = zend_register_internal_class(&_ce TSRMLS_CC);
    phpjs_JSObjectWrapper_ptr->create_object = phpjs_function_new;
}
