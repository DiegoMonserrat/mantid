#include "MantidKernel/Logger.h"
#include "MantidMatrix.h"
#include "MantidMatrixFunction.h"
#include "MantidKernel/Timer.h"
#include "MantidUI.h"
#include "../Graph3D.h"
#include "../ApplicationWindow.h"
#include "../Spectrogram.h"
#include "MantidMatrixDialog.h"
#include "Preferences.h"
#include "../pixmaps.h"

#include "TSVSerialiser.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/ReadLock.h"

#include "MantidQtAPI/PlotAxis.h"

#include <QtGlobal>
#include <QTextStream>
#include <QList>
#include <QEvent>
#include <QContextMenuEvent>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QHeaderView>
#include <QApplication>
#include <QVarLengthArray>
#include <QClipboard>
#include <QShortcut>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QLocale>
#include <QItemDelegate>
#include <QLabel>
#include <QStackedWidget>
#include <QImageWriter>
#include <QSvgGenerator>
#include <QFile>
#include <QUndoStack>
#include <QCheckBox>
#include <QTabWidget>

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <limits>

#include <boost/math/special_functions/fpclassify.hpp>
//using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::API;
using namespace Mantid::Geometry;

namespace
{
  Logger g_log("MantidMatrix");
}

MantidMatrix::MantidMatrix(Mantid::API::MatrixWorkspace_const_sptr ws, ApplicationWindow* parent, const QString& label, const QString& name, int start, int end)
  : MdiSubWindow(parent, label, name, 0),
  WorkspaceObserver(),
  m_appWindow(parent),
  m_workspace(ws),
  y_start(0.0),y_end(0.0),
  m_histogram(false),
  m_min(0),m_max(0),
  m_are_min_max_set(false),
  m_boundingRect(),
  m_strName(name.toStdString()),
  m_selectedRows(),
  m_selectedCols()
{
  setup(ws,start,end);
  setWindowTitle(name);
  setName(name);

  m_modelY = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::Y);
  m_table_viewY = new QTableView();
  connectTableView(m_table_viewY,m_modelY);
  setColumnsWidth(0,MantidPreferences::MantidMatrixColumnWidthY());
  setNumberFormat(0,MantidPreferences::MantidMatrixNumberFormatY(),
    MantidPreferences::MantidMatrixNumberPrecisionY());

  m_modelX = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::X);
  m_table_viewX = new QTableView();
  connectTableView(m_table_viewX,m_modelX);
  setColumnsWidth(1,MantidPreferences::MantidMatrixColumnWidthX());
  setNumberFormat(1,MantidPreferences::MantidMatrixNumberFormatX(),
    MantidPreferences::MantidMatrixNumberPrecisionX());

  m_modelE = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::E);
  m_table_viewE = new QTableView();
  connectTableView(m_table_viewE,m_modelE);
  setColumnsWidth(2,MantidPreferences::MantidMatrixColumnWidthE());
  setNumberFormat(2,MantidPreferences::MantidMatrixNumberFormatE(),
    MantidPreferences::MantidMatrixNumberPrecisionE());

  m_YTabLabel = QString("Y values");
  m_XTabLabel = QString("X values");
  m_ETabLabel = QString("Errors");

  m_tabs = new QTabWidget(this);
  m_tabs->insertTab(0,m_table_viewY, m_YTabLabel);
  m_tabs->insertTab(1,m_table_viewX, m_XTabLabel);
  m_tabs->insertTab(2,m_table_viewE, m_ETabLabel);

  setWidget(m_tabs);
  //for synchronizing the views
  //index is zero for the defualt view
  m_PrevIndex=0;
  //install event filter on  these objects
  m_table_viewY->installEventFilter(this);
  m_table_viewX->installEventFilter(this);
  m_table_viewE->installEventFilter(this);

  connect(m_tabs,SIGNAL(currentChanged(int)),this,SLOT(viewChanged(int)));

  setGeometry(50, 50, QMIN(5, numCols())*m_table_viewY->horizontalHeader()->sectionSize(0) + 55,
    (QMIN(10,numRows())+1)*m_table_viewY->verticalHeader()->sectionSize(0)+100);

  observeAfterReplace();
  observePreDelete();
  observeADSClear();

  connect(this,SIGNAL(needWorkspaceChange(Mantid::API::MatrixWorkspace_sptr)),this,SLOT(changeWorkspace(Mantid::API::MatrixWorkspace_sptr))); 
  connect(this,SIGNAL(needToClose()),this,SLOT(closeMatrix()));

  connect(this, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(selfClosed(MdiSubWindow*)));

  confirmClose(false);
}

bool MantidMatrix::eventFilter(QObject *object, QEvent *e)
{
  // if it's context menu on any of the views
  if (e->type() == QEvent::ContextMenu && (object == m_table_viewY || object == m_table_viewX || object == m_table_viewE)){
    e->accept();
    emit showContextMenu();
    return true;
  }
  return MdiSubWindow::eventFilter(object, e);
}

/** Called when switching between tabs
*  @param index :: The index of the new active tab
*/
void MantidMatrix::viewChanged(int index)
{
  // get the previous view and selection model
  QTableView* prevView=(QTableView*)m_tabs->widget(m_PrevIndex);
  if(prevView)
  {
    QItemSelectionModel *oldSelModel = prevView->selectionModel();
    QItemSelectionModel *selModel = activeView()->selectionModel();
    // Copy the selection from the previous tab into the newly-activated one
    selModel->select(oldSelModel->selection(),QItemSelectionModel::Select);
    // Clear the selection on the now-hidden tab
    oldSelModel->clearSelection();

    m_PrevIndex=index;
    //get the previous tab scrollbar positions
    int hValue=  prevView->horizontalScrollBar()->value();
    int vValue = prevView->verticalScrollBar()->value();
    //to synchronize the views
    //set  the previous view  scrollbar positions to current view
    activeView()->horizontalScrollBar()->setValue(hValue);
    activeView()->verticalScrollBar()->setValue(vValue);
  }

}

