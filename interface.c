#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "duktape.h"
#include "php_phpjs.h"

PHPAPI zend_class_entry *phpjs_JSException_ptr;
PHPAPI zend_class_entry *phpjs_JS_ptr;

typedef struct {
    zend_object zo;
    duk_context * ctx;
} php_js_t;


ZEND_METHOD(JS, evaluate)
{
    FETCH_THIS;

    char * str;
    int len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &len) == FAILURE)
        return;

    if (duk_peval_lstring(ctx, str, len) != 0) {
        duk_php_throw(ctx, -1 TSRMLS_CC);
        RETURN_FALSE;
    }

    duk_to_zval(&return_value, ctx, -1);
    duk_pop(ctx);
}

// public function load($path)
ZEND_METHOD(JS, __construct)
{
    FETCH_THIS_EX(0);
    obj->ctx = duk_create_heap(NULL, NULL, NULL, getThis(), NULL);
    duk_php_init(obj->ctx);
    // set API
    {
        zval* a_value;
        zval* require_fn;
        zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zz", &a_value, &require_fn);
        if(IS_ARRAY ==  Z_TYPE_P(a_value)){
            zval_to_duk(obj->ctx, "API", a_value);
        } else {
            duk_push_global_object(obj->ctx);
            duk_push_true(obj->ctx);
            duk_put_global_string(obj->ctx, "API");
            duk_pop(obj->ctx); 
        }
    }

    duk_push_global_object(obj->ctx);
    duk_push_c_function(obj->ctx, php_mod_search_handler, 1 /*nargs*/);
    duk_put_prop_string(obj->ctx, -2, "__require_from_php");
    duk_pop(obj->ctx);
    
    duk_push_global_object(obj->ctx);
    duk_push_c_function(obj->ctx, duk_set_into_php, DUK_VARARGS);
    duk_put_prop_string(obj->ctx, -2, "__set_into_php");
    duk_pop(obj->ctx);

    duk_push_global_object(obj->ctx);
    duk_push_c_function(obj->ctx, duk_get_from_php, DUK_VARARGS);
    duk_put_prop_string(obj->ctx, -2, "__get_from_php");
    duk_pop(obj->ctx);

    duk_push_string(obj->ctx,
    "var PHP,$PHP;" \
    "(function(){" \
    "   var api = API;" \
    "   var get_from_php = __get_from_php;" \
    "   var set_into_php = __set_into_php;" \
    "   var require_from_php = __require_from_php;" \
    "   API = __require_from_php = __set_into_php = __get_from_php = undefined;" \
    "   delete API;" \
    "   delete __set_into_php;" \
    "   delete __require_from_php;" \
    "   delete __get_from_php;" \
    "   $PHP = {" \
    "       set: function(self,name) {" \
    "           var b = []; for(var i = 0;i<arguments.length;i++)b[i] = arguments[i];"
    "           return (typeof api == 'boolean' && api === true) || (api.length && api.indexOf(name) != -1) ? set_into_php.apply(this,b) : undefined;" \
    "       }," \
    "       get: function(self,name) {" \
    "           var b = []; for(var i = 0;i<arguments.length;i++)b[i] = arguments[i];"
    "           return (typeof api == 'boolean' && api === true) || (api.length && api.indexOf(name) != -1) ? get_from_php.apply(this,b) : undefined;" \
    "       }" \
    "    };" \
    "   if(typeof api == 'boolean' && api === true){" \
    "       $PHP.get = get_from_php;" \
    "       $PHP.set = set_into_php;" \
    "   }" \
    "   $PHP = new Proxy({},$PHP);" \
    "   PHP = $PHP;" \
    "   Duktape.modSearch = function (id,require, exportsOrg) {" \
    "        var module = {exports:null},exports={},ret = require_from_php(String(id));" \
    "        if (ret && typeof ret === 'string'){" \
    "            try{"\
    "               (new Function('module,exports',ret))(module,exports);" \
    "               exports = module.exports || exports;" \
    "               for(var i in exports) exportsOrg[i] = exports[i];"\
    "               module = exports = null;"\
    "               return;"\
    "             }catch(e){throw 'module '+id+': ' + e.message;}" \
    "        }" \
    "        throw new Error('module not found: \"' + id+'\"');" \
    "    };" \
    "})()"
    );

    if (duk_peval(obj->ctx) != 0) {
        printf("eval failed: %s\n", duk_safe_to_string(obj->ctx, -1));
    }
    duk_pop(obj->ctx);
}

ZEND_METHOD(JS, load)
{
    FETCH_THIS;
    char * varname;
    int varname_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &varname, &varname_len) == FAILURE) {
        return;
    }

    if (duk_peval_file(ctx, varname) != 0) {
        duk_php_throw(ctx, -1 TSRMLS_CC);
        RETURN_FALSE;
    }

    duk_to_zval(&return_value, ctx, -1);
    duk_pop(ctx);
}

ZEND_METHOD(JS, offsetUnset)
{
}

ZEND_METHOD(JS, offsetExists)
{
    FETCH_THIS;
    char * varname;
    int varname_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &varname, &varname_len) == FAILURE)
        return;

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, varname);
    if (duk_get_type_mask(ctx, -3) & DUK_TYPE_UNDEFINED) {
        RETURN_FALSE;
    } else {
        RETURN_TRUE;
    }
}

