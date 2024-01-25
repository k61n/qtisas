/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2011 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A wrapper around QwtLinearColorMap from Qwt
 ******************************************************************************/

#include "LinearColorMap.h"

LinearColorMap::LinearColorMap() : QwtLinearColorMap(),
d_range(QwtDoubleInterval())
{}

LinearColorMap::LinearColorMap(const QColor &from, const QColor &to) : QwtLinearColorMap(from, to),
d_range(QwtDoubleInterval())
{}

QwtDoubleInterval LinearColorMap::intensityRange() const
{
	return d_range;
}

QString LinearColorMap::toXmlString()
{
	QString s = "<ColorMap>\n";
	s += "\t<Mode>" + QString::number(mode()) + "</Mode>\n";
	s += "\t<MinColor>" + color1().name() + "</MinColor>\n";
	s += "\t<MaxColor>" + color2().name() + "</MaxColor>\n";
	if (d_range.isValid())
		s += "\t<Range>" + QString::number(d_range.minValue(), 'g', 15) + "\t" + QString::number(d_range.maxValue(), 'g', 15) + "</Range>\n";
	QwtArray <double> colors = colorStops();
	int stops = (int)colors.size();
	s += "\t<ColorStops>" + QString::number(stops - 2) + "</ColorStops>\n";
	for (int i = 1; i < stops - 1; i++){
		s += "\t<Stop>" + QString::number(colors[i], 'g', 15) + "\t";
		s += color(i).name();
		s += "</Stop>\n";
	}
	return s += "</ColorMap>\n";
}

LinearColorMap LinearColorMap::fromXmlStringList(const QStringList& lst)
{
	QStringList::const_iterator line = lst.begin();
	QString s = (*line).trimmed();

	int mode = s.remove("<Mode>").remove("</Mode>").trimmed().toInt();
	s = *(++line);
	QColor color1 = QColor(s.remove("<MinColor>").remove("</MinColor>").trimmed());
	s = *(++line);
	QColor color2 = QColor(s.remove("<MaxColor>").remove("</MaxColor>").trimmed());

	LinearColorMap colorMap = LinearColorMap(color1, color2);
	colorMap.setMode((QwtLinearColorMap::Mode)mode);

	s = *(++line);
	if (s.contains("<Range>")){
		QStringList l = s.remove("<Range>").remove("</Range>").split("\t", QString::SkipEmptyParts);
		if (l.size() == 2)
			colorMap.setIntensityRange(QwtDoubleInterval(l[0].toDouble(), l[1].toDouble()));
		 s = *(++line);
	}

	int stops = s.remove("<ColorStops>").remove("</ColorStops>").trimmed().toInt();
	for (int i = 0; i < stops; i++){
		s = (*(++line)).trimmed();
		QStringList l = s.remove("<Stop>").remove("</Stop>").split("\t", QString::SkipEmptyParts);
		colorMap.addColorStop(l[0].toDouble(), QColor(l[1]));
	}

	return colorMap;
}