/// Checks if d is not infinity or a NaN
bool isANumber(volatile const double& d)
{
  return d != std::numeric_limits<double>::infinity() && !boost::math::isnan(d);
}

void MantidMatrix::setup(Mantid::API::MatrixWorkspace_const_sptr ws, int start, int end)
{
  if (!ws)
  {
    QMessageBox::critical(0,"WorkspaceMatrixModel error","2D workspace expected.");
    m_rows = 0;
    m_cols = 0;
    m_startRow = 0;
    m_endRow = 0;
    return;
  }

  m_workspace = ws;
  m_workspaceTotalHist = static_cast<int>(ws->getNumberHistograms());
  m_startRow = (start<0 || start>=m_workspaceTotalHist)?0:start;
  m_endRow   = (end<0 || end>=m_workspaceTotalHist || end < start)? m_workspaceTotalHist - 1 : end;
  m_rows = m_endRow - m_startRow + 1;
  m_cols = static_cast<int>(ws->blocksize());
  if ( ws->isHistogramData() ) m_histogram = true;
  connect(this,SIGNAL(needsUpdating()),this,SLOT(repaintAll()));


  m_bk_color = QColor(128, 255, 255);
  m_matrix_icon = getQPixmap("mantid_matrix_xpm");
  m_column_width = 100;

}

void MantidMatrix::connectTableView(QTableView* view,MantidMatrixModel*model)
{
  view->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
  view->setSelectionMode(QAbstractItemView::ExtendedSelection);
  view->setModel(model);
  view->setCornerButtonEnabled(false);
  view->setFocusPolicy(Qt::StrongFocus);

  QPalette pal = view->palette();
  pal.setColor(QColorGroup::Base, m_bk_color);
  view->setPalette(pal);

  // set header properties
  QHeaderView* hHeader = (QHeaderView*)view->horizontalHeader();
  hHeader->setMovable(false);
  hHeader->setResizeMode(QHeaderView::Interactive);
  hHeader->setDefaultSectionSize(m_column_width);

  view->resizeRowToContents(0);
  int row_height = view->rowHeight(0);

  QHeaderView* vHeader = (QHeaderView*)view->verticalHeader();
  vHeader->setDefaultSectionSize(row_height);
  vHeader->setResizeMode(QHeaderView::Fixed);
  vHeader->setMovable(false);
}

double MantidMatrix::cell(int row, int col)
{
  return m_modelY->data(row, col);
}

QString MantidMatrix::text(int row, int col)
{
  const int precision = 15;
  const char format = 'g';
  return QString::number(activeModel()->data(row, col),format,precision);
}

/** Sets new column width in a table view(s).
@param width :: New column width in pixels. All columns have the same width.
@param all :: If true the change will be applied to all three table views.
*/
void MantidMatrix::setColumnsWidth(int width, bool all)
{
  if (all)
  {
    m_table_viewY->horizontalHeader()->setDefaultSectionSize(width);
    m_table_viewX->horizontalHeader()->setDefaultSectionSize(width);
    m_table_viewE->horizontalHeader()->setDefaultSectionSize(width);

    int cols = numCols();
    for(int i=0; i<cols; i++)
    {
      m_table_viewY->setColumnWidth(i, width);
      m_table_viewX->setColumnWidth(i, width);
      m_table_viewE->setColumnWidth(i, width);
    }
    MantidPreferences::MantidMatrixColumnWidth(width);
  }
  else
  {
    QTableView* table_view = activeView();
    table_view->horizontalHeader()->setDefaultSectionSize(width);
    int cols = numCols();
    for(int i=0; i<cols; i++)
      table_view->setColumnWidth(i, width);
    switch (m_tabs->currentIndex())
    {
    case 0: MantidPreferences::MantidMatrixColumnWidthY(width);break;
    case 1: MantidPreferences::MantidMatrixColumnWidthX(width);break;
    case 2: MantidPreferences::MantidMatrixColumnWidthE(width);break;
    }
  }

  emit modifiedWindow(this);
}

/**  Sets column width to one table view.
@param i :: ordinal number of the view. 0 - Y, 1 - X, 2 - Error
@param width :: New column width in pixels. All columns have the same width.
*/
void MantidMatrix::setColumnsWidth(int i,int width)
{

  QTableView* table_view;
  switch(i)
  {
  case 0: table_view =  m_table_viewY; MantidPreferences::MantidMatrixColumnWidthY(width); break;
  case 1: table_view =  m_table_viewX; MantidPreferences::MantidMatrixColumnWidthX(width); break;
  case 2: table_view =  m_table_viewE; MantidPreferences::MantidMatrixColumnWidthE(width); break;
  default: table_view = activeView();
  };

  table_view->horizontalHeader()->setDefaultSectionSize(width);
  int cols = numCols();
  for(int i=0; i<cols; i++)
    table_view->setColumnWidth(i, width);

  emit modifiedWindow(this);
}

/**  Returns the width of a column.
@param i :: ordinal number of the view. 0 - Y, 1 - X, 2 - Error
@return The column width in pixels. All columns have the same width.
*/
int MantidMatrix::columnsWidth(int i)
{
  switch(i)
  {
  case 0: return m_table_viewY->columnWidth(0);
  case 1: return m_table_viewX->columnWidth(0);
  case 2: return m_table_viewE->columnWidth(0);
  };
  return activeView()->columnWidth(0);
}

/**  Return the pointer to the active table view.
*/
QTableView *MantidMatrix::activeView()
{
  switch (m_tabs->currentIndex())
  {
  case 0: return m_table_viewY;
  case 1: return m_table_viewX;
  case 2: return m_table_viewE;
  }
  return m_table_viewY;
}

