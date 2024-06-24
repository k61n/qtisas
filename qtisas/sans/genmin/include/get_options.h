
#ifndef GETOPTIONS_H
#define GETOPTIONS_H

/*	
 *	The command line options of the proposed software.
 * */
extern	int 	genome_count;
extern    int     genome_size;
extern	int	generations;
extern  double	selection_rate;
extern 	double	mutation_rate;
extern	int	random_seed;
extern    double    levenberg_abs;
extern    double    levenberg_rel;
extern    double    levenberg_step;
extern    int       levenberg_iters;
extern    bool      levenberg_delta;
extern    bool      levenberg_unscaled;
extern    bool      levenberg_constchi;
extern    bool      levenberg_yn;
extern    bool      genmin_yn;
//extern char	train_file[1024];
//extern void	parse_cmd_line(int argc,char **argv);

# endif
