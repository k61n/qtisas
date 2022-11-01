# ifndef __SYMBOL__H
# include <string>
# include <vector>
using namespace std;

class Rule;
 
/*	
 *	This class represents the symbols of the BNF grammar.
 * */
class Symbol
{
	private:
		/*	
		 *	name: The name of the symbol
		 *	rule: A pointer to the array of  production rules of the symbol,
		 *	      if the symbol is non - terminal.
		 *      count_rules: an integer holding the number of production rules.
		 *      is_terminal: a flag which indicates if the symbol is terminal or non terminal.
		 * */
		string name;
		vector<Rule*> rule;
		int	count_rules;
		int	is_terminal;
	public:
		/*	
		 *	Symbol(): The constructor of the class.
		 *	set(s,status):    Set the name to s and is_terminal to status.
		 *	setName(s):	  Set the name of the symbol to s.
		 *	getName():	  Return the name of the symbol.
		 *	setTerminalStatus(status): Set the is_terminal flag to status.
		 *	getTerminalStatus():	Return the is_terminal flag of the symbol.
		 *	addRule(r):	 Add the production rule r to the list of the production
		 *			 rules of the symbol.
		 *      getRule(pos):	 Return a pointer to the production rule in position pos.
		 *      getCountRules(): Return the amount  of production rules in the symbol.
		 *      ~Symbol():	 The destructor of the class.
		 * */
		Symbol();

		void	set(string s,int status);
		void	setName(string s);
		string	getName() const;
		
		void	setTerminalStatus(int status);
		int	getTerminalStatus() const;

		void	addRule(Rule *r);
		Rule	*getRule(int pos) const;
		int	getCountRules() const;
		~Symbol();
		
};

# define __SYMBOL__H
# endif
