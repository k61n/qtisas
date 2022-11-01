# ifndef __RULE__H
# include <symbol.h>
# include <vector>
/*
 * 	This is the maximum number of wrapping events
 * 	for the Grammatical Evolution procedure.
 */
# define REDO_MAX	5
using namespace std;

/*	19-07-2007
 *	This class represents the production rules
 *	of the corresponding BNF grammar.
 * */
class Rule
{
	private:
		/*	
		 *	data: The right hand symbols for this production rule.
		 *	length: The amount of symbols in the production rule.
		 * */
		vector<Symbol*> data;	
		int	length;
	public:
		/*	
		 *	Rule(): The construtor of the production rule.
		 *	addSymbol(s): Add a symbol to the list of right hand symbols of the rule.
		 *	getSymbolPos(s): Return the position of the symbol with name s.
		 *	getSymbol(pos): Return a pointer to the symbol in position pos of the 
		 *			right hand symbols.
		 *	setSymbol(pos,s): Set the name of the symbol in position pos to s.
		 *	getLength(): Return the amount of symbols in the production rule.
		 *	printRule(genome,pos,length): Return a string  constructed for the 
		 *				      chromosome genome starting from the position
		 *				      pos of the production rule. 
		 *	                              The integer redo increases every time that 
		 *	                              a wrapping efftect it takes place.
		 *	~Rule(): The destructor of the production rule.
		 * */
		Rule();
		void	addSymbol(Symbol *s);
		int	getSymbolPos(string s);
		Symbol	*getSymbol(int pos) const;
		void	setSymbol(int pos,Symbol *s);
		int	getLength() const;
		string	printRule(vector<int> genome,int &pos,int &redo);
		~Rule();
};
# define __RULE__H
# endif
