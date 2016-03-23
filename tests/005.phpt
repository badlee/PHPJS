--TEST--
Share variables from PHP <-> JS
--FILE--
<?php 
class foo {
	var $o = 12;
    function faire_foo() {
        echo "Faisant foo.";
    }
}
/* minimal modules lookup */
function JSModSearch($id){
	if (file_exists("tests/{$id}.mod.js"))
		return file_get_contents("tests/{$id}.mod.js");
};

/* shared PHP vars and function */
$count = 0;
$shared = "shared php var";
function fnc($str){
	echo "Hello from ",($str ? $str : 'php'),"\n";
};
function increment(){
	global $count;
	$count++;
};
$js = new JS();
$js['i'] = new foo;
$js['inc'] = function(){
	global $count;
	$count++;
};
$JS = '
var count=0;
PHP.$count++;
PHP.increment();
inc();
PHP.var_dump(i.o);
i.o++; // increment
PHP.var_dump(i.o);

PHP.$shared = "modified by JS ("+PHP.$shared+")";
PHP.fnc("JS");

function fnc(str){
	print("Hello from",(str ? str : "JS"));
};

var mod = require("005");
print(mod.uctime);
print(typeof mod.add);
';
$js->evaluate($JS);
echo "$count -> $shared\n";
$js->fnc("PHP");
echo "JS->count = ",$js->count++," in PHP (before increment)\n";
$js->load('tests/005.js');
echo "JS->count = ",$js->evaluate('count')," in JS (after increment)\n";
?>
--EXPECTF--
float(12)
float(13)
Hello from JS
string(10) "module 005"
%d
function
3 -> modified by JS (shared php var)
Hello from PHP
JS->count = 0 in PHP (before increment)
string(11) "file 005.js"
JS->count = 2 in JS (after increment)