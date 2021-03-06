#include "MantidQtSliceViewer/CustomTools.h"
#include <iomanip>
#include <iosfwd>
#include <iostream>

namespace MantidQt
{
namespace SliceViewer
{

  CustomPicker::CustomPicker(int xAxis, int yAxis, QwtPlotCanvas* canvas)
  : QwtPlotPicker(xAxis, yAxis, 0, CrossRubberBand, AlwaysOn, canvas)
  {
    setSelectionFlags(QwtPicker::PointSelection);
    setRubberBand(QwtPicker::CrossRubberBand);
    canvas->setMouseTracking(true);
  }


/** Called each time the mouse moves over the canvas */
  QwtText CustomPicker::trackerText(const QwtDoublePoint & pos) const
  {
    emit mouseMoved(pos.x(), pos.y());
    return QwtText();
  }

  void CustomPicker::widgetMouseMoveEvent(QMouseEvent *e)
  {
    if ( !isActive() )
    {
      setSelectionFlags(QwtPicker::PointSelection);

      begin();
      append(e->pos());
    }

    QwtPlotPicker::widgetMouseMoveEvent(e);
  }

  void CustomPicker::widgetLeaveEvent(QEvent *)
  {
    end();
  }

  void CustomMagnifier::rescale(double factor)
  {
    if ( factor != 0.0 )
    {
      QwtPlotMagnifier::rescale(1 / factor);
      emit rescaled(factor);
    }
  }

}
}

