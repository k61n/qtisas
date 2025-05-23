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

#ifndef BAR_H
#define BAR_H

#include <qwtplot3d/qwt3d_plot.h>

//! 3D bars (modified enrichment from QwtPlot3D)
class Bar : public Qwt3D::VertexEnrichment
{
public:
  Bar();
    explicit Bar(double rad, bool lines = true, bool filled = true, bool smooth = true);

    [[nodiscard]] Qwt3D::Enrichment *clone() const override
    {
        return new Bar(*this);
    }

  void configure(double rad, bool lines = true, bool filled = true, bool smooth = true);
    void drawBegin() override;
    void drawEnd() override;
    void draw(Qwt3D::Triple const &) override;

private:
    double radius_{};
    double diag_{};
    bool d_smooth{};
    bool d_draw_lines{};
    bool d_filled_bars{};
};

#endif
