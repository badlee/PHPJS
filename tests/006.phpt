--TEST--
Share Object from PHP <-> JS
--FILE--
<?php 
class foo {
	static $p = 123;
	var $foo = 'foo';
	var $bar = "bar";
	var $index = 0;
    function fn_foo() {
    	self::$p = $this->index;
        echo "Fn foo.\n",'$index=',$this->index,"\n";
    }
    function fn_bar($o,$str) {
    	$this->index = $o;
    	$this->foo = $str;
    }
    public function __get($name) {
    	if($name=="baz") return "baz";
    }
}

/* shared PHP vars and function */

$js = new JS();
$js['phpObj'] = new foo;
$js['fn'] = function(){
	echo "string\n";
};
$js['j'] = new foo;

$JS = '
// enumerate  properties and methods
for (var k in phpObj)
    print(k); 

print(Object.keys(phpObj));
print(Object.getOwnPropertyNames(phpObj));

// access to properties
print(phpObj.index,phpObj.foo,phpObj.bar);
print(phpObj.baz); // magic property


//set properties
phpObj.foo = "bar";
phpObj.bar = "foo";
phpObj.index++;
print(phpObj.index,phpObj.foo,phpObj.bar);

// call method
phpObj.fn_foo();
phpObj.fn_bar(33,"FOO");
print(phpObj.index,phpObj.foo,phpObj.bar);

// delete properties
delete phpObj.index;
delete phpObj.foo;
delete phpObj.bar;
delete phpObj.baz; // can not delete a magic property

print(phpObj.index,phpObj.foo,phpObj.bar);
print(phpObj.baz); // magic property

// get type and cast to string 
print("toString",phpObj); // return [object foo]
print("typeof",Duktape.typeof(phpObj)); // return foo

';
$js->evaluate($JS);
?>
--EXPECTF--
foo
bar
index
fn_foo
fn_bar
foo,bar,index,fn_foo,fn_bar
foo,bar,index,fn_foo,fn_bar
0 foo bar
baz
1 bar foo
Fn foo.
$index=1
33 FOO foo
null null null
baz
toString [php_object foo]
typeof foo