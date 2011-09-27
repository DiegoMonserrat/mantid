#ifndef ONECURVEPLOT_H_
#define ONECURVEPLOT_H_

#include <qwt_plot.h>
#include <qwt_plot_item.h>
#include <QList>

class QwtPlotCurve;
class QwtPlotZoomer;
class PeakLabel;
class PeakMarker2D;
/**
  * Implements a simple widget for plotting a single curve.
  */
class OneCurvePlot: public QwtPlot
{
  Q_OBJECT
public:
  OneCurvePlot(QWidget* parent);
  void setData(const double* x,const double* y,int dataSize);
  void setYAxisLabelRotation(double degrees);
  void addLabel(PeakLabel*);
  void clearLabels();
public slots:
  void setXScale(double from, double to);
  void setYScale(double from, double to);
  void clearCurve();
  void recalcAxisDivs();
  void setYLogScale();
  void setYLinearScale();
signals:
  void showContextMenu();
  void clickedAt(double,double);
protected:
  void resizeEvent(QResizeEvent *e);
  void contextMenuEvent (QContextMenuEvent *e);
  void mousePressEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
private:
  QwtPlotCurve* m_curve;
  QwtPlotZoomer* m_zoomer; ///< does zooming
  int m_x0; ///< save x coord of last left mouse click
  int m_y0; ///< save y coord of last left mouse click
  QList<PeakLabel*> m_peakLabels;
};

class PeakLabel: public QwtPlotItem
{
public:
  PeakLabel(const PeakMarker2D* m):m_marker(m){}
  void draw(QPainter *painter, 
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRect &canvasRect) const;
private:
  const PeakMarker2D* m_marker;
};

#endif /*ONECURVEPLOT_H_*/
