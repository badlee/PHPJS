--TEST--
check, access and define PHP constants
--FILE--
<?php 
$JS = '
	if(PHP.defined("PHP_VERSION")) // check if constant exists
		print("PHP_VERSION OK");
	print(PHP.constant("PHP_VERSION_ID")); // access to PHP constant
	PHP.define("LOL","LOL FROM JS"); // define a constant for PHP
	
';
$js = new JS();
$js->evaluate($JS);
echo LOL,"\n";
?>
--EXPECTF--
PHP_VERSION OK
%d
LOL FROM JS