/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: 3D bars (modified enrichment from QwtPlot3D)
 ******************************************************************************/

#include <qwtplot3d/qwt3d_curve.h>
#include <qwtplot3d/qwt3d_color.h>
#include "Bar.h"

using namespace Qwt3D;

/////////////////////////////////////////////////////////////////
//
//  3D Bars  (modified enrichement example from QwtPlot3D archive)
//
/////////////////////////////////////////////////////////////////

Bar::Bar()
{
  configure(0);
}

Bar::Bar(double rad, bool lines, bool filled, bool smooth)
{
  configure(rad, lines, filled, smooth);
}

void Bar::configure(double rad, bool lines, bool filled, bool smooth)
{
  plot = 0;
  radius_ = rad;
  d_smooth = smooth;
  d_draw_lines = lines;
  d_filled_bars = filled;
}

void Bar::drawBegin()
{
  diag_ = (plot->hull().maxVertex-plot->hull().minVertex).length() * radius_;
  glLineWidth( plot->meshLineWidth() );
  glEnable(GL_POLYGON_OFFSET_FILL);
  if (d_smooth)
    glEnable(GL_LINE_SMOOTH);
  else
    glDisable(GL_LINE_SMOOTH);

  glPolygonOffset(1, 1);
}

void Bar::drawEnd()
{
}

void Bar::draw(Qwt3D::Triple const& pos)
{
    GLdouble minz = plot->hull().minVertex.z;

	double xl = pos.x - diag_;
	double xr = pos.x + diag_;
	double yl = pos.y - diag_;
	double yr = pos.y + diag_;

    if (d_filled_bars){
        RGBA rgbat = (*plot->dataColor())(pos);
		glColor4d(rgbat.r,rgbat.g,rgbat.b,rgbat.a);
						
        glBegin(GL_QUADS);
		glVertex3d(xl,yl,minz);
		glVertex3d(xr,yl,minz);
		glVertex3d(xr,yr,minz);
		glVertex3d(xl,yr,minz);

		glVertex3d(xl,yl,pos.z);
		glVertex3d(xr,yl,pos.z);
		glVertex3d(xr,yr,pos.z);
		glVertex3d(xl,yr,pos.z);

		glVertex3d(xl,yl,minz);
		glVertex3d(xr,yl,minz);
		glVertex3d(xr,yl,pos.z);
		glVertex3d(xl,yl,pos.z);

		glVertex3d(xl,yr,minz);
		glVertex3d(xr,yr,minz);
		glVertex3d(xr,yr,pos.z);
		glVertex3d(xl,yr,pos.z);

		glVertex3d(xl,yl,minz);
		glVertex3d(xl,yr,minz);
		glVertex3d(xl,yr,pos.z);
		glVertex3d(xl,yl,pos.z);

		glVertex3d(xr,yl,minz);
		glVertex3d(xr,yr,minz);
		glVertex3d(xr,yr,pos.z);
		glVertex3d(xr,yl,pos.z);
        glEnd();
    }

    if (!d_draw_lines)
        return;

	Qwt3D::RGBA meshCol = plot->meshColor();//using mesh color to draw the lines
    glColor3d(meshCol.r, meshCol.g, meshCol.b);

  glBegin(GL_LINES);
	glVertex3d(xl,yl,minz); glVertex3d(xr,yl,minz);
	glVertex3d(xl,yl,pos.z); glVertex3d(xr,yl,pos.z);
	glVertex3d(xl,yr,pos.z); glVertex3d(xr,yr,pos.z);
	glVertex3d(xl,yr,minz); glVertex3d(xr,yr,minz);

	glVertex3d(xl,yl,minz); glVertex3d(xl,yr,minz);
	glVertex3d(xr,yl,minz); glVertex3d(xr,yr,minz);
	glVertex3d(xr,yl,pos.z); glVertex3d(xr,yr,pos.z);
	glVertex3d(xl,yl,pos.z); glVertex3d(xl,yr,pos.z);

	glVertex3d(xl,yl,minz); glVertex3d(xl,yl,pos.z);
	glVertex3d(xr,yl,minz); glVertex3d(xr,yl,pos.z);
	glVertex3d(xr,yr,minz); glVertex3d(xr,yr,pos.z);
	glVertex3d(xl,yr,minz); glVertex3d(xl,yr,pos.z);
  glEnd();
}
