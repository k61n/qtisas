/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: FFT for matrices
 ******************************************************************************/

#include <cmath>

#include "fft2D.h"
#include "Matrix.h"

int next2Power(int n)
{
	int l2n = 0, p = 1; //l2n will become log_2(n)
	while(p < n){
		p *= 2;
		l2n++;
	}
	n = 1 << l2n;
	return n;
}

bool isPower2(int n)
{
	return n && !(n & (n - 1));
}

void fft(double* x_int_re, double* x_int_im, int taille)
{
	int size_2 = taille >> 1, tmp1 = 0;
	double tmp, tmpcos, tmpsin, base = 2*M_PI/taille;
	const double SQ_2=sqrt(2);
    auto pair_re = new double[size_2];
    auto pair_im = new double[size_2];
    auto impair_re = new double[size_2];
    auto impair_im = new double[size_2];

	for(int i=0; i<size_2; i++){
		tmp1=(i<<1);
		pair_re[i]=x_int_re[tmp1];
		pair_im[i]=x_int_im[tmp1];
		impair_re[i]=x_int_re[tmp1+1];
		impair_im[i]=x_int_im[tmp1+1];
	}

	if(taille>2){
		fft(pair_re,pair_im,size_2);
		fft(impair_re,impair_im,size_2);
	}

	for(int i=0; i<size_2; i++){
		tmp=base*i;
		tmpcos=cos(tmp);
		tmpsin=sin(tmp);
		x_int_re[i]=(pair_re[i]+impair_re[i]*tmpcos+impair_im[i]*tmpsin)/SQ_2;
		x_int_im[i]=(pair_im[i]+impair_im[i]*tmpcos-impair_re[i]*tmpsin)/SQ_2;
		x_int_re[i+size_2]=(pair_re[i]-impair_re[i]*tmpcos-impair_im[i]*tmpsin)/SQ_2;
		x_int_im[i+size_2]=(pair_im[i]-impair_im[i]*tmpcos+impair_re[i]*tmpsin)/SQ_2;
	}
    delete[] pair_re;
    delete[] pair_im;
    delete[] impair_re;
    delete[] impair_im;
}

void fft_inv(double* x_int_re, double* x_int_im, int taille)
{
	int size_2 = taille >> 1, tmp1 = 0;
	double tmp, tmpcos, tmpsin, base=2*M_PI/taille;
	const double SQ_2=sqrt(2);
    auto pair_re = new double[size_2];
    auto pair_im = new double[size_2];
    auto impair_re = new double[size_2];
    auto impair_im = new double[size_2];

	for(int i=0; i<size_2; i++){
		tmp1=i<<1;
		pair_re[i]=x_int_re[tmp1];
		pair_im[i]=x_int_im[tmp1];
		impair_re[i]=x_int_re[tmp1+1];
		impair_im[i]=x_int_im[tmp1+1];
	}

	if(taille>2){
		fft_inv(pair_re, pair_im,size_2);
		fft_inv(impair_re, impair_im,size_2);
	}

	for(int i=0; i<size_2; i++){
		tmp=base*i;
		tmpcos=cos(tmp);
		tmpsin=sin(tmp);
		x_int_re[i]=(pair_re[i]+impair_re[i]*tmpcos-impair_im[i]*tmpsin)/SQ_2;
		x_int_im[i]=(pair_im[i]+impair_im[i]*tmpcos+impair_re[i]*tmpsin)/SQ_2;
		x_int_re[i+size_2]=(pair_re[i]-impair_re[i]*tmpcos+impair_im[i]*tmpsin)/SQ_2;
		x_int_im[i+size_2]=(pair_im[i]-impair_im[i]*tmpcos-impair_re[i]*tmpsin)/SQ_2;
	}
    delete[] pair_re;
    delete[] pair_im;
    delete[] impair_re;
    delete[] impair_im;
}

void fft2d(double **xtre, double **xtim, int width, int height, bool shift)
{
	double **xint_re = Matrix::allocateMatrixData(height, width);
	if (!xint_re)
		return;
	double **xint_im = Matrix::allocateMatrixData(height, width);
	if (!xint_im){
		Matrix::freeMatrixData(xint_re, height);
		return;
	}

    auto x_int_l = new double[width];
    auto x_int2_l = new double[width];
    auto x_int_c = new double[height];
    auto x_int2_c = new double[height];
	for(int k=0; k<height; k++){
		for(int j=0; j<width; j++){
			x_int_l[j] = xtre[k][j];
			x_int2_l[j] = xtim[k][j];
		}

		fft(x_int_l, x_int2_l, width);

		for(int j=0; j<width; j++){
			xint_re[k][j] = x_int_l[j];
			xint_im[k][j] = x_int2_l[j];
		}
	}

	for(int k=0; k<width; k++){
		for(int i=0; i<height; i++){
			x_int_c[i] = xint_re[i][k];
			x_int2_c[i] = xint_im[i][k];
		}

		fft(x_int_c, x_int2_c, height) ;

		if (shift){
			int col = (k+(width>>1))%width;
			for(int i=0; i<height; i++){
				int row = (i+(height>>1))%height;
				xtre[row][col] = x_int_c[i];
				xtim[row][col] = x_int2_c[i];
			}
		} else {
			for(int i=0; i<height; i++){
				xtre[i][k] = x_int_c[i];
				xtim[i][k] = x_int2_c[i];
			}
		}
	}
	Matrix::freeMatrixData(xint_re, height);
	Matrix::freeMatrixData(xint_im, height);
    delete[] x_int_l;
    delete[] x_int2_l;
    delete[] x_int_c;
    delete[] x_int2_c;
}

void fft2d_inv(double **xtre, double **xtim, double **xrec_re, double **xrec_im, int width, int height, bool undoShift)
{
	double **xint_re = Matrix::allocateMatrixData(height, width);
	if (!xint_re)
		return;
	double **xint_im = Matrix::allocateMatrixData(height, width);
	if (!xint_im){
		Matrix::freeMatrixData(xint_re, height);
		return;
	}

    auto x_int_l = new double[width];
    auto x_int2_l = new double[width];
    auto x_int_c = new double[height];
    auto x_int2_c = new double[height];

	for(int k = 0; k < height; k++){
		if (undoShift){
			int row = (k + (height>>1))%height;
			for(int j = 0; j < width; j++){
				int col = (j + (width>>1))%width;
				x_int_l[j] = xtre[row][col];
				x_int2_l[j] = xtim[row][col];
			}
		} else {
			for(int j = 0; j < width; j++){
				x_int_l[j] = xtre[k][j];
				x_int2_l[j] = xtim[k][j];
			}
		}

		fft_inv(x_int_l, x_int2_l, width);

		for(int j = 0; j < width; j++){
			xint_re[k][j] = x_int_l[j];
			xint_im[k][j] = x_int2_l[j];
		}
	}
	for(int k = 0; k < width; k++){
		for(int i = 0; i < height; i++){
			x_int_c[i] = xint_re[i][k];
			x_int2_c[i] = xint_im[i][k];
		}

		fft_inv(x_int_c, x_int2_c, height);

		for(int i = 0; i < height; i++){
			xrec_re[i][k] = x_int_c[i];
			xrec_im[i][k] = x_int2_c[i];
		}
	}
	Matrix::freeMatrixData(xint_re, height);
	Matrix::freeMatrixData(xint_im, height);
    delete[] x_int_l;
    delete[] x_int2_l;
    delete[] x_int_c;
    delete[] x_int2_c;
}
