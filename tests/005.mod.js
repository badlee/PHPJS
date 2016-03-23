PHP.var_dump('module 005');
module.exports = {
	now : PHP.date('d/m/Y H:i:s'),
	uctime : PHP.time(),
	add : function exports(x, y) { return x + y; }
}