/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: 3D bars (modified enrichment from QwtPlot3D)
 ******************************************************************************/

#ifndef BARS_H
#define BARS_H

#include <qwt3d_plot.h>

//! 3D bars (modifed enrichment from QwtPlot3D)
class Bar : public Qwt3D::VertexEnrichment
{
public:
  Bar();
  Bar(double rad, bool lines = true, bool filled = true, bool smooth = true);

  Qwt3D::Enrichment* clone() const {return new Bar(*this);}

  void configure(double rad, bool lines = true, bool filled = true, bool smooth = true);
  void drawBegin();
  void drawEnd();
  void draw(Qwt3D::Triple const&);

private:
  double radius_;
  double diag_;
  bool d_smooth;
  bool d_draw_lines;
  bool d_filled_bars;
};


#endif
