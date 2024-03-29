# ifndef __PROGRAM__H
# include <symbol.h>
# include <rule.h>
# include <vector>
using namespace std;

/*	
 *	This is a generic class for the 
 *	producted programs of the mapping
 *	procedure. 
 * */
class Program
{
	protected:
		/*	
		 *	start_symbol: The start symbol of the BNF grammar.
		 * */
		Symbol *start_symbol;
	public:
		/*	
		 *	Program(): The constructor of the class.
		 *	setStartSymbol(s): Set the start symbol of the grammar.
		 *	getStartSymbol(): Return the start symbol of the grammar.
		 *	printRandomProgram(genome,redo): Print in string format
		 *			 the program produced by the chromosome
		 *			 genome. If redo>=REDO_MAX the production
		 *			 has failed.
		 *      fitness(genome): The virtual function for the fitness value
		 *                      for the chromosome genome. The user must
		 *                      override this function.
		 *	~Program(): The destructor of the class.
		 * */
		Program();
		void	setStartSymbol(Symbol *s);
		Symbol	*getStartSymbol() const;
		string	printRandomProgram(vector<int> &genome,int &redo);
		virtual double	fitness(vector<int> &genome);
		~Program();
};

# define __PROGRAM__H
# endif
