
#ifndef MULTI_POPULATION_H
#define MULTI_POPULATION_H

#include <program.h>

/*
 *	This class represents the genetic population.
 * */
class MultiPopulation
{
	private:
		/*	
		 *	g: A help vector
		 *	genome: The chromosomes in the population.
		 *	children: The produced children chromosomes in each generation.
		 *	fitness_array: An array holding the fitness values of the population.
		 *	selection_rate: The selection rate (replication rate) of the population.
		 *	mutation_rate: The mutation rate of the population
		 *	genome_count: The amount of chromosomes in the population.
		 *	genome_size: The length of each chromosome.
		 *	generation: A counter used to determine the number of generations passed.
		 *	dimension: The dimension of the objective function.
		 *	program: A pointer to program that produces expression by the mapping 
		 *	        procedure of GE.
		 *	fitness(g): The function which produces the fitness for the chromosome g.
		 *	select(d): Sort the population according to the fitness values.
		 *	crossover(d): Implement the crossover procedure with tournament selection.
		 *	mutate(d): Perform the mutation procedure with rate mutation rate.
		 *	calcFitnessArray(): Calculate the fitness value for each chromosome.
		 * */
		vector<int> g;
		int	**children;
		int	**genome;
		double *fitness_array;
		double	mutation_rate,selection_rate;
		int	genome_count;
		int	genome_size;
		int	generation;
		int	dimension;

		Program* program;

		double 	fitness(vector<int> &g);
		void	select(int d);
		void	crossover(int d);
		void	mutate(int d);
		void	calcFitnessArray();
	public:
		/*	
		 *	MultiPopulation(gcount,gsize,d,p): The constructor of the class.
		 *	getGeneration(): Return the number of generations passed.
		 *	getCount(): Return the amount of chromosomes in the population.
		 *	getSize(): Return the size of each chromosome.
		 *	nextGeneration(): Perform the genetic operations.
		 *	setMutationRate(r): Set the mutation rate to r.
		 *	setSelectionRate(r): Set the selection rate to r.
		 *	getMutationRate(): Return the mutation rate.
		 *	getSelectionRate(): Return the selection rate.
		 *	getBestFitness(): Return the best fitness in the population.
		 *	evaluateBestFitness(): Evaluate the fitness of the best chromosome.
		 *	getBestGenome(): Return the best chromosome in the population.
		 *	~MultiPopulation(): The destructor of the class.
		 * */
		MultiPopulation(int gcount,int gsize,int d,Program *p);
		int	getGeneration() const;
		int	getCount() const;
		int	getSize() const;
		void	nextGeneration();
		void	setMutationRate(double r);
		void	setSelectionRate(double r);
		double	getSelectionRate() const;
		double	getMutationRate() const;
		double	getBestFitness() const;
		double	evaluateBestFitness();
		vector<int> getBestGenome();
		void	reset();
		~MultiPopulation();
		
};

#endif
