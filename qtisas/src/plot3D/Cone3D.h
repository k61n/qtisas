/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: 3D cone class (code from Cone class in QwtPlot3D library with modified destructor)
 ******************************************************************************/

#ifndef MYCONES_H
#define MYCONES_H

#include <qwtplot3d/qwt3d_plot.h>

//! 3D cone class (based on QwtPlot3D)
class Cone3D : public Qwt3D::VertexEnrichment
{
public:
  Cone3D();
  Cone3D(double rad, unsigned quality);
  ~Cone3D();

  Qwt3D::Enrichment* clone() const {return new Cone3D(*this);}
  
  void configure(double rad, unsigned quality);
  void draw(Qwt3D::Triple const&);

private:
  	GLUquadricObj *hat;
	GLUquadricObj *disk;
	unsigned quality_;
	double radius_;
	GLboolean oldstate_;
};

#endif