/**  Returns the pointer to the active model.
*/
MantidMatrixModel *MantidMatrix::activeModel()
{
  switch (m_tabs->currentIndex())
  {
  case 0: return m_modelY;
  case 1: return m_modelX;
  case 2: return m_modelE;
  }
  return m_modelY;
}

/**  Copies the current selection in the active table view into the system clipboard.
*/
void MantidMatrix::copySelection()
{
  QItemSelectionModel *selModel = activeView()->selectionModel();
  QString s = "";
  QString eol = applicationWindow()->endOfLine();
  if (!selModel->hasSelection()){
    QModelIndex index = selModel->currentIndex();
    s = text(index.row(), index.column());
  } else {
    QItemSelection sel = selModel->selection();
    QListIterator<QItemSelectionRange> it(sel);
    if(!it.hasNext())
      return;

    QItemSelectionRange cur = it.next();
    int top = cur.top();
    int bottom = cur.bottom();
    int left = cur.left();
    int right = cur.right();
    for(int i=top; i<=bottom; i++){
      for(int j=left; j<right; j++)
        s += text(i, j) + "\t";
      s += text(i,right) + eol;
    }
  }
  // Copy text into the clipboard
  QApplication::clipboard()->setText(s.trimmed());
}

/**  Returns minimum and maximum values in the matrix.
If setRange(...) has not been called it returns the true smalles ang largest Y-values in the matrix,
otherwise the values set with setRange(...) are returned. These are needed in plotGraph2D to set
the range of the third, colour axis.
@param[out] min is set to the minumium value
@param[out] max is set to the maximum value
*/
void MantidMatrix::range(double *min, double *max)
{
  if (!m_are_min_max_set)
  {
    findYRange(m_workspace, m_min, m_max);
    m_are_min_max_set = true;
  }
  *min = m_min;
  *max = m_max;
}



/**  Sets new minimum and maximum Y-values which can be displayed in a 2D graph
*/
void MantidMatrix::setRange(double min, double max)
{
  m_min = min;
  m_max = max;
  m_are_min_max_set = true;
}

double** MantidMatrix::allocateMatrixData(int rows, int columns)
{
  double** data = (double **)malloc(rows * sizeof (double*));
  if(!data){
    QMessageBox::critical(0, tr("MantidPlot") + " - " + tr("Memory Allocation Error"),
      tr("Not enough memory, operation aborted!"));
    return NULL;
  }

  for ( int i = 0; i < rows; ++i){
    data[i] = (double *)malloc(columns * sizeof (double));
    if(!data[i]){
      for ( int j = 0; j < i; j++)
        free(data[j]);
      free(data);

      QMessageBox::critical(0, tr("MantidPlot") + " - " + tr("Memory Allocation Error"),
        tr("Not enough memory, operation aborted!"));
      return NULL;
    }
  }
  return data;
}

void MantidMatrix::freeMatrixData(double **data, int rows)
{
  for ( int i = 0; i < rows; i++)
    free(data[i]);

  free(data);
}

void MantidMatrix::goTo(int row,int col)
{
  if(row < 1 || row > numRows())
    return;
  if(col < 1 || col > numCols())
    return;

  activeView()->selectionModel()->select(activeModel()->index(row - 1, col - 1), QItemSelectionModel::ClearAndSelect);
  activeView()->scrollTo(activeModel()->index(row - 1, col - 1), QAbstractItemView::PositionAtTop);
}

void MantidMatrix::goToRow(int row)
{
  if(row < 1 || row > numRows())
    return;

  //	activeView()->selectRow(row - 1); //For some reason, this did not highlight the row at all, hence the stupid line below
  activeView()->selectionModel()->select(QItemSelection(activeModel()->index(row - 1, 0), activeModel()->index(row - 1, numCols() - 1)), QItemSelectionModel::ClearAndSelect);

  activeView()->scrollTo(activeModel()->index(row - 1, 0), QAbstractItemView::PositionAtCenter);
}

void MantidMatrix::goToColumn(int col)
{
  if(col < 1 || col > numCols())
    return;

  //	activeView()->selectColumn(col - 1); //For some reason, this did not highlight the row at all, hence the stupid line below
  activeView()->selectionModel()->select(QItemSelection(activeModel()->index(0, col - 1), activeModel()->index(numRows() - 1, col - 1)), QItemSelectionModel::ClearAndSelect);
  activeView()->scrollTo(activeModel()->index(0, col - 1), QAbstractItemView::PositionAtCenter);

}

double MantidMatrix::dataX(int row, int col) const
{
  if (!m_workspace || row >= numRows() || col >= static_cast<int>(m_workspace->readX(row + m_startRow).size())) return 0.;
  double res = m_workspace->readX(row + m_startRow)[col];
  return res;

}

double MantidMatrix::dataY(int row, int col) const
{
  if (!m_workspace || row >= numRows() || col >= numCols()) return 0.;
  double res = m_workspace->readY(row + m_startRow)[col];
  return res;

}

double MantidMatrix::dataE(int row, int col) const
{
  if (!m_workspace || row >= numRows() || col >= numCols()) return 0.;
  double res = m_workspace->readE(row + m_startRow)[col];
  return res;

}

QString MantidMatrix::workspaceName() const
{
  return QString::fromStdString(m_strName);
}

