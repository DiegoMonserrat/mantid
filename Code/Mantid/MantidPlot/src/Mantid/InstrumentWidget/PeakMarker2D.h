#ifndef MANTIDPLOT_PEAKMARKER2D_H_
#define MANTIDPLOT_PEAKMARKER2D_H_

#include "Shape2D.h"
#include "MantidAPI/IPeak.h"

/**
 * Shape representing a peak marker on un unwrapped surface.
 * A marker consists of a symbol marking location of a peak
 * and a text label.
 */
class PeakMarker2D: public Shape2D
{
public:
  enum Symbol {Circle = 0,Diamond,Square};
  PeakMarker2D(const QPointF& centre,Symbol symbol = Circle,int markerSize = 0);
  /* --- Implemented Shape2D virtual methods --- */
  virtual Shape2D* clone()const{return new PeakMarker2D(*this);}
  virtual bool selectAt(const QPointF& p)const;
  virtual bool contains(const QPointF& p)const{return m_boundingRect.contains(p);}
  virtual void addToPath(QPainterPath& path) const;
  /* --- Own public methods --- */
  /// Set new marker size to s
  void setMarkerSize(const int& s);
  /// Get marker size
  int getMarkerSize()const{return m_markerSize;}
  /// Get default marker size
  static const int getDefaultMarkerSize(){return g_defaultMarkerSize;}
  Symbol getSymbol()const{return m_symbol;}
  void setSymbol(Symbol s){m_symbol=s;}
  void setPeak(const Mantid::API::IPeak& peak);
  double getH()const{return m_h;}
  double getK()const{return m_k;}
  double getL()const{return m_l;}
  int getDetectorID()const{return m_detID;}
  double getTOF()const{return m_tof;}
  /// Get label's area on the screen
  const QRectF& getLabelRect()const{return m_labelRect;}
  /// Allows PeakOverlay to move the label to avoid overlapping
  void moveLabelRectTo(const QPointF& p)const{m_labelRect.moveTo(p);}
  QString getLabel()const{return m_label;}
protected:
  /* --- Implemented Shape2D protected virtual methods --- */
  virtual void drawShape(QPainter& painter) const;
  virtual void refit(){}
  /* --- Own protected methods --- */
  void drawCircle(QPainter& painter)const;
  void drawDiamond(QPainter& painter)const;
  void drawSquare(QPainter& painter)const;
private:

  int m_markerSize;
  static const int g_defaultMarkerSize;
  Symbol m_symbol; ///< Shape of the marker
  double m_h, m_k, m_l; ///< Peak's h,k,l
  int m_detID;
  double m_tof;
  QString m_label;
  mutable QRectF m_labelRect; ///< label's area on the screen
};

#endif /*MANTIDPLOT_PEAKMARKER2D_H_*/
