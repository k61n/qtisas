
#ifndef RLSPROGRAM_H
#define RLSPROGRAM_H

# include <problem.h>
# include <program.h>

/*	
 *	This class is used for the production
 *	of points by the BNF grammar.
 * */
class RlsProgram :public Program
{
	private:
		/*	
		 *	problem: The objective function to be minimized.
		 *	dimension: The dimension of the objective function.
		 *	lmargin:  The left bounds of the objective function.
		 *	rmargin:  The right bounds of the objective function.
		 *	dx:	  A help vector.
		 *	Dot,Digit,Digit,DigitList: The symbols
		 *						of the BNF grammar.
		 *	rule: The vector of production rules.
		 *	maketerminals(): This function produces the terminal symbols
		 *			 of the grammar.
		 *      makeNonTerminals(): This function produces the non terminal
		 *      		 symbols of the grammar.
		 *      makeRules(): This function produces the production rules.
		 *      newRules(): This function is called each time that a new rule
		 *      	    is produced.
		 * */
		Problem *problem;
		DataG lmargin,rmargin,dx;
		int dimension;
		Symbol Dot, Digit0, DigitList;
		vector<Symbol> Digit;
		vector<Rule*> rule;
		void	makeTerminals();
		void	makeNonTerminals();
		void	makeRules();
		int	newRule();
	public:
		/*	
		 *	RlsProgram(P): The constructor of the class.
		 *	fitness(genome): Return the fitness value for the chromosome
		 *			 indicated by the array genome.
		 *	getX(genome,x):  Return in argument x the correspondig feasible
		 *			 point x produced by the chromosome genome
		 *			 using the mapping procedure of GE.
		 *	printProgram(genome): Return in string format the feasible point
		 *			produced by GE.
		 *	~RlsProgram(): The destructor of the class.
		 * */
		RlsProgram(Problem *P);
		virtual double fitness(vector<int> &genome);
		void    getX(vector<int> &genome,DataG &x);
		~RlsProgram();
};

#endif