QwtDoubleRect MantidMatrix::boundingRect()
{
  if (m_boundingRect.isNull())
  {
    m_spectrogramRows = numRows() > 100 ? numRows() : 100;

    // This is only meaningful if a 2D (or greater) workspace
    if (m_workspace->axes() > 1)
    {
      const Mantid::API::Axis* const ax = m_workspace->getAxis(1);
      y_start =(*ax)(m_startRow);
      y_end   =(*ax)(m_endRow);
    }

    double dy = fabs(y_end - y_start)/(double)(numRows() - 1);

    int i0 = m_startRow;
    x_start = x_end = 0;
    while(x_start == x_end && i0 <= m_endRow)
    {
      const Mantid::MantidVec& X = m_workspace->readX(i0);
      x_start = X[0];
      if (X.size() != m_workspace->readY(i0).size()) x_end = X[m_workspace->blocksize()];
      else
        x_end = X[m_workspace->blocksize()-1];
      if ( !isANumber(x_start) || !isANumber(x_end))
      {
        x_start = x_end = 0;
      }
      i0++;
    }

    // if i0 > m_endRow there aren't any plottable rows
    if (i0 <= m_endRow)
    {
      // check if all X vectors are the same
      bool theSame = true;
      double dx = 0.;
      for(int i=i0;i<=m_endRow;++i)
      {
        if (m_workspace->readX(i).front() != x_start || m_workspace->readX(i).back() != x_end)
        {
          theSame = false;
          break;
        }
      }
      dx = fabs(x_end - x_start)/(double)(numCols() - 1);

      if ( !theSame )
      {
        // Find the smallest bin width and thus the number of columns in a spectrogram
        // that can be plotted from this matrix
        double ddx = dx;
        for(int i=m_startRow+1;i<=m_endRow;++i)
        {
          const Mantid::MantidVec& X = m_workspace->readX(i);
          if (X.front() < x_start)
          {
            double xs = X.front();
            if (!isANumber(xs)) continue;
            x_start = xs;
          }
          if (X.back() > x_end)
          {
            double xe = X.back();
            if (!isANumber(xe)) continue;
            x_end = xe;
          }
          for(int j=1;j<static_cast<int>(X.size());++j)
          {
            double d = X[j] - X[j-1];
            if (ddx == 0 && d < ddx)
            {
              ddx = d;
            }
          }
        }
        m_spectrogramCols = static_cast<int>((x_end - x_start)/ddx);
        if (m_spectrogramCols < 100) m_spectrogramCols = 100;
      }
      else
      {
        m_spectrogramCols = numCols() > 100 ? numCols() : 100;
      }
      m_boundingRect = QwtDoubleRect(QMIN(x_start, x_end) - 0.5*dx, QMIN(y_start, y_end) - 0.5*dy,
        fabs(x_end - x_start) + dx, fabs(y_end - y_start) + dy).normalized();

    }
    else
    {
      m_spectrogramCols = 0;
      m_boundingRect = QwtDoubleRect(0, QMIN(y_start, y_end) - 0.5*dy,
        1, fabs(y_end - y_start) + dy).normalized();
    }
  }// Define the spectrogram bounding box
  return m_boundingRect;
}

//----------------------------------------------------------------------------

Graph3D * MantidMatrix::plotGraph3D(int style)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  ApplicationWindow* a = applicationWindow();
  QString labl = a->generateUniqueName(tr("Graph"));

  Graph3D *plot = new Graph3D("", a);
  plot->resize(500,400);
  plot->setWindowTitle(labl);
  plot->setName(labl);
  plot->setTitle(tr("Workspace ")+name());
  a->customPlot3D(plot);
  plot->customPlotStyle(style);
  int resCol = numCols() / 200;
  int resRow = numRows() / 200;
  plot->setResolution( qMax(resCol,resRow) );

  double zMin =  1e300;
  double zMax = -1e300;
  for(int i=0;i<numRows();i++)
    for(int j=0;j<numCols();j++)
    {
      if (cell(i,j) < zMin) zMin = cell(i,j);
      if (cell(i,j) > zMax) zMax = cell(i,j);
    }

    // Calculate xStart(), xEnd(), yStart(), yEnd()
    boundingRect();

    MantidMatrixFunction *fun = new MantidMatrixFunction(*this);
    plot->addFunction(fun, xStart(), xEnd(), yStart(), yEnd(), zMin, zMax, numCols(), numRows() );

    using MantidQt::API::PlotAxis;
    plot->setXAxisLabel(PlotAxis(*m_workspace, 0).title());
    plot->setYAxisLabel(PlotAxis(*m_workspace, 1).title());
    plot->setZAxisLabel(PlotAxis(false, *m_workspace).title());

    a->initPlot3D(plot);
    //plot->confirmClose(false);
    QApplication::restoreOverrideCursor();

    return plot;
}

void MantidMatrix::attachMultilayer(MultiLayer* ml)
{
  m_plots2D << ml;
  connect(ml, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(dependantClosed(MdiSubWindow*)));
}


/** Creates a MultiLayer graph and plots this MantidMatrix as a Spectrogram.

@param type :: The "curve" type.
@return Pointer to the created graph.
*/
MultiLayer* MantidMatrix::plotGraph2D(Graph::CurveType type)
{
  if (numRows() == 1)
  {
    QMessageBox::critical(0,"MantidPlot - Error","Cannot plot a workspace with only one spectrum.");
    return NULL;
  }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  ApplicationWindow *a = applicationWindow();
  MultiLayer* g = a->multilayerPlot(a->generateUniqueName(tr("Graph")));
  attachMultilayer( g );
  //#799 fix for  multiple dialog creation on double clicking/ on right click menu scale on  2d plot
  //   a->connectMultilayerPlot(g);
  Graph* plot = g->activeGraph();
  plotSpectrogram(plot,a,type,false,NULL);
  // g->confirmClose(false);
  QApplication::restoreOverrideCursor();
  return g;
}

