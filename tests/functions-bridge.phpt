--TEST--
TODO - check, access and register PHP functions
--SKIPIF--
<?php if (TRUE) print "skip"; ?>
--FILE--
<?php 
$JS = '
	if (PHP.function_exists("print_r")) // check if print_r is defined
	    PHP.print_r("OK\n"); // access to a PHP function
	// Register PHP function
	PHP.register_function("echoJS",function(){
		var objects = [];
	    for (var i = 0; i < arguments.length; i++)
	      objects.push(arguments[i]);
	    print(objects.join(" "));
	});
';
$js = new JS();
$js->evaluate($JS);
echoJS("PHP_VERSION_ID",PHP_VERSION_ID);
?>
--EXPECTF--
OK
PHP_VERSION_ID %d