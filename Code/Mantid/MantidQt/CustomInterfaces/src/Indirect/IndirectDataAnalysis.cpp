//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/Indirect/IndirectDataAnalysis.h"

// IndirectDataAnalysisTab subclasses:
#include "MantidQtCustomInterfaces/Indirect/Elwin.h"
#include "MantidQtCustomInterfaces/Indirect/MSDFit.h"
#include "MantidQtCustomInterfaces/Indirect/Iqt.h"
#include "MantidQtCustomInterfaces/Indirect/IqtFit.h"
#include "MantidQtCustomInterfaces/Indirect/ConvFit.h"
#include "MantidQtCustomInterfaces/Indirect/CalcCorr.h"
#include "MantidQtCustomInterfaces/Indirect/ApplyCorr.h"
#include "MantidQtCustomInterfaces/Indirect/AbsorptionCorrections.h"

#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtAPI/ManageUserDirectories.h"

#include "MantidAPI/AnalysisDataService.h"

#include <QDesktopServices>
#include <QUrl>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  // Add this class to the list of specialised dialogs in this namespace
  DECLARE_SUBWINDOW(IndirectDataAnalysis)

  /**
   * Constructor.
   *
   * @param parent :: the parent QWidget.
   */
  IndirectDataAnalysis::IndirectDataAnalysis(QWidget *parent) :
    UserSubWindow(parent),
    m_valInt(NULL), m_valDbl(NULL),
    m_changeObserver(*this, &IndirectDataAnalysis::handleDirectoryChange)
  {
    m_uiForm.setupUi(this);

    // Allows us to get a handle on a tab using an enum, for example "m_tabs[ELWIN]".
    // All tabs MUST appear here to be shown in interface.
    // We make the assumption that each map key corresponds to the order in which the tabs appear.
    m_tabs.insert(std::make_pair(ELWIN,      new Elwin(m_uiForm.twIDATabs->widget(ELWIN))));
    m_tabs.insert(std::make_pair(MSD_FIT,    new MSDFit(m_uiForm.twIDATabs->widget(MSD_FIT))));
    m_tabs.insert(std::make_pair(IQT,        new Iqt(m_uiForm.twIDATabs->widget(IQT))));
    m_tabs.insert(std::make_pair(IQT_FIT,    new IqtFit(m_uiForm.twIDATabs->widget(IQT_FIT))));
    m_tabs.insert(std::make_pair(CONV_FIT,   new ConvFit(m_uiForm.twIDATabs->widget(CONV_FIT))));
  }

  /**
   * @param :: the detected close event
   */
  void IndirectDataAnalysis::closeEvent(QCloseEvent*)
  {
    Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
  }

  /**
   * Handles a change in directory.
   *
   * @param pNf :: notification
   */
  void IndirectDataAnalysis::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf)
  {
    std::string key = pNf->key();

    if ( key == "defaultsave.directory" )
      loadSettings();
  }

  /**
   * Initialised the layout of the interface.  MUST be called.
   */
  void IndirectDataAnalysis::initLayout()
  {
    // Connect Poco Notification Observer
    Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

    // Set up all tabs
    for(auto tab = m_tabs.begin(); tab != m_tabs.end(); ++tab)
    {
      tab->second->setupTab();
      connect(tab->second, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
      connect(tab->second, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
    }

    connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this, SLOT(exportTabPython()));
    connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
    connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(run()));
    connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(openDirectoryDialog()));
  }

  /**
   * Allow Python to be called locally.
   */
  void IndirectDataAnalysis::initLocalPython()
  {
    QString pyInput = "from mantid.simpleapi import *";
    QString pyOutput = runPythonCode(pyInput).trimmed();
    loadSettings();
  }

  /**
   * Load the settings saved for this interface.
   */
  void IndirectDataAnalysis::loadSettings()
  {
    QSettings settings;
    QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
    QString saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

    settings.beginGroup(settingsGroup + "ProcessedFiles");
    settings.setValue("last_directory", saveDir);

    // Load each tab's settings.
    auto tab = m_tabs.begin();
    for( ; tab != m_tabs.end(); ++tab )
      tab->second->loadTabSettings(settings);

    settings.endGroup();
  }

  /**
   * Private slot, called when the Run button is pressed.  Runs current tab.
   */
  void IndirectDataAnalysis::run()
  {
    const unsigned int currentTab = m_uiForm.twIDATabs->currentIndex();
    m_tabs[currentTab]->runTab();
  }

  /**
   * Opens a directory dialog.
   */
  void IndirectDataAnalysis::openDirectoryDialog()
  {
    MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
    ad->show();
    ad->setFocus();
  }

  /**
   * Opens the Mantid Wiki web page of the current tab.
   */
  void IndirectDataAnalysis::help()
  {
    MantidQt::API::HelpWindow::showCustomInterface(NULL, QString("Indirect_DataAnalysis"));
  }

  /**
   * Handles exporting a Python script for the current tab.
   */
  void IndirectDataAnalysis::exportTabPython()
  {
    unsigned int currentTab = m_uiForm.twIDATabs->currentIndex();
    m_tabs[currentTab]->exportPythonScript();
  }

  /**
   * Slot to wrap the protected showInformationBox method defined
   * in UserSubWindow and provide access to composed tabs.
   *
   * @param message The message to display in the message box
   */
  void IndirectDataAnalysis::showMessageBox(const QString& message)
  {
    showInformationBox(message);
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