Spectrogram* MantidMatrix::plotSpectrogram(Graph* plot, ApplicationWindow* app, Graph::CurveType type,bool project,const ProjectData* const prjData)
{
  app->setPreferences(plot);

  plot->setTitle(tr("Workspace ") + name());

  using MantidQt::API::PlotAxis;
  plot->setXAxisTitle(PlotAxis(*m_workspace, 0).title());
  plot->setYAxisTitle(PlotAxis(*m_workspace, 1).title());

  // Set the range on the third, colour axis
  double minz, maxz;
  auto fun = new MantidMatrixFunction(*this);
  range(&minz,&maxz);
  Spectrogram *spgrm = plot->plotSpectrogram(fun, m_spectrogramRows, m_spectrogramCols, boundingRect(), minz, maxz, type);
  app->setSpectrogramTickStyle(plot);
  if( spgrm )
  {
    if(project)
    {
      spgrm->mutableColorMap().loadMap(prjData->getColormapFile());
      spgrm->setCustomColorMap(spgrm->mutableColorMap());
      spgrm->setIntensityChange(prjData->getIntensity());
      if(!prjData->getGrayScale())spgrm->setGrayScale();
      if(prjData->getContourMode())
      {spgrm->setDisplayMode(QwtPlotSpectrogram::ContourMode, true);
      spgrm->showContourLineLabels(true);
      }
      spgrm->setDefaultContourPen(prjData->getDefaultContourPen());
      spgrm->setColorMapPen(false);
      if(prjData->getColorMapPen())spgrm->setColorMapPen(true);
      ContourLinesEditor* contourEditor=prjData->getContourLinesEditor();
      if(contourEditor) 
      {
        contourEditor->setSpectrogram(spgrm);
        contourEditor->updateContents();
        contourEditor->updateContourLevels();
      }
    }

  }
  plot->setAutoScale();
  return spgrm;
}

void MantidMatrix::setBinGraph(MultiLayer *ml, Table* t)
{
  MantidUI::setUpBinGraph(ml,name(),workspace());
  connect(ml, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(dependantClosed(MdiSubWindow*)));
  if (t)
  {
    m_plots1D[ml] = t;
    connect(t, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(dependantClosed(MdiSubWindow*)));
  }
  else
    m_plots2D<<ml;
}

/// Returns a list of the selected rows
const QList<int>& MantidMatrix::getSelectedRows() const
{
  return m_selectedRows;
}

/**
* Sets the internal cache of selected rows.
* @return True if rows are selected, false otherwise
*/
bool MantidMatrix::setSelectedRows()
{
  QTableView *tv = activeView();
  QItemSelectionModel *selModel = tv->selectionModel();
  if( !selModel  ) return false;

  m_selectedRows.clear();
  const QModelIndexList rows = selModel->selectedRows();
  QModelIndexList::const_iterator it;
  for ( it = rows.constBegin(); it != rows.constEnd(); ++it)
  {
    m_selectedRows.append(it->row()+m_startRow);
  }

  return (m_selectedRows.empty() ? false : true);
}

/// Returns a list of the selected columns
const QList<int>& MantidMatrix::getSelectedColumns() const
{
  return m_selectedCols;
}

/**
* Sets the internal cache of selected columns.
* @return True if columns are selected, false otherwise
*/
bool MantidMatrix::setSelectedColumns()
{
  QTableView *tv = activeView();
  QItemSelectionModel *selModel = tv->selectionModel();
  if( !selModel ||  !selModel->hasSelection()) return false;

  m_selectedCols.clear();
  const QModelIndexList cols = selModel->selectedColumns();
  QModelIndexList::const_iterator it;
  for ( it = cols.constBegin(); it != cols.constEnd(); ++it)
  {
    m_selectedCols.append(it->column());
  }

  return (m_selectedCols.empty() ? false : true);
}

void MantidMatrix::dependantClosed(MdiSubWindow* w)
{
  if( w->isA("Table") )
  {
    QMap<MultiLayer*,Table*>::iterator itr;
    for( itr = m_plots1D.begin(); itr != m_plots1D.end(); ++itr )
    {
      if( itr.value() == dynamic_cast<Table*>(w) )
      {
        m_plots1D.erase(itr);
        break;
      }
    }
  }
  else if (w->isA("MultiLayer"))
  {
    int i = m_plots2D.indexOf(dynamic_cast<MultiLayer*>(w));
    if (i >= 0)m_plots2D.remove(i);
    else
    {QMap<MultiLayer*,Table*>::iterator i = m_plots1D.find(dynamic_cast<MultiLayer*>(w));
    if (i != m_plots1D.end())
    {
      if (i.value() != 0)
      {
        i.value()->confirmClose(false);
        i.value()->close();
      }
      m_plots1D.erase(i);
    }
    }
  }
}

/**
Repaints all 1D and 2D plots attached to this MantidMatrix
*/
void MantidMatrix::repaintAll()
{
  repaint();

  // Repaint 2D plots
  QVector<MultiLayer*>::iterator vEnd  = m_plots2D.end();
  for( QVector<MultiLayer*>::iterator vItr = m_plots2D.begin(); vItr != vEnd; ++vItr )
  {
    (*vItr)->activeGraph()->replot();
  }

  // Updates the 1D plots by modifying the attached tables
  QMap<MultiLayer*,Table*>::iterator mEnd = m_plots1D.end();
  for(QMap<MultiLayer*,Table*>::iterator mItr = m_plots1D.begin(); mItr != mEnd;  ++mItr)
  {
    Table* t = mItr.value();
    if ( !t ) continue;
    int charsToRemove = t->name().size() + 1;
    int nTableCols(t->numCols());
    for(int col = 1; col < nTableCols; ++col)
    {
      QString colName = t->colName(col).remove(0,charsToRemove);
      if( colName.isEmpty() ) break;
      //Need to determine whether the table was created from plotting a spectrum
      //or a time bin. A spectrum has a Y column name YS and a bin YB
      QString ident = colName.left(2);
      colName.remove(0,2); //This now contains the number in the MantidMatrix
      int matrixNumber = colName.toInt();
      if( matrixNumber < 0 ) break;
      bool errs = (ident[0] == QChar('E')) ? true : false;
      if( ident[1] == QChar('S') )
      {
        if( matrixNumber >= numRows() ) break;
        int endCount = numCols();
        for(int j = 0; j < endCount; ++j)
        {
          if( errs ) t->setCell(j, col, dataE(matrixNumber, j));
          else t->setCell(j, col, dataY(matrixNumber, j));
        }
      }
      else
      {
        if( matrixNumber >= numCols() ) break;
        int endCount = numRows();
        for(int j = 0; j < endCount; ++j)
        {
          if( errs ) t->setCell(j, col, dataE(j, matrixNumber));
          else t->setCell(j, col, dataY(j, matrixNumber));
        }
      }
    }
    t->notifyChanges();
  }
}

