--TEST--
Load Js modules Fails
--FILE--
<?php 
// For use require you must implement : String JSModSearch($id)
// if not implemented all require fails
/*
Example of JSModSearch
function JSModSearch($id){
	switch($id){
		case "local":
			return file_get_contents('/tmp/local.js');
		case "distant":
			return file_get_contents('http://www.domaine.io/module.js');
		case "lib/http/client.js":
			return "module.exports = function(http,port){
				this.port=port||80;
				this.server = host || 'localhost';
			};
			module.exports.prototype = {
				request:function(){ return 'not yet implemented!';}
			};";
		case 'foo':
			return 'exports.hello = function() { print("Hello from foo!"); };';
		case 'bar':
			return 'exports.hello = function() { print("Hello from bar!"); };';
		default: //lookup in database
			try {
			   $dbh = new PDO('mysql:host=localhost;dbname=test', $user, $pass);
			   $stmt = $dbh->prepare("SELECT * FROM REGISTRY where nom = ? LIMIT 1");
			   if ($stmt->execute(array($id))) {
					$row = $stmt->fetch(PDO::FETCH_LAZY);
					if($row->isShared){
						$stmt = $dbh->prepare("UPDATE REGISTRY set nAcessTime = nAcessTime+1, dLastAcess = ?");
						$stmt->execute(array(date("d/m/Y H:i:s [U]")));
						return $row->src;
					}
				}
			   $dbh = null;
			} catch (PDOException $e) {
				// can't load js
			}
		// return NULL for not found
	}
}
/*/
$js = new JS();
$JS = '
try{
	var http = require("lib/http/client.js");
}catch(e){
	 print("expected exception: " + e.message);
}
';
$js->evaluate($JS);
?>
--EXPECTF--
expected exception: module not found: "lib/http/client.js"