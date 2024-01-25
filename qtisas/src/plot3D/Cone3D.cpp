/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: 3D cone class (code from Cone class in QwtPlot3D library with modified destructor)
 ******************************************************************************/

#include <math.h>
#include "qwt3d_color.h"
#include "qwt3d_plot.h"
#include <qwt3d_curve.h>
#include "Cone3D.h"

using namespace Qwt3D;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Cone3D (code from Cone class  in QwtPlot3D library with modified destructor)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Cone3D::Cone3D()
{
	hat      = gluNewQuadric();
	disk     = gluNewQuadric();

  configure(0, 3);
}

Cone3D::Cone3D(double rad, unsigned quality)
{
	hat      = gluNewQuadric();
	disk     = gluNewQuadric();

  configure(rad, quality);
}

Cone3D::~Cone3D()
{
}

void Cone3D::configure(double rad, unsigned quality)
{
  plot = 0;
  radius_ = rad;
  quality_ = quality;
  oldstate_ = GL_FALSE;

	gluQuadricDrawStyle(hat,GLU_FILL);
	gluQuadricNormals(hat,GLU_SMOOTH);
	gluQuadricOrientation(hat,GLU_OUTSIDE);
	gluQuadricDrawStyle(disk,GLU_FILL);
	gluQuadricNormals(disk,GLU_SMOOTH);
	gluQuadricOrientation(disk,GLU_OUTSIDE);
}

void Cone3D::draw(Qwt3D::Triple const& pos)
{  
	RGBA rgba = (*plot->dataColor())(pos);
  glColor4d(rgba.r,rgba.g,rgba.b,rgba.a);

  GLint mode;
	glGetIntegerv(GL_MATRIX_MODE, &mode);
	glMatrixMode( GL_MODELVIEW );
  glPushMatrix();

  glTranslatef(pos.x, pos.y, pos.z);

  gluCylinder(hat, 0.0, radius_, radius_*2, quality_, 1);
  glTranslatef(0, 0, radius_*2);
	gluDisk(disk, 0.0, radius_, quality_, 1);

  glPopMatrix();
	glMatrixMode(mode);
}
