/***************************************************************************
 File                 : deadTimeRoot.cpp
 Project              : QtiSAS :: DAN
 --------------------------------------------------------------------
 Copyright /QtiSAS/  : (C) 2006-2021 by Vitaliy Pipich
 Email (use @ for *)  : v.pipich*gmail.com
 Description          : Dead time correction Non paralysable N=n exp(-nt) 
 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

#include "deadTimeRoot.h"
#include <math.h>

double
deadTimeRoot_f (double x, void *params)
{
  struct deadTimeRoot_params *p
    = (struct deadTimeRoot_params *) params;
  double I = p->I;
  double tau = p->tau;

  return x*exp(-tau*x) - I;
}

double
deadTimeRoot_deriv (double x, void *params)
{
  struct deadTimeRoot_params *p
    = (struct deadTimeRoot_params *) params;
  double I = p->I;
  double tau = p->tau;
 
  return (1-tau*x)*exp(-tau*x);
}

void
deadTimeRoot_fdf (double x, void *params,
               double *y, double *dy)
{
  struct deadTimeRoot_params *p
    = (struct deadTimeRoot_params *) params;
  double I = p->I;
  double tau = p->tau;

  *y = (1-tau*x)*exp(-tau*x);
  *dy = (1-tau*x)*exp(-tau*x);
}
