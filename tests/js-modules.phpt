--TEST--
Load Js modules
--FILE--
<?php 
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
?>
--EXPECTF--
Hello from foo!
Hello from bar!
expected exception: module not found: "nonExistentModule"