
#ifndef TOLMIN_H
#define TOLMIN_H

# include <problem.h>

#ifdef __cplusplus
extern "C"{
# endif

/*	
 *	This is the prototype for the local search
 *	procedure used by the method. The local
 *	search procedure is a BFGS procedure 
 *	modified by Powell.
 * */
extern double	tolmin(DataG &x,MinInfo &Info);

#ifdef __cplusplus
}
#endif

#endif
