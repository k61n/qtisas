
#include <cstdlib>
#include <cstring>

#include "multi_population.h"

# define MAX_RULE	256

/*	This class creates a list of  genetic subpopulations 
 *	using integer encoding.
 *	CLASS VARIABLES
 *	======================================================================
 *	genome:		The vector of chromosomes.
 *	children: 	The vector of children produced in crossover operation.
 *	fitness_array: 	The vector where we store the fitness for 
 *		       	every chromosome.
 *	selection_rate: The selection rate in [0,1]. It determines the 
 *			chromosomes that will remain intact from generation
 *			to generation.
 *	mutation_rate:	The mutation rate in [0,1]. It controls the 
 *			random disturbance in each chromosome.
 *	genome_count:	The total number of chromosomes in every subpopulation.
 *	genome_size:	The length of each chromosome.
 *	generation:	A counter holding the number of genetic iteration.
 *	dimension:	The dimension of the objective function and  the 
 *			total number of subpopulations.
 *	program:	A pointer to program that produces expression by the 
 *			mapping  procedure of GE.
 *	g:		A help vector
 *
 * */

/*	The constructor of the class. The argument gcount determines the 
 *	number of chromosomes in each subpopulation and the argument gsize
 *	determines the length of the chromosomes. The argument p is a 
 *	pointer to the objective function. The constructor will create
 *	d subpopulations, where d is the dimension of the objective function.
 * */
MultiPopulation::MultiPopulation(int gcount,int gsize,int d,Program *p)
{
	dimension      = d;
	selection_rate = 0.1;
	mutation_rate  = 0.1;
	genome_count   = gcount;
	genome_size    = gsize;
	generation     = 0;
	program        = p;

	genome=new int*[dimension * genome_count];
	children=new int*[dimension * genome_count];
	for(int i=0;i<dimension * genome_count;i++)
	{
		genome[i]=new int[genome_size];
		children[i]=new int[genome_size];
		for(int j=0;j<genome_size;j++)
			genome[i][j]=rand()%MAX_RULE;
	}
	fitness_array=new double[dimension * genome_count];
	g.resize(genome_size*dimension);
}

/*	Reinitialize all genetic subpopulations.
 * */
void	MultiPopulation::reset()
{
	generation = 0;
	for(int i=0;i<dimension * genome_count;i++)
		for(int j=0;j<genome_size;j++)
				genome[i][j]=rand()%MAX_RULE;
	for(int i=0;i<genome_count;i++)
			fitness_array[i]=-1e+8;
}

/*	Evaluate and return the fitness value for
 *	chromosome g.
 * */
double 	MultiPopulation::fitness(vector<int> &g)
{
	return program->fitness(g);
}

/*	Sort the chromosomes in the genetic subpopulation d
 *	according to the fitness values. The chromosomes 
 *	with the lowest fitness value go the begining of
 *	the subpopulation.
 * */
void	MultiPopulation::select(int d)
{
	int s=d*genome_count;
	int *itemp=new int[genome_size];
	for(int i=0;i<genome_count;i++)
	{
		for(int j=0;j<genome_count-1;j++)
		{
		if(fitness_array[s+j+1]>fitness_array[s+j])
		{
			double dtemp;
			dtemp=fitness_array[s+j];
			fitness_array[s+j]=fitness_array[s+j+1];
			fitness_array[s+j+1]=dtemp;
			memcpy(itemp,genome[s+j],genome_size*sizeof(int));
			memcpy(genome[s+j],genome[s+j+1],genome_size*sizeof(int));
			memcpy(genome[s+j+1],itemp,genome_size*sizeof(int));
		}
		}
	}
	delete[] itemp;
}

/*	Perform crossover using tournament selection for the subpopulation d.
 * */
