/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
Description: Interface for tools operating on a Graph
 ******************************************************************************/

#ifndef PLOT_TOOL_INTERFACE_H
#define PLOT_TOOL_INTERFACE_H

class Graph;
#include <QObject>

/*! Interface for tools operating on a Graph.
 *
 * The basic idea is to have one PlotToolInterface subclass per user-visible tool operating on a Graph,
 * although there may be cases where exceptions to this rule are appropriate.
 * %Note that tools can be presented to the user via icons (like DataPickerTool) or via menu entries
 * (like TranslateCurveTool); the basic concept is quite similar.
 * The main point in managing plot tools as subclasses of this class (as opposed to using void pointers)
 * is the virtual destructor that allows tools to clean up after themselves.
 * Additionally, a pointer to the parent Graph (#d_graph) is managed.
 * In the future, this class may provide other generic tool functionality.
 *
 * %Note that zooming and range selection are somewhat special in that they can be active in addition
 * to other tools. These are handled as special cases, while all other tools are rendered mutually exclusive
 * by having Graph manage a pointer to the currently active tool (Graph::d_active_tool).
 *
 * It would be nice for some of the plot tools (like TranslateCurveTool or MultiPeakFitTool) to send a signal
 * when they are finished and to generalize the statusText signal provided by most tools, but having
 * PlotToolInterface inherit from QObject would make it impossible for
 * plot tools to also inherit from other QObject decendants (such as QwtPlotPicker).
 * As a workaround, plot tools can call Graph::setActiveTool(), carefully noting that they are deleted
 * during this call.
 *
 * Currently, plot tools are instantiated by ApplicationWindow and handed to the Graph in question;
 * this scheme will have to be revised for dynamically adding new tools via plugins.
 */
class PlotToolInterface
{
	public:

    enum RttiValues
    {
        Rtti_PlotTool = 0,

        Rtti_RangeSelector,
        Rtti_DataPicker,
        Rtti_TranslateCurveTool,
        Rtti_MultiPeakFitTool,
        Rtti_LineProfileTool,
		Rtti_AddWidgetTool,
		Rtti_DrawDataPoints,
		Rtti_ImageProfilesTool,
		Rtti_SubtractLineTool,
        Rtti_PlotUserTool = 1000
    };

		PlotToolInterface(Graph *graph, const QObject *status_target = nullptr, const char *status_slot = "") {d_status_target = status_target; d_status_slot = status_slot; d_graph = graph;};
		virtual ~PlotToolInterface() {};

        virtual int rtti() const { return Rtti_PlotTool;};

	protected:
		Graph *d_graph;
		const QObject *d_status_target;
		const char *d_status_slot;
};

#endif // ifndef PLOT_TOOL_INTERFACE_H
