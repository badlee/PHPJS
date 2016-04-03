--TEST--
Restrict PHP access
--FILE--
<?php 
function criticalFn($str="??"){
	echo $str." call criticalFn from PHP\n";
}
$secure = 'PHP';
$unsecure = 'PHP';
// unrestricted Access
$js = new JS();
$JS = '
PHP.criticalFn("JS");
PHP.var_dump("JS");
PHP.$secure ="JS "+PHP.date("d/m/Y H:i:s");
PHP.$unsecure ="JS "+PHP.date("d/m/Y H:i:s");
';
$js->evaluate($JS);
echo '$secure = ',$secure,' - ','$unsecure = ',$unsecure,"\n";
// restricted Access
$js2 = new JS([
	/* Can allow only a global variable */
	'$unsecure', // allow access to $unsecure variable
	/* Can allow any defined functions */
	'date', // allow access to date function
	//'var_dump',
	//'criticalFn'
]);
$JS = '
try{
	PHP.criticalFn("JS");
	print("criticalFn is allowed");
}catch(e){
	print("criticalFn is not allowed");
}

try{
	PHP.var_dump("JS");
	print("var_dump is allowed");
}catch(e){
	print("var_dump is not allowed");
}

PHP.$secure ="JS2 "+PHP.date("d/m/Y H:i:s");
PHP.$unsecure ="JS2 "+PHP.date("d/m/Y H:i:s");
';
$js2->evaluate($JS);
echo '$secure = ',$secure,' - ','$unsecure = ',$unsecure,"\n";

?>
--EXPECTF--
JS call criticalFn from PHP
string(2) "JS"
$secure = JS %d/%d/%d %d:%d:%d - $unsecure = JS %d/%d/%d %d:%d:%d
criticalFn is not allowed
var_dump is not allowed
$secure = JS %d/%d/%d %d:%d:%d - $unsecure = JS2 %d/%d/%d %d:%d:%d