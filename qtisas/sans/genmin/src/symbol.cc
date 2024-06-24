
#include "symbol.h"

/*	This class represents the symbols (terminal or non terminal ) 
 *	in the  BNF grammar.
 *	class variables
 *	======================================================================
 *	name: This is a string represents the name of the Symbol.
 *	count_rules: If the symbol is non terminal, this counter represents 
 *	the number of production rules, where the the symbol is on the left side.
 *      is_terminal: A flag which denotes if the symbol is terminal or non terminal.
 * */


/*	The constructor of the class.
 * */
Symbol::Symbol()
{
	name = "";
	count_rules = 0;
	is_terminal = 1;
}


/*	Set the name of the symbol to s and the is_terminal value 
 *	(terminal or non terminal) to status. The user should call
 *	this method after the creation of the Symbol.
 * */
void	Symbol::set(string s,int status)
{
	setName(s);
	setTerminalStatus(status);
}

/*	Set the name of the Symbol to s.
 * */
void	Symbol::setName(string s)
{
	name = s;
}

/*	Return the name of the Symbol.
 * */
string	Symbol::getName() const
{
	return name;
}

/*	Set the value of the terminal status.
 * */
void	Symbol::setTerminalStatus(int status)
{
	is_terminal=status;
}

/*	Return the value of the terminal status.
 * */
int	Symbol::getTerminalStatus() const
{
	return is_terminal;
}

/*	Add a new rule to the rule list and increment the 
 *	total number of rules.
 * */
void	Symbol::addRule(Rule *r)
{
	rule.push_back(r);
	count_rules++;
}

/*	Return a pointer to the rule in position pos.
 *	If there is not a rule, the function returns nullptr.
 * */
Rule	*Symbol::getRule(int pos) const
{
	if(pos<0 || pos>=count_rules) return nullptr;
	return rule[pos];
}

/*	Return the number of rules in the rule list.
 * */
int	Symbol::getCountRules() const
{
	return count_rules;
}

/*	The destructor of the class.
 * */
Symbol::~Symbol()
{
}
