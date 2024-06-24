
#include "program.h"

/*	This is a generic class, which represents the programs created
 *	by  the BNF grammar of the problem. 
 *	CLASS VARIABLES
 *	===============================================================
 *	start_symbol: a pointer to the starting symbol of the grammar.
 * */


/*	The constructor of the class. 
 * */
Program::Program()
{
	start_symbol = nullptr;
}

/*	Set the start symbol of the grammar to the pointer s.
 * */
void	Program::setStartSymbol(Symbol *s)
{
	start_symbol = s;
}

/*	Return the start symbol of the grammar.
 * */
Symbol	*Program::getStartSymbol() const
{
	return start_symbol;
}


/*	Return a string composed by the array genome. The creation starts
 *	from the start symbol of the grammar and it is gradually creates
 *	the program by replacing non terminal symbols with the right
 *	hand of the selected production rule. The integer variable redo
 *	will hold the number of wrapping events took place.
 * */
string	Program::printRandomProgram(vector<int> &genome,int &redo)
{
	string str="";
	int count=0;
	Rule *r;
	int pos=genome[count]%start_symbol->getCountRules();
	r=start_symbol->getRule(0);
	redo = 0;
	str=r->printRule(genome,count,redo);
	return str;
}

/*	A generic method, that returns a fitness value for a given genome.
 *	The user must override this method according to his needs.
 * */
double	Program::fitness(vector<int> &genome)
{
	return 0.0;
}

/*	The destructor of the class.
 * */
Program::~Program()
{

}