void MantidMatrix::afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  if ( !ws ) return;
  if( wsName != m_strName )
  {
    if ( ws == m_workspace ) // i.e. this is a rename
    {
      m_strName = wsName;
      QString qwsName = QString::fromStdString(wsName);
      setWindowTitle(qwsName);
      setName(qwsName);
      setObjectName(qwsName);
    }
    return;
  }

  Mantid::API::MatrixWorkspace_sptr new_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(m_strName));

  //If the cast failed (e.g. Matrix2D became a GroupWorkspace) do not try to change the matrix, just close it
  if(new_workspace)
  {
    emit needWorkspaceChange( new_workspace );
  }
  else
  {
    g_log.warning("Workspace type changed. Closing matrix window.");
    emit needToClose();
  }
}

void MantidMatrix::changeWorkspace(Mantid::API::MatrixWorkspace_sptr ws)
{
  if (m_cols != static_cast<int>(ws->blocksize()) ||
    m_workspaceTotalHist != static_cast<int>(ws->getNumberHistograms()))
  {
    closeDependants();
  }

  // Save selection
  QItemSelectionModel *oldSelModel = activeView()->selectionModel();
  QModelIndexList indexList = oldSelModel->selectedIndexes();
  QModelIndex curIndex = activeView()->currentIndex();

  setup(ws,-1,-1);

  m_modelY = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::Y);
  connectTableView(m_table_viewY,m_modelY);

  m_modelX = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::X);
  connectTableView(m_table_viewX,m_modelX);

  m_modelE = new MantidMatrixModel(this,ws.get(),m_rows,m_cols,m_startRow,MantidMatrixModel::E);
  connectTableView(m_table_viewE,m_modelE);

  // Restore selection
  activeView()->setCurrentIndex(curIndex);
  if (indexList.size())
  {
    QItemSelection sel(indexList.first(),indexList.last());
    QItemSelectionModel *selModel = activeView()->selectionModel();
    selModel->select(sel,QItemSelectionModel::Select);
  }

  invalidateBoundingRect();

  repaintAll();

}

void MantidMatrix::closeDependants()
{
  while(m_plots2D.size())
  {
    MultiLayer* ml = m_plots2D.front();
    ml->confirmClose(false);
    ml->close();// this calls slot dependantClosed() which removes the pointer from m_plots2D
  }

  while(m_plots1D.size())
  {
    MultiLayer* ml = m_plots1D.begin().key();
    ml->confirmClose(false);
    ml->close();// this calls slot dependantClosed() which removes the pointer from m_plots1D
  }

}

void MantidMatrix::setNumberFormat(const QChar& f,int prec, bool all)
{
  if (all)
  {
    modelY()->setFormat(f,prec);
    modelX()->setFormat(f,prec);
    modelE()->setFormat(f,prec);
    MantidPreferences::MantidMatrixNumberFormat(f);
    MantidPreferences::MantidMatrixNumberPrecision(prec);
  }
  else
  {
    activeModel()->setFormat(f,prec);
    switch (m_tabs->currentIndex())
    {
    case 0: MantidPreferences::MantidMatrixNumberFormatY(f);
      MantidPreferences::MantidMatrixNumberPrecisionY(prec);
      break;
    case 1: MantidPreferences::MantidMatrixNumberFormatX(f);
      MantidPreferences::MantidMatrixNumberPrecisionX(prec);
      break;
    case 2: MantidPreferences::MantidMatrixNumberFormatE(f);
      MantidPreferences::MantidMatrixNumberPrecisionE(prec);
      break;
    }
  }
}

void MantidMatrix::setNumberFormat(int i,const QChar& f,int prec, bool all)
{
  (void) all; //Avoid unused warning
  switch (i)
  {
  case 0: m_modelY->setFormat(f,prec);
    MantidPreferences::MantidMatrixNumberFormatY(f);
    MantidPreferences::MantidMatrixNumberPrecisionY(prec);
    break;
  case 1: m_modelX->setFormat(f,prec);
    MantidPreferences::MantidMatrixNumberFormatX(f);
    MantidPreferences::MantidMatrixNumberPrecisionX(prec);
    break;
  case 2: m_modelE->setFormat(f,prec);
    MantidPreferences::MantidMatrixNumberFormatE(f);
    MantidPreferences::MantidMatrixNumberPrecisionE(prec);
    break;
  }
}

QChar MantidMatrix::numberFormat()
{
  return activeModel()->format();
}

int MantidMatrix::precision()
{
  return activeModel()->precision();
}



void MantidMatrix::setMatrixProperties()
{
  MantidMatrixDialog dlg(m_appWindow);
  dlg.setMatrix(this);
  dlg.exec();
}

void MantidMatrix::preDeleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  (void) wsName; //Avoid unused warning
  if (m_workspace.get() == ws.get())
  {
    emit needToClose();
  }
}

void MantidMatrix::clearADSHandle()
{
  emit needToClose();
}


void MantidMatrix::closeMatrix()
{
  confirmClose(false);
  close();
}

void MantidMatrix::selfClosed(MdiSubWindow* w)
{
  (void) w; //Avoid unused warning
  closeDependants();
}

//-------------------------------
// Python API commands
//------------------------------

void MantidMatrix::goToTab(const QString & name)
{
  if( m_tabs->tabText(m_tabs->currentIndex()) == name ) return;

  if( name == m_YTabLabel )
  {
    m_tabs->setCurrentIndex(0);
  }
  else if( name == m_XTabLabel )
  {
    m_tabs->setCurrentIndex(1);
  }
  else if( name == m_ETabLabel )
  {
    m_tabs->setCurrentIndex(2);
  }
  else return;
}