void	MultiPopulation::crossover(int d)
{
	int s=d*genome_count;
        int parent[2];
        int nchildren=(int)((1.0 - selection_rate) * genome_count);
	if(!(nchildren%2==0)) nchildren++;
        const int tournament_size =(genome_count<=100)?4:10;
        int count_children=0;
        while(1)
        {
                for(int i=0;i<2;i++)
                {
			
                        double max_fitness=-1e+10;
                        int    max_index=-1;
			int r;
                        for(int j=0;j<tournament_size;j++)
                        {
				r=rand() % (genome_count);
                                if(j==0 || fitness_array[s+r]>max_fitness)
                                {
                                        max_index=s+r;
                                        max_fitness=fitness_array[s+r];
                                }
                        }
                        parent[i]=max_index;
                }
		int pt1;

		pt1=rand() % genome_size;
		memcpy(children[count_children],
			genome[parent[0]],pt1 * sizeof(int));
		memcpy(&children[count_children][pt1],
			&genome[parent[1]][pt1],(genome_size-pt1)*sizeof(int));
		memcpy(children[count_children+1],
			genome[parent[1]],pt1 * sizeof(int));
		memcpy(&children[count_children+1][pt1],
			&genome[parent[0]][pt1],(genome_size-pt1)*sizeof(int));
		count_children+=2;
		if(count_children>=nchildren) break;
	}
	

	for(int i=0;i<nchildren;i++)
	{
		memcpy(genome[s+genome_count-i-1],children[i],genome_size * sizeof(int));
	}
}

/*	Perform the mutation procedure for the subpopulation d.
 *	For every element of each chromosome in subpopulation d
 *	a random number r in [0,1] is estimated. If the random 
 *	number is less than mutation rate, then the element is
 *	changed randomly else it remains intact.
 * */
void	MultiPopulation::mutate(int d)
{
	int s=d*genome_count;
	int start = 1;
	for(int i=start;i<genome_count;i++)
	{
		for(int j=0;j<genome_size;j++)
		{
			double r=(double(rand()) / RAND_MAX);//+++ drand48();
			if(r<mutation_rate) genome[s+i][j]=rand() % MAX_RULE;
		}
	}
}

/*	Calculates the fitness for every chromosomes in all genetic
 *	subpopulations.
 * */
void	MultiPopulation::calcFitnessArray()
{
	for(int i=0;i<genome_count;i++)
	{
		for(int d=0;d<dimension;d++)
		for(int j=0;j<genome_size;j++) 
			g[d*genome_size+j]=genome[d*genome_count+i][j];	
		double f=fitness(g);
		for(int d=0;d<dimension;d++) 
			fitness_array[d*genome_count+i]=f;
	}
}

/*	Return the number of generation counter.
 * */
int	MultiPopulation::getGeneration() const
{
	return generation;
}

/*	Return the total number of the chromosomes.
 * */
int	MultiPopulation::getCount() const
{
	return genome_count;
}

/*	Return the size of the chromosomes.
 * */
int	MultiPopulation::getSize() const
{
	return genome_size;
}

/*	The main step of the genetic algorithm.
 *	The steps are
 *	1. Mutation.
 *	2. Calculation of fitness arrays.
 *	3. Crossover.
 *	4. Increase the  generation counter.
 * */
void	MultiPopulation::nextGeneration()
{
	if(generation)
	{
		for(int d=0;d<dimension;d++) mutate(d);
	}
	calcFitnessArray();
	for(int d=0;d<dimension;d++)
	{
		select(d);
		crossover(d);
	}
	++generation;
}

/*	Set the mutation rate to the parameter r.
 * */
void	MultiPopulation::setMutationRate(double r)
{
	if(r>=0 && r<=1) mutation_rate = r;
}

/*	Set the selection rate to the parameter r.
 * */
void	MultiPopulation::setSelectionRate(double r)
{
	if(r>=0 && r<=1) selection_rate = r;
}

/*	Return the selection rate of the population.
 * */
double	MultiPopulation::getSelectionRate() const
{
	return selection_rate;
}

/*	Return the mutation rate of the population.
 * */
double	MultiPopulation::getMutationRate() const
{
	return mutation_rate;
}

/*	Return without additional evaluation the fitness of the 
 *	best chromosome in the population.
 * */
double	MultiPopulation::getBestFitness() const
{
	return  fitness_array[0];
}

/*	Return the best chromosome in the population.
 * */
vector<int> MultiPopulation::getBestGenome()
{
	for(int d=0;d<dimension;d++)
	for(int i=0;i<genome_size;i++) g[d*genome_size+i]=genome[0+d*genome_count][i];
	return g;
}

/*	Evaluate and return the fitness of the best chromosome in the 
 *	population.
 * */
double	MultiPopulation::evaluateBestFitness() 
{
	for(int i=0;i<genome_size;i++) g[i]=genome[0][i];	
	return fitness(g);
}

/*	The destructor of the class.
 * */
MultiPopulation::~MultiPopulation()
{
    for(int i=0;i< dimension*genome_count ;i++)
	{
		delete[] children[i];
		delete[] genome[i];
	}

	delete[] genome;
	delete[] children;
	delete[] fitness_array;
    
//    g.clear(); //+++2015 test
}
