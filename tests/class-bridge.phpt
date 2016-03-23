--TEST--
TODO - check, access, register and instanciate PHP class
--SKIPIF--
<?php if (TRUE) print "skip"; ?>
--FILE--
<?php 
class MyPHPClass extends stdClass {
	public function __construct($properties){
		foreach ($properties as $key => $value) {
			$this->$key = $value;
		}
	}
}

$JS = <<<JS
	// custom JS class ES5
	var Person = function(name,age){
		if ( !(this instanceof Person) ) {
			return new Person( name||"",age||0 );
		}
		this._name = name || "";
		this._age  = age || 0;
	};
	Person.prototype = {
		_name : "", 
    	_age  : 0, 
		compare : function(p1,p2){
			if ( !(p1 instanceof Person) )
				throw new Error("Argument 1 is not a Person");
			if ( !(p2 instanceof Person) )
				throw new Error("Argument 2 is not a Person");
			return p1._age > p2._age
		},
		setAge :function(a){
			this._age = a;
		},
		setName : function(n){
			this._name = n;
		},
		get name(){
			return this._name; 
		},
		get age(){
			return this._age; 
		},
		get info(){
			return this._name + " " + this._age; 
		},
		typeof : function(){
			return "Person";
		}
	};
	print(Duktape.typeof(new Person)); // custom typeof for return the typeof property like toString for String and toJSON for json 

	if (PHP.class_exists("MyPHPClass")) // check if stdClass is defined
	    PHP.print_r("OK\\n"); // access to a PHP function
	// Register PHP function
	PHP.register_class("jsDate",Date);
	PHP.register_class("MyJSClass",Person);
	PHP.register_class("String",String);

	// instanciate PHP class
	PHP.new("MyPHPClass",[1,2,3,4,54,6,"toto"]);
JS;
$js = new JS();
$js->evaluate($JS);
$a = new MyJSClass("oshimin",31); // create a person object
var_dump($a->name);
$a->setName("badlee");
var_dump($a->age);

$date = new jsDate;
echo "today is ",$date->getDate(),"\n";


?>
--EXPECTF--
person
OK
oshimin
31
today is %d