/**  returns the workspace name
*/
const std::string & MantidMatrix::getWorkspaceName()
{return m_strName;
}

// ----------   MantidMatrixModel   ------------------ //

/**   MantidMatrixModel constructor.
@param parent :: Pointer to the parent MantidMatrix
@param ws :: Underlying workspace
@param rows :: Number of rows in the workspace to be visible via MantidMatrixModel
@param cols :: Number of columns (time bins)
@param start :: Starting index
@param type :: Type of the data to display: Y, X, or E
*/
MantidMatrixModel::MantidMatrixModel(QObject *parent,
                                     const Mantid::API::MatrixWorkspace* ws,
                                     int rows,
                                     int cols,
                                     int start,
                                     Type type):
QAbstractTableModel(parent),m_type(type),
  m_format('e'),m_prec(6)
{
  setup(ws,rows,cols,start);
}

/// Call this function if the workspace has changed
void MantidMatrixModel::setup(const Mantid::API::MatrixWorkspace* ws,
                              int rows,
                              int cols,
                              int start)
{
  m_workspace = ws;
  m_rows = rows;
  m_cols = cols;
  m_colNumCorr = 1;
  m_endRow = m_rows - 1;
  m_startRow = start >= 0? start : 0;
  m_mon_color = QColor(255,255,204);
  if (ws->blocksize() != 0)
    m_colNumCorr = ws->isHistogramData() ? 1 : 0;
  else
    m_colNumCorr = 0;
}


double MantidMatrixModel::data(int row, int col) const
{
  Mantid::Kernel::ReadLock _lock(*m_workspace);

  double val;
  if (m_type == X)
  {
    val = m_workspace->readX(row + m_startRow)[col];
  }
  else if (m_type == Y)
  {
    val = m_workspace->readY(row + m_startRow)[col];
  }
  else
  {
    val = m_workspace->readE(row + m_startRow)[col];
  }
  return val;
}

QVariant MantidMatrixModel::headerData(int section, Qt::Orientation orientation, int role ) const
{
  if (!(role == Qt::DisplayRole || role == Qt::ToolTipRole)) return QVariant();
  if (orientation == Qt::Vertical && m_workspace->axes() > 1)
  {
    Mantid::API::TextAxis* xAxis = dynamic_cast<Mantid::API::TextAxis*>(m_workspace->getAxis(1));
    if (xAxis)
    {
      return QString::fromStdString(xAxis->label(section));
    }
  }

  //initialise with horizontal values
  int axisIndex = 0;
  QString toolTipSeperator = "\n";
  QString headerSeperator = "\n";
  if (orientation == Qt::Vertical) 
  {
    axisIndex = 1;
    toolTipSeperator = "\n";
    headerSeperator = " ";
  }

  if (m_workspace->axes() > axisIndex) //if the axis exists
  {
    Mantid::API::Axis* axis = m_workspace->getAxis(axisIndex);
    Mantid::API::TextAxis* textAxis = dynamic_cast<Mantid::API::TextAxis*>(axis);
    if (textAxis) //just return the text label
    {
      return QString::fromStdString(textAxis->label(section));
    }

    Mantid::API::SpectraAxis* spAxis = dynamic_cast<Mantid::API::SpectraAxis*>(axis);
    if (spAxis)
    {
      if (role == Qt::ToolTipRole) 
      {
        return QString("index %1%2spectra no %3").arg(QString::number(section), toolTipSeperator, 
          QString::number(m_workspace->getSpectrum(section)->getSpectrumNo()));          
      }
      else
      {
        return QString("%1%2sp-%3").arg(QString::number(section), headerSeperator,
          QString::number(m_workspace->getSpectrum(section)->getSpectrumNo()));
      }
    }

    
    QString unit = QString::fromStdWString( axis->unit()->label().utf8());

    // Handle RefAxis for X axis
    Mantid::API::RefAxis* refAxis = dynamic_cast<Mantid::API::RefAxis*>(axis);
    if (refAxis && axisIndex == 0)
    {
      //still need to protect from ragged workspaces
      if (m_type==X)
      {
        if (role == Qt::ToolTipRole) 
        {
          return QString("index %1").arg(QString::number(section));
        }
        else
        {
          return section;
        }
      }

      if (!m_workspace->isCommonBins())
      {
        if (role == Qt::ToolTipRole) 
        {
          return QString("index %1%2bin centre value varies%3Rebin to set common bins").arg(QString::number(section),toolTipSeperator,toolTipSeperator);
        }
        else
        {
          return QString("%1%2bins vary").arg(QString::number(section),headerSeperator);
        }
      }

      //get bin centre value
      double binCentreValue;
      const Mantid::MantidVec xVec = m_workspace->readX(0);
      if (m_workspace->isHistogramData())
      {
        if ((section+1) >= static_cast<int>(xVec.size())) return section;
        binCentreValue= (xVec[section] + xVec[section+1])/2.0;
      }
      else
      {
        if (section >= static_cast<int>(xVec.size())) return section;
        binCentreValue = xVec[section];
      }

      if (role == Qt::ToolTipRole) 
      {
        return QString("index %1%2%3 %4%5 (bin centre)").arg(QString::number(section), toolTipSeperator,
          QString::fromStdString(axis->unit()->caption()),
          QString::number(binCentreValue), unit);
      }
      else
      {
        return QString("%1%2%3%4").arg(QString::number(section), headerSeperator,
          QString::number(binCentreValue), unit);
      }
    }

    // Handle BinEdgeAxis for vertical axis
    Mantid::API::BinEdgeAxis* binEdgeAxis = dynamic_cast<Mantid::API::BinEdgeAxis*>(axis);
    if (binEdgeAxis && axisIndex == 1)
    {
      const Mantid::MantidVec axisBinEdges = binEdgeAxis->getValues();
      double binCentreValue = (axisBinEdges[section] + axisBinEdges[section + 1]) / 2.0;

      if (role == Qt::ToolTipRole)
      {
        return QString("index %1%2%3 %4%5 (bin centre)").arg(QString::number(section), toolTipSeperator,
          QString::fromStdString(axis->unit()->caption()),
          QString::number(binCentreValue), unit);
      }
      else
      {
        return QString("%1%2%3%4").arg(QString::number(section), headerSeperator,
          QString::number(binCentreValue), unit);
      }
    }

    Mantid::API::NumericAxis* numAxis = dynamic_cast<Mantid::API::NumericAxis*>(axis);
    if (numAxis)
    {
      QString valueString;
      try
      {
        valueString = QString::number(numAxis->getValue(section));          
      }
      catch (Mantid::Kernel::Exception::IndexError&)
      {
        valueString="";
      }

      if (role == Qt::ToolTipRole) 
      {
        return QString("index %1%2%3 %4%5").arg(QString::number(section), toolTipSeperator, 
            QString::fromStdString(axis->unit()->caption()),
            valueString, unit);          
      }
      else
      {
        if (headerSeperator == " ") headerSeperator = "   ";
        return QString("%1%2%3%4").arg(QString::number(section), headerSeperator, 
          valueString, unit);
      }
    }

  }
  // fall through value, just return the section value
  return section;


}

