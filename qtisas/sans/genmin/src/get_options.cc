#if (_MSC_VER != 1600)
# include <unistd.h>
#endif
# include <string.h>
# include <stdio.h>
# include <stdlib.h>

int 	genome_count;
int	generations;
double	selection_rate;
double	mutation_rate;
int	random_seed;
int genome_size;
double    levenberg_abs;
double    levenberg_rel;
double    levenberg_step;
int       levenberg_iters;
bool      levenberg_delta;
bool      levenberg_unscaled;
bool      levenberg_constchi;
bool      levenberg_yn;
bool      genmin_yn;

void	parse_cmd_line(int argc,char **argv);
const char *short_options="c:g:s:m:r:h";
/*
void	print_usage()
{
	printf("\t-h	Print this help screen.\n"
		"\t-c	Specify genome count (default 100).\n"
		"\t-g	Specify maximum number of generations (default 500).\n"
		"\t-s	Specify selection rate (default 0.10).\n"
		"\t-m	Specify mutation rate (default 0.05).\n"
		"\t-r	Specify random seed (default 1).\n");
}

void	parse_cmd_line(int argc,char **argv)
{
	int next_option;
	//	DEFAULT VALUES

	genome_count=100;
	generations=500;
	selection_rate=0.10;
	mutation_rate=0.05;
	random_seed=1;
	do
	{
		next_option=getopt(argc,argv,short_options);
		switch(next_option)
		{
			case 'm':
				mutation_rate = atof(optarg);
				break;
			case 'c':
				genome_count=atoi(optarg);
				break;
			case 'g':
				generations=atoi(optarg);
				break;
			case 's':
				selection_rate=atof(optarg);	
				break;
			case 'r':
				random_seed=atoi(optarg);
				break;
			case -1:
				break;
			case '?':
				print_usage();
				exit(1);
				break;
			default:
				print_usage();
				exit(1);
				break;
		}
	}while(next_option!=-1);
}
*/
