[![Build Status](https://travis-ci.org/badlee/PHPJS.svg)](https://travis-ci.org/badlee/PHPJS)

# PHPJS

***This is experimental***

Run javascript inside PHP, powered by the awesome [Duktape](http://duktape.org) Javascript engine.

Why?
----

1. It's fun!
2. Javascript is becoming mainstream, with hundreds of libraries. Having an easy way of sharing code with Javascript natively makes using
3. 

How to install it?
------------------

```bash
phpize
./configure --enable-phpjs
make install
```

Then add `extension=phpjs.so` to your php.ini

How to use it?

There is `JS` class, each instance runs it's own Javascript *virtual machine* (or duktape's context).


```php
$js1 = new JS;
$js1->load("foobar.js");
```

The `JS` object is like a proxy between the `Javascript` and the `PHP` userlands. In the PHP side, the `JS` looks like an Array or an object.

```php
$js['foobar'] = 1; // Set `foobar` inside Javascript (global variable)
var_dump($js['XXX']); // Read global variable 'XXX' from javascript
$js->fnc(); // Call fnc() function from Javascript.
```

Inside `Javascript` there are also some intergration. For instance we have a `PHP` (or `$PHP`) global object.

```js
PHP.var_dump("something"); // Call a PHP function!
PHP.$something = 1; // set a variable
print(PHP.$something_else); // read a variable from PHP
```

## Documentation

### JS Object (API)


#### __constructor

**@PARAMS**
	$allowedVarAndFunc (Optional) : String[]
		Array to PHP function and global variables updatable via PHP's object

```PHP
$VM = new JS(/*$allowedVarAndFunc*/);
// the VM can handle any PHP function and get/set any PHP global variables

$VM = new JS(['$myVar','$args','$argv','apache_getenv','apache_get_module','headers','date']);
// the VM can handle only PHP function and get/set any PHP global variables listed
```

#### load

**@PARAMS**
	$filename : String
		path to js source file (Absolute ou Relative)

```PHP
$VM = new JS;
$VM->load("file.js"); // relative path,  throw an error if file not found
$VM->load("/tmp/file2.js"); // with an absolute path
```

#### evaluate

**@PARAMS**
	$jsCode : String
		path to js source to execute

```PHP
$VM = new JS;
$VM->evaluate("var i = 1; print('From JS CODE',i);");
$VM->evaluate("i++;print('From JS CODE',i);"); // allow to share data across code evalution (it's a same context)
```

### Modules

#### Module loading

PHPJS (Duktape) has a built-in minimal module loading framework based on [CommonJS modules version 1.1.1](http://wiki.commonjs.org/wiki/Modules/1.1.1), with additional support for module.exports.

Module is writen in javascript only

```JS
// Module foo/bar
exports.hello = function(){
  print("world");
}
```

You can load modules from javascript code with the global require() function:

```JS
var mod = require('foo/bar');
mod.hello();
```

#### Module lookup function

The modules lookup function is the PHP's global `JSModSearch` or the local function `$JS->JSModSearch`

##### global lookup function

```PHP
// Global lookup function
function JSModSearch($id){
    if ($id === 'foo') {
        return 'exports.hello = function() { print("Hello from foo!"); };';
    } else if ($id === 'bar') {
        return 'exports.hello = function() { print("Hello from bar!"); };';
    }
}
/* shared PHP vars and function */
$js = new JS([]);
$JS = '
var foo = require("foo");
foo.hello();
var bar = require("bar");
bar.hello();
try{
	var nonExistentModule = require("nonExistentModule");
}catch(e){
	 print("expected exception: " + e.message);
}
';
$js->evaluate($JS);
```

##### local lookup function

```PHP
/* shared PHP vars and function */
$js = new JS();
// Local lookup function
$js->JSModSearch = function ($id){
    if ($id === 'foo/bar') {
        return 'exports.hello = function() { print("Hello from foo/bar!"); };';
    } else if ($id === 'bar') {
        return 'exports.hello = function() { print("Hello from bar!"); };';
    }
};

$JS = '
var foo = require("foo");
foo.hello();
';
$js->evaluate($JS);
```

More
----

*For more documentation look tests folder*

TODO
----

1. Better sharing of objects between PHP and Javascript
2. More documentation
3. More integration :-)