Qt::ItemFlags MantidMatrixModel::flags(const QModelIndex & index ) const
{
  // MG: For item selection to work correclty in later Qt versions it must be marked as enabled
  if (index.isValid())
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  else
    return Qt::ItemIsEnabled;
}

/**
@param f :: Number format:  'f' - fixed, 'e' - scientific, 'g' - shorter of the earlier two.
@param prec :: New precision (number of digits after the decimal point) with which the data will
be shown in MantidMatrix.
*/
void MantidMatrixModel::setFormat(const QChar& f,int prec)
{
  QString formats = " efg";
  if ( formats.indexOf(f) > 0 )
  {
    m_format = f.toAscii();
    m_prec = prec;
  }
}

QVariant MantidMatrixModel::data(const QModelIndex &index, int role) const
{  
  switch (role)
  {
  case Qt::DisplayRole:
    {
      double val = data(index.row(),index.column());
      return QVariant(m_locale.toString(val,m_format,m_prec));
    }
  case Qt::BackgroundRole:
    {
      if (checkMonitorCache(index.row()))
      {
        return m_mon_color;
      }
      else
      {
        return QVariant();
      }
    }
  default:
    return QVariant();
  }
}

/**   Checks the row cache to see if the detector flag is stored, then returns it, otherwise it looks it up and adds it to the cache for quick lookup
@param row :: current row in the table that maps to a detector.
@return bool :: the value of if the detector is a monitor or not.
*/
bool MantidMatrixModel::checkMonitorCache(int row) const
{
  row += m_startRow; //correctly offset the row
  if (m_workspace->axes() > 1 && m_workspace->getAxis(1)->isSpectra())
  {
    bool isMon = false;
    if (m_monCache.contains(row))
    {
      isMon = m_monCache.value(row);
    }
    else
    {
      try
      {
        size_t wsIndex = static_cast<size_t>(row);
        IDetector_const_sptr det = m_workspace->getDetector(wsIndex);
        if (det->isMonitor())
        {
          isMon = true;
        }
        else
        {
          isMon = false;
        }
        m_monCache.insert(row, isMon);
      }
      catch (std::exception&)
      {
        m_monCache.insert(row,false);
        isMon = false;
      }
    }
    return isMon;
  }
  else
  {
    return false;
  }
}


void findYRange(MatrixWorkspace_const_sptr ws, double &miny, double &maxy)
{
  //this is here to fill m_min and m_max with numbers that aren't nan
  miny = std::numeric_limits<double>::max();
  maxy = -std::numeric_limits<double>::max();

  if ( ws )
  {

    PARALLEL_FOR1( ws )
      for (int wi=0; wi < static_cast<int>(ws->getNumberHistograms()); wi++)
      {
        double local_min, local_max;
        const Mantid::MantidVec & Y = ws->readY(wi);

        local_min = std::numeric_limits<double>::max();
        local_max = -std::numeric_limits<double>::max();

        for (size_t i=0; i < Y.size(); i++)
        {
          double aux = Y[i];
          if (fabs(aux) == std::numeric_limits<double>::infinity() || aux != aux)
            continue;
          if (aux < local_min)
            local_min = aux;
          if (aux > local_max)
            local_max = aux;
        }

        // Now merge back the local min max
        PARALLEL_CRITICAL(MantidMatrix_range_max)
        {
          if (local_max > maxy)
            maxy = local_max;
        }
        PARALLEL_CRITICAL(MantidMatrix_range_min)
        {
          if (local_min < miny)
            miny = local_min;
        }
      }
  }

  // Make up some reasonable values if nothing was found
  if (miny == std::numeric_limits<double>::max())
    miny = 0;
  if (maxy == -std::numeric_limits<double>::max())
    maxy = miny + 1e6;

  if (maxy == miny)
  {
    if ( maxy == 0.0 ) maxy += 1.0;
    else
      maxy += fabs(miny);
  }
}

void MantidMatrix::loadFromProject(const std::string& lines, ApplicationWindow* app, const int fileVersion)
{
  Q_UNUSED(lines);
  Q_UNUSED(app);
  Q_UNUSED(fileVersion);
  //We don't actually need to do any loading. It's all taken care of by ApplicationWindow.
}

std::string MantidMatrix::saveToProject(ApplicationWindow* app)
{
  TSVSerialiser tsv;

  tsv.writeRaw("<mantidmatrix>");
  tsv.writeLine("WorkspaceName") << m_strName;
  tsv.writeRaw(app->windowGeometryInfo(this));
  tsv.writeRaw("</mantidmatrix>");

  return tsv.outputLines();
}
