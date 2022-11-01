# include <rlsprogram.h>
# include <math.h>
# include <stdio.h>
# include <tolmin.h>
# include <cstdlib>

/*	This is a class that inherites the Program class.
 *	This class implements a BNF grammar that creates
 *	digit constants in range [0,1).
 *	CLASS VARIABLES
 *	=====================================================================
 *	problem:	A pointer to the objective function.
 *	dimension:	The dimension of the objective function.
 *	start_symbol:	The starting symbol of the BNF grammar.
 *	dx:		A help vector.
 *	lmargin:	The left bounds of the objective function.
 *	rmargin:	The right bounds of the objective function.
 *	Digit:		A vector of symbols. Each symbol is a decimal digit.
 *	Digit0:		A non terminal symbol.
 *	Dot:		The symbol of . 
 *	DigitList:	A non terminal symbol.
 *
 * */

/*	The constructor of the class. Set the problem to the parameter P
 *	and construct the BNF grammar.
 * */
RlsProgram::RlsProgram(Problem *P)
{
	problem = P;
	dimension = problem->getDimension();
	dx.resize(dimension);
	problem->getLeftMargin(lmargin);
	problem->getRightMargin(rmargin);
	makeTerminals();
	makeNonTerminals();
	makeRules();
	int r=newRule();
	start_symbol=new Symbol();
	start_symbol->setName("START");
	start_symbol->setTerminalStatus(0);
	
	rule[r]->addSymbol(&Digit[0]);
	rule[r]->addSymbol(&Dot);
	rule[r]->addSymbol(&DigitList);
	
	start_symbol->addRule(rule[r]);
}

/*	Create the terminal symbols of the grammar.
 * */
void	RlsProgram::makeTerminals()
{
	Digit.resize(10);
	for(int i=0;i<10;i++)
	{
		char str[10];
		sprintf(str,"%d",i);
		Digit[i].setName(str);
		Digit[i].setTerminalStatus(1);
	}
	Dot.setName(".");
	Dot.setTerminalStatus(1);
}

/*	Create the non terminal symbols of the grammar.
 * */
void	RlsProgram::makeNonTerminals()
{
	DigitList.setName("DIGITLIST");
	DigitList.setTerminalStatus(0);
	Digit0.setName("DIGIT0");
	Digit0.setTerminalStatus(0);
}

/*	Create the rules of the BNF grammar.
 * */
void	RlsProgram::makeRules()
{
	int r;

	r=newRule();
	rule[r]->addSymbol(&Digit0);
	DigitList.addRule(rule[r]);

	r=newRule();
	rule[r]->addSymbol(&Digit0);
	rule[r]->addSymbol(&DigitList);
	DigitList.addRule(rule[r]);

	for(int i=0;i<Digit.size();i++)
	{
		r=newRule();
		rule[r]->addSymbol(&Digit[i]);
		Digit0.addRule(rule[r]);
	}
}

/*	Create and return a new empty rule.
 * */
int	RlsProgram::newRule()
{
	int s=rule.size();
	rule.resize(s+1);
	rule[s]=new Rule();
	return s;
}

/*	Return the fitness value for the array genome.
 *	This function will create a point using the
 *	proposed BNF grammar in [0,1] and it will be 
 *	scaled to the bounds of the objective function.
 *	The function returns the value of objective 
 *	function evaluated at the created point.
 * */
double	RlsProgram::fitness(vector<int> &genome)
{
	vector<int> subgenome;
	subgenome.resize(genome.size()/dimension);
	for(int i=0;i<dimension;i++)
	{
		for(int j=0;j<subgenome.size();j++)
			subgenome[j]=genome[i*subgenome.size()+j];
		int redo=0;
		string str=printRandomProgram(subgenome,redo);
		if(redo>=REDO_MAX) return -1e+100;
		dx[i]=atof(str.c_str());
		dx[i]=lmargin[i]+(rmargin[i]-lmargin[i])*(dx[i]);
	}
    
    subgenome.clear();//2014 new
	return -problem->funmin(dx);
}

/*	Return the point using the same procedure 
 *	of the function previous fitness().
 * */
void	RlsProgram::getX(vector<int> &genome,DataG &x)
{
	vector<int> subgenome;
	subgenome.resize(genome.size()/dimension);
	for(int i=0;i<dimension;i++)
	{
		for(int j=0;j<subgenome.size();j++)
			subgenome[j]=genome[i*subgenome.size()+j];
		int redo=0;
		string str=printRandomProgram(subgenome,redo);
		if(redo>=REDO_MAX) return ;
		dx[i]=atof(str.c_str());
		dx[i]=lmargin[i]+(rmargin[i]-lmargin[i])*fabs(dx[i]);
	}
    //subgenome.clear();//2014 new
	x=dx;
}

/*	The destructor of the class.
 *	Delete the rule list and the start symbol of the grammar.
 * */
RlsProgram::~RlsProgram()
{

	for(int i=0;i<rule.size();i++) delete rule[i];
	delete start_symbol;
}

