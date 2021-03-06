#ifndef LINEPLOTOPTIONS_H
#define LINEPLOTOPTIONS_H

#include <QtGui/QWidget>
#include "ui_LinePlotOptions.h"
#include "MantidQtAPI/MantidQwtIMDWorkspaceData.h"
#include "MantidAPI/IMDWorkspace.h"
#include "DllOption.h"

class EXPORT_OPT_MANTIDQT_SLICEVIEWER LinePlotOptions : public QWidget
{
  Q_OBJECT

public:
  LinePlotOptions(QWidget *parent = 0, bool logScaleOption=false);
  ~LinePlotOptions();

  void setOriginalWorkspace(Mantid::API::IMDWorkspace_sptr ws);

  int getPlotAxis() const;
  void setPlotAxis(int choice);

  Mantid::API::MDNormalization getNormalization() const;
  void setNormalization(Mantid::API::MDNormalization method);
  bool isLogScaledY() const;

public slots:
  void radPlot_changed();
  void radNormalization_changed();
  void onYScalingChanged();

signals:
  /// Signal emitted when the PlotAxisChoice changes
  void changedPlotAxis();
  /// Signal emitted when the Normalization method changes
  void changedNormalization();
  /// Signal emitted when the Y log scaling changes
  void changedYLogScaling();

private:

  void addPlotRadioButton(const std::string & text, const std::string & tooltip, const bool bIntegrated = false);

  Ui::LinePlotOptionsClass ui;

  /// Vector of the various plot axis radio buttons
  QVector<QRadioButton *> m_radPlots;

  /// Chosen plot X-axis
  int m_plotAxis;

  /// Chosen normalization method
  Mantid::API::MDNormalization m_normalize;
};

#endif // LINEPLOTOPTIONS_H
