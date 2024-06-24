
#include <iostream>

#include "rule.h"

/*	This class represents the rule list used in the BNF grammar. 
 *	The members of the rule list are pointers of the class Symbol.
 *	CLASS MEMBERS
 *	=======================================================================
 *	length: The amount of symbols in the symbol list
 *	data:   A table holding pointers to the elements of symbol list.
 * */

/*	The constructor of the Rule class. Simple set the counter length to zero.
 * */
Rule::Rule()
{
	length =0;
}

/*	This function adds a Symbol to the end of the symbol stack of the Rule.
 * */
void	Rule::addSymbol(Symbol *s)
{
	data.push_back(s);
	length++;
}

/*	Return the position of the symbol labeled with the string s. 
 *	If the symbol is not in the symbol stack, the function returns -1.
 * */
int	Rule::getSymbolPos(string s)
{
	for(int i=0;i<length;i++)
		if(data[i]->getName()==s) return i;
	return -1;
}

/*	Return a pointer pointing to the symbol in position denoted by pos.
 * */
Symbol	*Rule::getSymbol(int pos) const
{
	if(pos<0 || pos>=length) return nullptr;
	return data[pos];
}

/*	Replace the symbol in position denoted by pos with the Symbol s.
 * */
void	Rule::setSymbol(int pos,Symbol *s)
{
	if(pos<0 || pos>=length) return; 
	data[pos]=s;
}

/*	Return the number of symbols in the symbol list.
 * */
int	Rule::getLength() const
{
	return length;
}

/*	Produce a string composed by the names in the atom list.
 *	The production is guided by the array genome, which holds
 *	the production rules for every non terminal symbol in the atom
 *	list according to Grammatical Evolution procedure. The integer 
 *	variable pos will hold the position in genome, where the production
 *	will stop. The integer variable redo will hold the number of wrapping
 *	events. If this number is greater than a predefined number (REDO_MAX)
 *	the function stops.
 * */
string	Rule::printRule(vector<int> genome,int &pos,int &redo)
{

	string str="";
	string str2="";
	Rule *r;
	for(int i=0;i<length;i++)
	{
		Symbol *s=data[i];
		if(s->getTerminalStatus())
		{
			str=str+s->getName();	
		}
		else
		{
			if(pos>=genome.size()) {redo++;pos=0;}
			r=s->getRule((genome[pos]%s->getCountRules()));
			pos++;
			if(pos>=genome.size()) {redo++;pos=0;}
			if(redo>=REDO_MAX) return str;
			str2=r->printRule(genome,pos,redo);
			str=str+str2;
		}
	}
	return str;
}

/*	The destructor of the class. 
 * */
Rule::~Rule()
{
}

