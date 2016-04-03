--TEST--
global and local module loader
--FILE--
<?php 
/* global module loader for all JS context */
function JSModSearch($id){
	if ($id === 'foo') {
        return 'exports.hello = function() { print("Hello from foo!"); };';
    } else if ($id === 'bar') {
        return 'exports.hello = function() { print("Hello from bar!"); };';
    }
};

/* shared PHP vars and function */

$JS = '
try{
    var foo = require("foo");
    foo.hello();    
}catch(e){
    print("module foo not found");
}
try{
    var bar = require("bar");
    bar.hello();    
}catch(e){
    print("module bar not found");
}
try{
    var baz = require("baz");
    baz.hello();    
}catch(e){
    print("module baz not found");
}
';


$js = new JS();
$js->evaluate($JS);

$js2 = new JS();
/* if local module loader fail the global loader will never executed */
$js2->JSModSearch = function($id){
    if ($id === 'foo') {
        return 'exports.hello = function() { print("Hello from local foo!"); };';
    } else if ($id === 'baz') {
        return 'exports.hello = function() { print("Hello from local baz!"); };';
    }
};
$js2->evaluate($JS);

?>
--EXPECTF--
Hello from foo!
Hello from bar!
module baz not found
Hello from local foo!
module bar not found
Hello from local baz!