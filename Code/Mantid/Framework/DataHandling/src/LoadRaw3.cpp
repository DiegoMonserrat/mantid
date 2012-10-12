//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FileProperty.h"
#include "LoadRaw/isisraw2.h"
#include "MantidDataHandling/LoadLog.h"
#include "MantidAPI/LoadAlgorithmFactory.h"

#include <boost/shared_ptr.hpp>
#include <Poco/Path.h>
#include <cmath>
#include <cstdio> //Required for gcc 4.4
#include "MantidGeometry/ISpectraDetectorMap.h"

using Mantid::Geometry::ISpectraDetectorMap;

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadRaw3)
    DECLARE_LOADALGORITHM(LoadRaw3)

    /// Sets documentation strings for this algorithm
    void LoadRaw3::initDocs()
    {
      this->setWikiSummary("Loads a data file in ISIS [[RAW_File | RAW]] format and stores it in a 2D [[workspace]] ([[Workspace2D]] class). ");
      this->setOptionalMessage("Loads a data file in ISIS  RAW format and stores it in a 2D workspace (Workspace2D class).");
    }


    using namespace Kernel;
    using namespace API;

    /// Constructor
    LoadRaw3::LoadRaw3() :
      m_filename(), m_noTimeRegimes(0),m_prog(0.0), m_prog_start(0.0), m_prog_end(1.0),
      m_lengthIn(0), m_timeChannelsVec(),m_total_specs(0)
    {
    }

    LoadRaw3::~LoadRaw3()
    {
    }

    /// Initialisation method.
    void LoadRaw3::init()
    {
      LoadRawHelper::init();
      auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
      mustBePositive->setLower(1);
      declareProperty("SpectrumMin", 1, mustBePositive,
          "The index number of the first spectrum to read.  Only used if\n"
          "spectrum_max is set.");
      declareProperty("SpectrumMax", EMPTY_INT(), mustBePositive,
          "The number of the last spectrum to read. Only used if explicitly\n"
          "set.");
      declareProperty(new ArrayProperty<specid_t> ("SpectrumList"),
          "A comma-separated list of individual spectra to read.  Only used if\n"
          "explicitly set.");

      std::vector<std::string> monitorOptions;
      monitorOptions.push_back("Include");
      monitorOptions.push_back("Exclude");
      monitorOptions.push_back("Separate");
      declareProperty("LoadMonitors","Include", boost::make_shared<StringListValidator>(monitorOptions),
          "Use this option to control the loading of monitors.\n"
          "The defalut is Include option  which loads the monitors into the output workspace.\n"
          "Other options are Exclude and Separate.The Exclude option exludes monitors from the output workspace \n"
          "and the Separate option loads monitors into a separate workspace called OutputWorkspace_Monitor.\n");
    }


    /** Executes the algorithm. Reading in the file and creating and populating
     *  the output workspace
     *  @throw Exception::FileError If the RAW file cannot be found/opened
     *  @throw std::invalid_argument If the optional properties are set to invalid values
     */
    void LoadRaw3::exec()
    {
      // Retrieve the filename from the properties
      m_filename = getPropertyValue("Filename");
      //open the raw file
      FILE* file=openRawFile(m_filename);

      bool bLoadlogFiles = getProperty("LoadLogFiles");

      bool bincludeMonitors = true;
      bool bseparateMonitors = false;
      bool bexcludeMonitors = false;
      bincludeMonitors = isIncludeMonitors();
      if (!bincludeMonitors)
      {
        bseparateMonitors = isSeparateMonitors();
        bexcludeMonitors = isExcludeMonitors();
      }

      std::string title;
      //read workspace title from raw file
      readTitle(file,title);

      //read workspace dimensions,mumber of periods etc from the raw file.
      readworkspaceParameters(m_numberOfSpectra,m_numberOfPeriods,m_lengthIn,m_noTimeRegimes);

      setOptionalProperties();
      // to validate the optional parameters, if set
      checkOptionalProperties();

      // Calculate the size of a workspace, given its number of periods & spectra to read
      m_total_specs = calculateWorkspaceSize();

      // If there is not enough memory use ManagedRawFileWorkspace2D.
      if ( ConfigService::Instance().getString("ManagedRawFileWorkspace.DoNotUse") != "1" &&
          m_numberOfPeriods == 1 && m_total_specs == m_numberOfSpectra &&
          MemoryManager::Instance().goForManagedWorkspace(m_total_specs,m_lengthIn, m_lengthIn-1) )
      {
        fclose(file);
        goManagedRaw(bincludeMonitors, bexcludeMonitors, bseparateMonitors,m_filename);
        return;
      }

      // Get the time channel array(s) and store in a vector inside a shared pointer
      m_timeChannelsVec =getTimeChannels(m_noTimeRegimes,m_lengthIn);

      // Create the 2D workspace for the output
      DataObjects::Workspace2D_sptr localWorkspace = createWorkspace(m_total_specs, m_lengthIn,m_lengthIn-1,title);

      // Only run the sub-algorithms once
      loadRunParameters(localWorkspace);
      runLoadMappingTable(m_filename,localWorkspace);
      runLoadInstrument(m_filename,localWorkspace, 0.0, 0.4);
      m_prog_start = 0.4;
      Run& run = localWorkspace->mutableRun();
      if (bLoadlogFiles)
      {
        runLoadLog(m_filename,localWorkspace, 0.4, 0.5);
        m_prog_start = 0.5;
        const int period_number = 1;
        createPeriodLogs(period_number, localWorkspace);
      }
      // Set the total proton charge for this run
      setProtonCharge(run);
      setRunNumber(run);

      // Set progress to start of range taking account of subalgorithms
      setProg(0.0);

      // populate instrument parameters
      g_log.debug("Populating the instrument parameters...");
      progress(m_prog, "Populating the instrument parameters...");
      localWorkspace->populateInstrumentParameters();

      WorkspaceGroup_sptr ws_grp = createGroupWorkspace();
      WorkspaceGroup_sptr monitorws_grp;
      DataObjects::Workspace2D_sptr monitorWorkspace;
      specid_t normalwsSpecs = 0;
      specid_t monitorwsSpecs = 0;
      std::vector<specid_t> monitorSpecList;

      if (bincludeMonitors)
      {
        setWorkspaceProperty("OutputWorkspace", title, ws_grp, localWorkspace,m_numberOfPeriods, false);
      }
      else
      {
        //gets the monitor spectra list from workspace
        getmonitorSpectrumList(localWorkspace, monitorSpecList);
        //calculate the workspace size for normal workspace and monitor workspace
        calculateWorkspacesizes(monitorSpecList,normalwsSpecs, monitorwsSpecs);
        try
        {
          validateWorkspaceSizes(bexcludeMonitors,bseparateMonitors,normalwsSpecs, monitorwsSpecs);
        }
        catch(std::out_of_range&)
        {
          fclose(file);
          throw ;
        }

        //now create a workspace of size normalwsSpecs and set it as outputworkspace
        if(normalwsSpecs > 0)
        {
          localWorkspace = createWorkspace(localWorkspace,normalwsSpecs,m_lengthIn,m_lengthIn-1);
          setWorkspaceProperty("OutputWorkspace", title, ws_grp, localWorkspace,m_numberOfPeriods,false);
        }
        //now create monitor workspace if separateMonitors selected
        if (bseparateMonitors)
        {
          createMonitorWorkspace(monitorWorkspace,localWorkspace,monitorws_grp,monitorwsSpecs,
              normalwsSpecs,m_numberOfPeriods,m_lengthIn,title);
        }
      }

      if(bseparateMonitors && normalwsSpecs == 0)
      {
        // Ensure we fill the correct group as if we are only loading monitors then we essentially want normal behaviour
        // with no extra _Monitors workspace
        ws_grp = monitorws_grp;
      }

      // Loop over the number of periods in the raw file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period)
      {
        if (period > 0)
        {
          if(localWorkspace)
          {
            localWorkspace=createWorkspace(localWorkspace);
          }

          if (bLoadlogFiles)
          {
            //remove previous period data
            std::stringstream prevPeriod;
            prevPeriod << "PERIOD " << (period);
            //std::string prevPeriod="PERIOD "+suffix.str();
            const int period_number = period + 1;
            if(localWorkspace)
            {
              Run& runObj = localWorkspace->mutableRun();
              runObj.removeLogData(prevPeriod.str());
              runObj.removeLogData("current_period");
              //add current period data
              createPeriodLogs(period_number, localWorkspace);
            }
            if(monitorWorkspace)
            {
              Run& runObj = monitorWorkspace->mutableRun();
              runObj.removeLogData(prevPeriod.str());
              runObj.removeLogData("current_period");
              //add current period data
              createPeriodLogs(period_number, monitorWorkspace);
            }
          }//end of if loop for loadlogfiles
          if (bseparateMonitors)
          {
            try
            {
              monitorWorkspace=createWorkspace(monitorWorkspace,monitorwsSpecs,m_lengthIn,m_lengthIn-1);
            }
            catch(std::out_of_range& )
            {
              g_log.information()<<"Separate Monitors option is selected and no monitors in the selected specra range."<<std::endl;
              g_log.information()<<"Error in creating one of the output workspaces"<<std::endl;
            }
            catch(std::runtime_error& )
            {
              g_log.information()<<"Separate Monitors option is selected,Error in creating one of the output workspaces"<<std::endl;
            }
          }//end of separate Monitors
        }
        //skipping the first spectra in each period
        skipData(file, static_cast<int>(period * (m_numberOfSpectra + 1)));

        if (bexcludeMonitors)
        {
          excludeMonitors(file,period,monitorSpecList,localWorkspace);
        }
        if (bincludeMonitors)
        {
          includeMonitors(file,period,localWorkspace);
        }
        if (bseparateMonitors)
        {
          separateMonitors(file,period,monitorSpecList,localWorkspace,monitorWorkspace);
        }

        // Re-update spectra etc.
        if (localWorkspace)
          localWorkspace->updateSpectraUsingMap();

        if (monitorWorkspace)
          monitorWorkspace->updateSpectraUsingMap();

        // Assign the result to the output workspace property
        if (m_numberOfPeriods > 1)
        {
          if (bseparateMonitors)
          {
            if(normalwsSpecs > 0)
            {
              // declare and set monitor workspace for each period
              setWorkspaceProperty(monitorWorkspace, monitorws_grp, period, true);
            }
            else
            {
              localWorkspace = monitorWorkspace;
            }
            // declare and set output workspace for each period
            setWorkspaceProperty(localWorkspace, ws_grp, period, false);
          }
          else
          {
            setWorkspaceProperty(localWorkspace, ws_grp, period, false);
          }
          // progress for workspace groups
          setProg ( static_cast<double>(period) / static_cast<double>(m_numberOfPeriods - 1) );
        }

      } // loop over periods
      // Clean up

      reset();
      fclose(file);
    }
    /** This method creates outputworkspace excluding monitors
     *@param file :: -pointer to file
     *@param period :: period number
     *@param monitorList :: a list conatining the spectrum numbers for monitors
     *@param ws_sptr :: shared pointer to workspace
     */
    void LoadRaw3::excludeMonitors(FILE* file,const int & period,const std::vector<specid_t>& monitorList,
        DataObjects::Workspace2D_sptr ws_sptr)
    {
      int64_t histCurrent = -1;
      int64_t wsIndex=0;
      double histTotal = static_cast<double>(m_total_specs * m_numberOfPeriods);
      //loop through the spectra
      for (specid_t i = 1; i <= m_numberOfSpectra; ++i)
      {
        specid_t histToRead = i + period * (m_numberOfSpectra + 1);
        if ((i >= m_spec_min && i < m_spec_max) || (m_list && find(m_spec_list.begin(), m_spec_list.end(),
            i) != m_spec_list.end()))
        {
          progress(m_prog, "Reading raw file data...");
          //skip monitor spectrum
          if (isMonitor(monitorList, i))
          {
            skipData(file, histToRead);
            continue;
          }

          //read spectrum
          if (!readData(file, histToRead))
          {
            throw std::runtime_error("Error reading raw file");
          }
          //set the workspace data
          setWorkspaceData(ws_sptr, m_timeChannelsVec, wsIndex, i, m_noTimeRegimes,m_lengthIn,1);
          //increment workspace index
          ++wsIndex;

          if (m_numberOfPeriods == 1)
          {
            if (++histCurrent % 100 == 0)
            {
              setProg( static_cast<double>(histCurrent) / histTotal );
            }
            interruption_point();
          }

        }//end of if loop for spec min,max check
        else
        {
          skipData(file, histToRead);
        }
      }//end of for loop
    }

    /**This method creates outputworkspace including monitors
     *@param file :: -pointer to file
     *@param period :: period number
     *@param ws_sptr :: shared pointer to workspace
     */
    void LoadRaw3::includeMonitors(FILE* file,const int64_t& period,DataObjects::Workspace2D_sptr ws_sptr)
    {

      int64_t histCurrent = -1;
      int64_t wsIndex=0;
      double histTotal = static_cast<double>(m_total_specs * m_numberOfPeriods);
      //loop through spectra
      for (specid_t i = 1; i <= m_numberOfSpectra; ++i)
      {
        int64_t histToRead = i + period * (m_numberOfSpectra + 1);
        if ((i >= m_spec_min && i < m_spec_max) || (m_list && find(m_spec_list.begin(), m_spec_list.end(),
            i) != m_spec_list.end()))
        {
          progress(m_prog, "Reading raw file data...");

          //read spectrum from raw file
          if (!readData(file, histToRead))
          {
            throw std::runtime_error("Error reading raw file");
          }
          //set workspace data
          setWorkspaceData(ws_sptr, m_timeChannelsVec, wsIndex, i, m_noTimeRegimes,m_lengthIn,1);
          ++wsIndex;

          if (m_numberOfPeriods == 1)
          {
            if (++histCurrent % 100 == 0)
            {
              setProg( double(histCurrent) / histTotal );
            }
            interruption_point();
          }

        }
        else
        {
          skipData(file, histToRead);
        }
      }
      //loadSpectra(file,period,m_total_specs,ws_sptr,m_timeChannelsVec);
    }

    /** This method separates monitors and creates two outputworkspaces
     *@param file :: -pointer to file
     *@param period :: period number
     *@param monitorList :: -a list conatining the spectrum numbers for monitors
     *@param ws_sptr :: -shared pointer to workspace
     *@param mws_sptr :: -shared pointer to monitor workspace
     */

    void LoadRaw3::separateMonitors(FILE* file,const int64_t& period,const std::vector<specid_t>& monitorList,
        DataObjects::Workspace2D_sptr ws_sptr,DataObjects::Workspace2D_sptr mws_sptr)
    {
      int64_t histCurrent = -1;
      int64_t wsIndex=0;
      int64_t mwsIndex=0;
      double histTotal = static_cast<double>(m_total_specs * m_numberOfPeriods);
      //loop through spectra
      for (specid_t i = 1; i <= m_numberOfSpectra; ++i)
      {
        int64_t histToRead = i + period * (m_numberOfSpectra + 1);
        if ((i >= m_spec_min && i < m_spec_max) || (m_list && find(m_spec_list.begin(), m_spec_list.end(),
            i) != m_spec_list.end()))
        {
          progress(m_prog, "Reading raw file data...");

          //read spectrum from raw file
          if (!readData(file, histToRead))
          {
            throw std::runtime_error("Error reading raw file");
          }
          //if this a moniotor  store that spectrum to monitor workspace
          if (isMonitor(monitorList, i))
          {
            setWorkspaceData(mws_sptr, m_timeChannelsVec, mwsIndex, i, m_noTimeRegimes,m_lengthIn,1);
            ++mwsIndex;
          }
          else
          {
            //not a monitor,store the spectrum to normal output workspace
            setWorkspaceData(ws_sptr, m_timeChannelsVec, wsIndex, i, m_noTimeRegimes,m_lengthIn,1);
            ++wsIndex;
          }

          if (m_numberOfPeriods == 1)
          {
            if (++histCurrent % 100 == 0)
            {
              setProg( static_cast<double>(histCurrent) / histTotal );
            }
            interruption_point();
          }

        }
        else
        {
          skipData(file, histToRead);
        }
      }

    }
    /// This sets the optional property to the LoadRawHelper class
    void LoadRaw3::setOptionalProperties()
    {
      //read in the settings passed to the algorithm
      m_spec_list = getProperty("SpectrumList");
      m_spec_max = getProperty("SpectrumMax");
      m_spec_min = getProperty("SpectrumMin");

    }

    /// This sets the progress taking account of progress time taken up by subalgorithms
    void LoadRaw3::setProg( double prog )
    {
      m_prog = m_prog_start + (m_prog_end - m_prog_start)*prog;
    }


    /**This method validatates workspace sizes if exclude monitors or separate monitors options is selected
     *@param bexcludeMonitors :: boolean option for exclude monitors
     *@param bseparateMonitors :: boolean option for separate monitors
     *@param normalwsSpecs :: number of spectrums in the output workspace excluding monitors
     *@param monitorwsSpecs :: number of monitor spectra
     */
    void LoadRaw3::validateWorkspaceSizes( bool bexcludeMonitors ,bool bseparateMonitors,
        const int64_t normalwsSpecs,const int64_t  monitorwsSpecs)
    {
      if (normalwsSpecs <= 0 && bexcludeMonitors)
      {
        throw std::out_of_range(
            "All the spectra in the selected range for this workspace are monitors and Exclude monitors option selected ");
      }
      if(bseparateMonitors)
      {
        if(normalwsSpecs<=0 &&  monitorwsSpecs<=0)
        {
          throw std::out_of_range(
              "Workspace size is zero,Error in creating output workspace.");

        }
      }

    }
    /** This method checks the value of LoadMonitors property and returns true or false
     *  @return true if Exclude Monitors option is selected,otherwise false
     */
    bool LoadRaw3::isExcludeMonitors()
    {
      bool bExclude;
      std::string monitorOption = getPropertyValue("LoadMonitors");
      monitorOption.compare("Exclude") ? (bExclude = false) : (bExclude = true);
      return bExclude;
    }

    /**This method checks the value of LoadMonitors property and returns true or false
     * @return true if Include Monitors option is selected,otherwise false
     */
    bool LoadRaw3::isIncludeMonitors()
    {
      bool bExclude;
      std::string monitorOption = getPropertyValue("LoadMonitors");
      monitorOption.compare("Include") ? (bExclude = false) : (bExclude = true);
      return bExclude;
    }

    /** This method checks the value of LoadMonitors property and returns true or false
     *  @return true if Separate Monitors option is selected,otherwise false
     */
    bool LoadRaw3::isSeparateMonitors()
    {
      bool bSeparate;
      std::string monitorOption = getPropertyValue("LoadMonitors");
      monitorOption.compare("Separate") ? (bSeparate = false) : (bSeparate = true);
      return bSeparate;
    }

    /** This method checks given spectrum is a monitor
     *  @param monitorIndexes :: a vector holding the list of monitors
     *  @param spectrumNum ::  the requested spectrum number
     *  @return true if it's a monitor
     */
    bool LoadRaw3::isMonitor(const std::vector<specid_t>& monitorIndexes, specid_t spectrumNum)
    {
      bool bMonitor;
      std::vector<specid_t>::const_iterator itr;
      itr = find(monitorIndexes.begin(), monitorIndexes.end(), spectrumNum);
      (itr != monitorIndexes.end()) ? (bMonitor = true) : (bMonitor = false);
      return bMonitor;
    }

    /// Creates a ManagedRawFileWorkspace2D
    /// @param bincludeMonitors :: Include monitors or not
    /// @param bexcludeMonitors :: Exclude monitors or not
    /// @param bseparateMonitors :: Separate monitors or not
    /// @param fileName :: the filename
    void LoadRaw3::goManagedRaw(bool bincludeMonitors, bool bexcludeMonitors, bool bseparateMonitors,
        const std::string& fileName)
    {
      const std::string cache_option = getPropertyValue("Cache");
      bool bLoadlogFiles = getProperty("LoadLogFiles");
      size_t option = find(m_cache_options.begin(), m_cache_options.end(), cache_option)
        - m_cache_options.begin();
      progress(m_prog, "Reading raw file data...");
      DataObjects::Workspace2D_sptr localWorkspace = DataObjects::Workspace2D_sptr(
          new ManagedRawFileWorkspace2D(fileName, static_cast<int>(option)));
      setProg( 0.2 );
      loadRunParameters(localWorkspace);
      runLoadInstrument(fileName,localWorkspace, 0.2, 0.4 );
      setProg( 0.4 );
      runLoadMappingTable(fileName,localWorkspace);
      setProg( 0.5 );
      if (bLoadlogFiles)
      {
        runLoadLog(fileName,localWorkspace, 0.5, 0.7);
        const int current_period = 1;
        createPeriodLogs(current_period,localWorkspace);
      }
      setProtonCharge(localWorkspace->mutableRun());

      setProg( 0.7 );
      progress(m_prog);
      for (int i = 0; i < m_numberOfSpectra; ++i)
      {
        localWorkspace->getAxis(1)->spectraNo(i) = i + 1;
      }
      setProg( 0.9 );
      localWorkspace->populateInstrumentParameters();
      separateOrexcludeMonitors(localWorkspace, bincludeMonitors, bexcludeMonitors,
          bseparateMonitors,m_numberOfSpectra,fileName);
      setProg( 1.0 );
      progress(m_prog);
      setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(localWorkspace));
    }

    /** This method separates/excludes monitors from output workspace and creates a separate workspace for monitors
     *  THIS METHOD IS ONLY CALLED BY THE goManagedRaw METHOD ABOVE AND NOT IN THE GENERAL CASE
     *  @param localWorkspace :: shared pointer to workspace
     *  @param binclude :: boolean  variable for including monitors
     *  @param bexclude ::  boolean variable for excluding monitors
     *  @param bseparate ::  boolean variable for separating the monitor workspace from output workspace
     *  @param m_numberOfSpectra ::  number of spectra
     *  @param fileName ::  raw file name
     */
    void LoadRaw3::separateOrexcludeMonitors(DataObjects::Workspace2D_sptr localWorkspace,
        bool binclude,bool bexclude,bool bseparate,
        int64_t m_numberOfSpectra,const std::string &fileName)
    {
      (void) binclude; // Avoid compiler warning

      std::vector<specid_t> monitorSpecList;
      std::vector<specid_t> monitorwsList;
      DataObjects::Workspace2D_sptr monitorWorkspace;
      FILE *file(NULL);
      getmonitorSpectrumList(localWorkspace, monitorSpecList);
      if (bseparate && !monitorSpecList.empty())
      {
        Property *ws = getProperty("OutputWorkspace");
        std::string localWSName = ws->value();
        std::string monitorWSName = localWSName + "_Monitors";

        declareProperty(new WorkspaceProperty<Workspace> ("MonitorWorkspace", monitorWSName,
            Direction::Output));
        //create monitor workspace
        const int64_t nMons(static_cast<int64_t>(monitorSpecList.size()));
        monitorWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            WorkspaceFactory::Instance().create(localWorkspace,nMons,m_lengthIn,m_lengthIn-1));

        setProperty("MonitorWorkspace", boost::dynamic_pointer_cast<Workspace>(monitorWorkspace));
        file =openRawFile(fileName);
        ioRaw(file,true );
      }
      // Now check whether there is more than one time regime in use
      m_noTimeRegimes =getNumberofTimeRegimes();
      // Get the time channel array(s) and store in a vector inside a shared pointer
      m_timeChannelsVec = getTimeChannels(m_noTimeRegimes,m_lengthIn);
      //read raw file
      if (bseparate && !monitorSpecList.empty())
      {
        readData(file, 0);
      }
      int64_t monitorwsIndex = 0;
      for (specid_t i = 0; i < m_numberOfSpectra; ++i)
      {
        int64_t histToRead = i + 1;
        if (bseparate && !monitorSpecList.empty())
        {
          if (!readData(file, histToRead))
          {
            throw std::runtime_error("Error reading raw file");
          }
        }
        if ((bseparate && !monitorSpecList.empty()) || bexclude)
        {
          if (isMonitor(monitorSpecList, static_cast<specid_t>(i) + 1))
          {
            spec2index_map wsIndexmap;
            SpectraAxis* axis = dynamic_cast<SpectraAxis*>(localWorkspace->getAxis(1));
            axis->getSpectraIndexMap(wsIndexmap);
            spec2index_map::const_iterator wsItr;
            wsItr = wsIndexmap.find(static_cast<specid_t>(i + 1));
            if (wsItr != wsIndexmap.end())
              monitorwsList.push_back(static_cast<specid_t>(wsItr->second));
            if (bseparate)
            {
              monitorWorkspace->getAxis(1)->spectraNo(monitorwsIndex) =
                  static_cast<specid_t>(i + 1);
              setWorkspaceData(monitorWorkspace, m_timeChannelsVec, monitorwsIndex, i + 1, m_noTimeRegimes,m_lengthIn,1);
              ++monitorwsIndex;
            }
          }
        }

      }
      if ((bseparate && !monitorwsList.empty()) || bexclude)
      {
        localWorkspace->setMonitorList(monitorwsList);
        // RJT: Comment this out on removal of method from Workspace2D. This whole method doesn't do
        // the right thing anyway, so this only makes things slightly worse pending a fix.
        //    localWorkspace->sethistogramNumbers(
        //      m_numberOfSpectra - static_cast<int64_t>(monitorwsList.size()));
        if (bseparate)
        {
          fclose(file);
        }
      }
    }

    /**This method does a quick file check by checking the no.of bytes read nread params and header buffer
     *  @param filePath- path of the file including name.
     *  @param nread :: no.of bytes read
     *  @param header :: The first 100 bytes of the file as a union
     *  @return true if the given file is of type which can be loaded by this algorithm
     */
    bool LoadRaw3::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
    {
      return LoadRawHelper::quickFileCheck(filePath,nread,header);

    }
    /**checks the file by opening it and reading few lines
     *  @param filePath :: name of the file inluding its path
     *  @return an integer value how much this algorithm can load the file
     */
    int LoadRaw3::fileCheck(const std::string& filePath)
    {
      return LoadRawHelper::fileCheck(filePath);
    }


  } // namespace DataHandling
} // namespace Mantid