ZEND_METHOD(JS, offsetGet)
{
    FETCH_THIS;
    char * varname;
    int varname_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &varname, &varname_len) == FAILURE)
        return;

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, varname);
    duk_to_zval(&return_value, ctx, -1);
}


// public function __get($name)
ZEND_METHOD(JS, __get)
{
    FETCH_THIS;
    char * varname;
    int varname_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &varname, &varname_len) == FAILURE)
        return;

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, varname);
    duk_to_zval(&return_value, ctx, -1);
    php_duk_free_return(ctx);
}

// public function __set($name, $value)
ZEND_METHOD(JS, __set)
{
    FETCH_THIS;
    zval* a_value;
    char * name;
    int len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &a_value) == FAILURE)
        return;

    zval_to_duk(ctx, name, a_value);
}

ZEND_METHOD(JS, offsetSet)
{
    FETCH_THIS;
    zval* a_value;
    char * name;
    int len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &a_value) == FAILURE)
        return;

    zval_to_duk(ctx, name, a_value);
}


// public function __call($name, $args)
ZEND_METHOD(JS, __call)
{
    FETCH_THIS;
    zval* a_args;
    char * fnc;
    int lfnc;
    int ind;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &fnc, &lfnc, &a_args) == FAILURE)
        return;

    duk_push_global_object(ctx);
    phpjs_php__call(ctx, fnc, a_args, return_value TSRMLS_CC); 
}

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS_evaluate, 0, 0, 1)
    ZEND_ARG_INFO(0, str)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS_offsetExists, 0, 0, 1)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS_offsetGet, 0, 0, 1)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS_offsetSet, 0, 0, 2)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS_offsetUnset, 0, 0, 1)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS_load, 0, 0, 1)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS_export, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS___get, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS___set, 0, 0, 2)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_phpjs_JS___call, 0, 0, 2)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()

    static zend_function_entry phpjs_JS_functions[] = {
        ZEND_ME(JS, __construct, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
        ZEND_ME(JS, evaluate, ai_phpjs_JS_evaluate, ZEND_ACC_PUBLIC)
        ZEND_ME(JS, load, ai_phpjs_JS_load, ZEND_ACC_PUBLIC)
        ZEND_ME(JS, offsetExists, ai_phpjs_JS_offsetExists, ZEND_ACC_PUBLIC)
        ZEND_ME(JS, offsetGet, ai_phpjs_JS_offsetGet, ZEND_ACC_PUBLIC)
        ZEND_ME(JS, offsetSet, ai_phpjs_JS_offsetSet, ZEND_ACC_PUBLIC)
        ZEND_ME(JS, offsetUnset, ai_phpjs_JS_offsetUnset, ZEND_ACC_PUBLIC)
        ZEND_ME(JS, __get, ai_phpjs_JS___get, ZEND_ACC_PUBLIC)
        ZEND_ME(JS, __set, ai_phpjs_JS___set, ZEND_ACC_PUBLIC)
        ZEND_ME(JS, __call, ai_phpjs_JS___call, ZEND_ACC_PUBLIC)
        ZEND_FE_END
    };

static zend_function_entry phpjs_functions[] = {
    {NULL, NULL, NULL}
};


static void php_js_free_storage(php_js_t *obj TSRMLS_DC)
{
    zend_object_std_dtor(&obj->zo TSRMLS_CC);
    if (obj->ctx) {
        duk_destroy_heap(obj->ctx);
    }
    efree(obj);
}


zend_object_value phpjs_new_vm(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    php_js_t * obj;

    obj = (php_js_t *) emalloc(sizeof(php_js_t));
    memset(obj, 0, sizeof(php_js_t));
    zend_object_std_init(&obj->zo, ce TSRMLS_CC);
#if PHP_VERSION_ID < 50399
    zval * tmp;
    zend_hash_copy(obj->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
#else
    object_properties_init((zend_object*) &(obj->zo), ce);
#endif


    retval.handle = zend_objects_store_put(obj, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)php_js_free_storage, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}


PHP_MINIT_FUNCTION(phpjs)
{
    zend_class_entry _ce, *_if;
    zval* _val;

    INIT_CLASS_ENTRY(_ce, "JSException", NULL);
    phpjs_JSException_ptr = zend_register_internal_class_ex(&_ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
    zend_declare_property_string(phpjs_JSException_ptr, _S("js_stack"), "", ZEND_ACC_PROTECTED TSRMLS_CC);

    INIT_CLASS_ENTRY(_ce, "JS", phpjs_JS_functions);
    phpjs_JS_ptr = zend_register_internal_class(&_ce TSRMLS_CC);
    phpjs_JS_ptr->create_object = phpjs_new_vm;
    zend_do_implement_interface(phpjs_JS_ptr, zend_ce_arrayaccess TSRMLS_CC);


    php_register_object_handler(TSRMLS_C);
    php_register_function_handler(TSRMLS_C);

    return SUCCESS;
}


PHP_MINFO_FUNCTION(phpjs)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "phpjs", "enabled");
    php_info_print_table_row(2, "Version", "1.0");
    php_info_print_table_end();
}

zend_module_entry phpjs_module_entry = {
    STANDARD_MODULE_HEADER,
    "phpjs",
    phpjs_functions,
    PHP_MINIT(phpjs),
    NULL,
    NULL,
    NULL,
    PHP_MINFO(phpjs),
    "1.0",
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PHPJS
ZEND_GET_MODULE(phpjs)
#endif
