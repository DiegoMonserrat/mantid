#ifndef BOXCONTROLLER_NEXUS_IO_TEST_H
#define BOXCONTROLLER_NEXUS_IO_TEST_H

#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>
#include <Poco/File.h>
#include <nexus/NeXusFile.hpp>
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidMDEvents/BoxControllerNxSIO.h"
#include "MantidAPI/FileFinder.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::API;
//using namespace Mantid::MDEvens;

class BoxControllerSaveableTest : public CxxTest::TestSuite
{
    BoxController_sptr sc;

    BoxControllerSaveableTest()
    {
        sc = BoxController_sptr(new BoxController(3));
    }



  /** Deletes the file created by do_saveNexus */
  static void do_deleteNexusFile(std::string barefilename = "MDBoxTest.nxs")
  {
    std::string filename = (ConfigService::Instance().getString("defaultsave.directory") + barefilename);
    if (Poco::File(filename).exists())  Poco::File(filename).remove();
  }

public:
static BoxControllerSaveableTest *createSuite() { return new BoxControllerSaveableTest(); }
static void destroySuite(BoxControllerSaveableTest * suite) { delete suite; }    



 void test_contstructor_setters()
 {
     TS_ASSERT_THROWS(MDEvents::BoxControllerNxSIO Saver(11),std::invalid_argument);

     MDEvents::BoxControllerNxSIO *pSaver;
     TS_ASSERT_THROWS_NOTHING(pSaver=new MDEvents::BoxControllerNxSIO(4));

     size_t CoordSize;
     std::string typeName;
     TS_ASSERT_THROWS_NOTHING(pSaver->getDataType(CoordSize, typeName));
     // default settings
     TS_ASSERT_EQUALS(4,CoordSize);
     TS_ASSERT_EQUALS("MDEvent",typeName);

     //set size
    TS_ASSERT_THROWS(pSaver->setDataType(9,typeName),std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(pSaver->setDataType(8, typeName));
    TS_ASSERT_THROWS_NOTHING(pSaver->getDataType(CoordSize, typeName));
    TS_ASSERT_EQUALS(8,CoordSize);
    TS_ASSERT_EQUALS("MDEvent",typeName);

     //set type
    TS_ASSERT_THROWS(pSaver->setDataType(4,"UnknownEvent"),std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(pSaver->setDataType(4, "MDLeanEvent"));
    TS_ASSERT_THROWS_NOTHING(pSaver->getDataType(CoordSize, typeName));
    TS_ASSERT_EQUALS(4,CoordSize);
    TS_ASSERT_EQUALS("MDLeanEvent",typeName);


     delete pSaver;
 }

 void testCreateOrOpenFile()
 {
     MDEvents::BoxControllerNxSIO *pSaver;
     TS_ASSERT_THROWS_NOTHING(pSaver=new MDEvents::BoxControllerNxSIO(4));

     std::string testFile = "testFile.nxs";
     std::string FullPathFile = API::FileFinder::Instance().getFullPath(testFile);
     if(!FullPathFile.empty())
            Poco::File(FullPathFile).remove();   

     TSM_ASSERT_THROWS("new file does not open in read mode",pSaver->openFile(testFile,"r"), Kernel::Exception::FileError);

     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(testFile,"w"));
     TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());
     TS_ASSERT(pSaver->isOpened());
     TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
     TS_ASSERT(!pSaver->isOpened());

     TSM_ASSERT("file created ",!API::FileFinder::Instance().getFullPath(FullPathFile).empty());

     // now I can open this file for reading 
     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(FullPathFile,"r"));
     TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());
     TS_ASSERT(pSaver->isOpened());
     TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
     TS_ASSERT(!pSaver->isOpened());

     // now I can open this file for writing
     TS_ASSERT_THROWS_NOTHING(pSaver->openFile(FullPathFile,"W"));
     TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());
     TS_ASSERT(pSaver->isOpened());
     TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
     TS_ASSERT(!pSaver->isOpened());

     delete pSaver;
     if(Poco::File(FullPathFile).exists())
         Poco::File(FullPathFile).remove();   
 }
//
//
//  //-----------------------------------------------------------------------------------------
//  /** Can we save to a file ? */
//  void xest_saveNexus()
//  {
//    std::string filename = do_saveNexus();
//    TS_ASSERT(Poco::File(filename.c_str()).exists());
//    if (Poco::File(filename.c_str()).exists()) Poco::File(filename).remove();
//  }
//
//
//
//
//    //-----------------------------------------------------------------------------------------
//  /** xest the methods related to the file back-end */
//  void xest_fileBackEnd_related()
//  {
//    // Box with 100 events
//    MDBox<MDLeanEvent<2>,2> b;
//    MDEventsxestHelper::feedMDBox(&b, 1, 10, 0.5, 1.0);
//    TS_ASSERT_EQUALS( b.getNPoints(), 100);
//    b.refreshCache();
//    TS_ASSERT_DELTA( b.getSignal(), 100., 0.001);
//    TS_ASSERT_DELTA( b.getErrorSquared(), 100., 0.001);
//
//    // Because it wasn't set, the # of points on disk is 0, so NPoints = data.size() + 0
//    TS_ASSERT_EQUALS( b.getNPoints(), 100);
//    b.setFilePosition(1234, 100);
//    // Now it returns the cached number of points + the number in the data
//    TS_ASSERT_EQUALS( b.getNPoints(), 200);
//    // Still returns the signal/error
//    TS_ASSERT_DELTA( b.getSignal(), 100., 0.001);
//    TS_ASSERT_DELTA( b.getErrorSquared(), 100., 0.001);
//  }
//
//  //-----------------------------------------------------------------------------------------
//  /** Create a xest .NXS file with some data for a MDBox<3>
//   * 1000 events starting at position 500 of the file are made.
//   *
//   * @param goofyWeights :: weights increasing from 0 to 999
//   * @param barefilename :: file to save to (no path)
//   * @return filename with full path that was saved.
//   * */
//  static std::string do_saveNexus(bool goofyWeights = true, std::string barefilename = "MDBoxTest.nxs")
//  {
//    // Box with 1000 events evenly spread
//    MDBox<MDLeanEvent<3>,3> b;
//    MDEventsTestHelper::feedMDBox(&b, 1, 10, 0.5, 1.0);
//    TS_ASSERT_EQUALS( b.getNPoints(), 1000);
//    if (goofyWeights)
//    {
//      // Give them goofy weights to be more interesting
//      for (size_t i=0; i<1000; i++)
//      {
//        b.getEvents()[i].setSignal(float(i));
//        b.getEvents()[i].setErrorSquared(float(i)+float(0.5));
//      }
//    }
//
//    // Start a NXS file
//    std::string filename = (ConfigService::Instance().getString("defaultsave.directory") + barefilename);
//    if (Poco::File(filename.c_str()).exists()) 
//    {
//        Poco::File(filename.c_str()).remove();
//    }
//    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_CREATE5);
//    file->makeGroup("my_test_group", "NXdata", 1);
//
//    // Must prepare the data.
//    MDLeanEvent<3>::prepareNexusData(file, 2000);
//    //b.getBoxController()->setFile(file,filename,2000);
//
//    // Save it with some offset
//    b.setFilePosition(500, 1000);
//    b.saveNexus(file);
//
//    file->closeData();
//    file->closeGroup();
//    file->close();
//
//    return filename;
//  }
//
//
//  //-----------------------------------------------------------------------------------------
//  /** Create a test .NXS file with some data for a MDBox<3>.
//   * 1000 events starting at position 500 of the file are made.
//   * Each event is spread evenly around a 10x10x10 region from 0.5 to 9.5 in each direction
//   * Then the file is open appropriately and returned.
//   *
//   * @param goofyWeights :: weights increasing from 0 to 999
//   * @param barefilename :: file to save to (no path)
//   * @param box :: MDBox3 that will get set to be file-backed
//   * @return ptr to the NeXus file object
//   * */
//  static ::NeXus::File * do_saveAndOpenNexus(MDBox<MDLeanEvent<3>,3> & box,
//      std::string barefilename = "MDBoxTest.nxs", bool goofyWeights = true)
//  {
//    // Create the NXS file
//    std::string filename = do_saveNexus(goofyWeights, barefilename);
//    // Open the NXS file
//    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_RDWR);
//    file->openGroup("my_test_group", "NXdata");
//    // Must get ready to load in the data
//    API::BoxController::openEventNexusData(file);
//
//    // Set it in the BoxController
//    if (box.getBoxController())
//      box.getBoxController()->setFile(file,filename, 2000);
//
//    // Make the box know where it is in the file
//    box.setFilePosition(500, 1000);
//    // This would be set on loading. Only makes sense with GoofyWeights == false
//    box.setSignal(1000.0);
//    box.setErrorSquared(1000.0);
//
//    return file;
//  }
//
//
//  //-----------------------------------------------------------------------------------------
//  /** Can we load it back? */
//  void xest_loadNexus()
//  {
//    // A box to load stuff from
//    MDBox<MDLeanEvent<3>,3> c;
//    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);
//
//    // Create and open the xest NXS file
//    ::NeXus::File * file = do_saveAndOpenNexus(c);
//
//    //c.loadNexus(file);
//    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
//    // still on disk
//    TS_ASSERT_EQUALS( c.getFileSize(), 1000);
//    TS_ASSERT_EQUALS( c.getDataMemorySize(), 0);
//    const std::vector<MDLeanEvent<3> > & events = c.getEvents();
//    TSM_ASSERT("Got events so data should be busy unill events are released ",c.isBusy());
//
//    // Try a couple of events to see if they are correct
//    TS_ASSERT_DELTA( events[0].getErrorSquared(), 0.5, 1e-5);
//    TS_ASSERT_DELTA( events[50].getSignal(), 50.0, 1e-5);
//    TS_ASSERT_DELTA( events[990].getErrorSquared(), 990.5, 1e-5);
//
//    file->close();
//    do_deleteNexusFile();
//  }
//
//
//  //-----------------------------------------------------------------------------------------
//  /** What if the box has no events, does it crash? */
//  void xest_loadNexus_noEvents()
//  {
//    // A box to load stuff from
//    MDBox<MDLeanEvent<3>,3> c;
//    TS_ASSERT_EQUALS( c.getNPoints(), 0);
//
//    // Create and open the test NXS file
//    ::NeXus::File * file = do_saveAndOpenNexus(c);
//    // Tell it we actually have no events
//    c.setFilePosition(500, 0);
//    c.loadNexus(file);
//    TS_ASSERT_EQUALS( c.getNPoints(), 0);
//
//    file->close();
//    do_deleteNexusFile();
//  }
//
//  //-----------------------------------------------------------------------------------------
//  /** Set up the file back end and xest accessing data */
//  void xest_fileBackEnd()
//  {
//    // Create a box with a controller for the back-end
//    BoxController_sptr bc(new BoxController(3));
//
//    // Handle the disk DiskBuffer values
//    bc->setCacheParameters(sizeof(MDLeanEvent<3>), 10000);
//    DiskBuffer & dbuf = bc->getDiskBuffer();
//    // It is empty now
//    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);
//
//    // Create and open the test NXS file
//    MDBox<MDLeanEvent<3>,3> c(bc, 0);
//    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);
//    ::NeXus::File * file = do_saveAndOpenNexus(c);
//
//    // Set the stuff that is handled outside the box itself
//    c.setSignal(1234.5); // fake value loaded from disk
//    c.setErrorSquared(456.78);
//
//    // Now it gives the cached value
//    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
//    TS_ASSERT_DELTA( c.getSignal(), 1234.5, 1e-5);
//    TS_ASSERT_DELTA( c.getErrorSquared(), 456.78, 1e-5);
//    TSM_ASSERT("Data is not flagged as busy", !c.isBusy());
//    TSM_ASSERT("System expects that data were saved ",c.wasSaved());
//
//    // This should actually load the events from the file
//    const std::vector<MDLeanEvent<3> > & events = c.getConstEvents();
//    TSM_ASSERT("Data accessed and flagged as modified", c.isBusy());
//    // Try a couple of events to see if they are correct
//    TS_ASSERT_EQUALS( events.size(), 1000);
//    if (events.size() != 1000) return;
//    TS_ASSERT_DELTA( events[0].getErrorSquared(), 0.5, 1e-5);
//    TS_ASSERT_DELTA( events[50].getSignal(), 50.0, 1e-5);
//    TS_ASSERT_DELTA( events[990].getErrorSquared(), 990.5, 1e-5);
//
//    // The box's data is busy
//    TS_ASSERT( c.isBusy() );
//    // Done with the data.
//    c.releaseEvents();
//    TS_ASSERT( !c.isBusy() );
//    // Something in the to-write buffer
//    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 1000);
//
//     // Now this actually does it
//    c.refreshCache();
//    // The real values are back
//    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
//    TS_ASSERT_DELTA( c.getSignal(), 499500.0, 1e-2);
//    TS_ASSERT_DELTA( c.getErrorSquared(), 500000.0, 1e-2);
//
//     // This should NOT call the write method since we had const access. Hard to test though!
//    dbuf.flushCache();
//    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);
//
//    file->close();
//    do_deleteNexusFile();
//  }
//
//
//  //-----------------------------------------------------------------------------------------
//  /** Set up the file back end and test accessing data.
//   * This time, use no DiskBuffer so that reading/loading is done within the object itself */
//  void xest_fileBackEnd_noMRU()
//  {
//    // Create a box with a controller for the back-end
//    BoxController_sptr bc(new BoxController(3));
//
//    // Handle the disk DiskBuffer values
//    bc->setCacheParameters(sizeof(MDLeanEvent<3>), 0);
//    // DiskBuffer won't be used
////    TS_ASSERT( !bc->useWriteBuffer());
//    DiskBuffer & dbuf = bc->getDiskBuffer();
//    // It is empty now
//    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);
//
//    // Create and open the test NXS file
//    MDBox<MDLeanEvent<3>,3> c(bc, 0);
//    TSM_ASSERT_EQUALS( "Box starts empty", c.getNPoints(), 0);
//    ::NeXus::File * file = do_saveAndOpenNexus(c);
//
//    // Set the stuff that is handled outside the box itself
//    c.setSignal(1234.5); // fake value loaded from disk
//    c.setErrorSquared(456.78);
//
//    // Now it gives the cached value
//    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
//    TS_ASSERT_DELTA( c.getSignal(), 1234.5, 1e-5);
//    TS_ASSERT_DELTA( c.getErrorSquared(), 456.78, 1e-5);
//    TSM_ASSERT("Data is not flagged as modified", !c.isDataChanged());
//
//    // This should actually load the events from the file
//    const std::vector<MDLeanEvent<3> > & events = c.getConstEvents();
//    TSM_ASSERT("Data is STILL not flagged as modified", !c.isDataChanged());
//    // Try a couple of events to see if they are correct
//    TS_ASSERT_EQUALS( events.size(), 1000);
//    if (events.size() != 1000) return;
//    TS_ASSERT_DELTA( events[0].getErrorSquared(), 0.5, 1e-5);
//    TS_ASSERT_DELTA( events[50].getSignal(), 50.0, 1e-5);
//    TS_ASSERT_DELTA( events[990].getErrorSquared(), 990.5, 1e-5);
//
//  //  TSM_ASSERT_EQUALS( "DiskBuffer has nothing still - it wasn't used",  dbuf.getWriteBufferUsed(), 0);
//    TSM_ASSERT_EQUALS( "DiskBuffer has this object inside",  dbuf.getWriteBufferUsed(), 1000);
//    TSM_ASSERT("Data is busy", c.isBusy() );
//    TSM_ASSERT("Data is in memory", c.getInMemory() );
//    // Done with the data.
//    c.releaseEvents();
//    TSM_ASSERT("Data is no longer busy", !c.isBusy() );
//    TSM_ASSERT("Data stillin memory", c.getInMemory() );
//    dbuf.flushCache();
//    TSM_ASSERT("Data is not in memory", !c.getInMemory() );
//    TSM_ASSERT_EQUALS( "DiskBuffer has nothing still - it wasn't used",  dbuf.getWriteBufferUsed(), 0);
//
//    file->close();
//    do_deleteNexusFile();
//  }
//
//
//  //-----------------------------------------------------------------------------------------
//  /** Set up the file back end and test accessing data
//   * in a non-const way, and writing it back out*/
//  void xest_fileBackEnd_nonConst_access()
//  {
//    // Create a box with a controller for the back-end
//    BoxController_sptr bc(new BoxController(3));
//
//    // Handle the disk DiskBuffer values
//    bc->setCacheParameters(sizeof(MDLeanEvent<3>), 10000);
//    DiskBuffer & dbuf = bc->getDiskBuffer();
//    // It is empty now
//    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);
//
//    // A new empty box.
//    MDBox<MDLeanEvent<3>,3> c(bc, 0);
//
//    // Create and open the test NXS file
//    ::NeXus::File * file = do_saveAndOpenNexus(c);
//
//    // The # of points (from the file, not in memory)
//    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
//    TSM_ASSERT("Data is not flagged as modified", !c.isDataChanged());
//
//    // Non-const access to the events.
//    std::vector<MDLeanEvent<3> > & events = c.getEvents();
//    TSM_ASSERT("Data is flagged as modified", c.isDataChanged());
//    TS_ASSERT_EQUALS( events.size(), 1000);
//    if (events.size() != 1000) return;
//    TS_ASSERT_DELTA( events[123].getSignal(), 123.0, 1e-5);
//
//    // Modify the event
//    events[123].setSignal(456.0);
//
//    // Done with the events
//    c.releaseEvents();
//
//    // Flushing the cache will write out the events.
//    dbuf.flushCache();
//
//    // Now let's pretend we re-load that data into another box
//    MDBox<MDLeanEvent<3>,3> c2(bc, 0);
//    c2.setFilePosition(500, 1000);
// 
//
//    // Is that event modified?
//    std::vector<MDLeanEvent<3> > & events2 = c2.getEvents();
//    TS_ASSERT_EQUALS( events2.size(), 1000);
//    if (events2.size() != 1000) return;
//    TS_ASSERT_DELTA( events2[123].getSignal(), 456.0, 1e-5);
//
//    file->close();
//    do_deleteNexusFile();
//  }
//
//  
//
//
//  //-----------------------------------------------------------------------------------------
//  /** Set up the file back end and xest accessing data
//   * where the number of events in the box is reduced or increased. */
//  void xest_fileBackEnd_nonConst_EventListChangesSize()
//  {
//    // Create a box with a controller for the back-end
//    BoxController_sptr bc(new BoxController(3));
//
//    // Handle the disk DiskBuffer values
//    bc->setCacheParameters(sizeof(MDLeanEvent<3>), 10000);
//    DiskBuffer & dbuf = bc->getDiskBuffer();
//    // It is empty now
//    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);
//
//    // A new empty box.
//    MDBox<MDLeanEvent<3>,3> c(bc, 0);
//
//    // Create and open the test NXS file
//    ::NeXus::File * file = do_saveAndOpenNexus(c);
//
//    // The # of points (from the file, not in memory)
//    TS_ASSERT_EQUALS( c.getNPoints(), 1000);
//    TSM_ASSERT("Data is not flagged as modified", !c.isDataChanged());
//
//    // Non-const access to the events.
//    std::vector<MDLeanEvent<3> > & events = c.getEvents();
//    TSM_ASSERT("Data is flagged as modified", c.isDataChanged());
//    TS_ASSERT_EQUALS( events.size(), 1000);
//    if (events.size() != 1000) return;
//    TS_ASSERT_DELTA( events[123].getSignal(), 123.0, 1e-5);
//
//    // Modify an event
//    events[123].setSignal(456.0);
//    // Also change the size of the event list
//    events.resize(600);
//
//    // Done with the events
//    c.releaseEvents();
//
//    // Flushing the cache will write out the events.
//    dbuf.flushCache();
//
//    // The size on disk should have been changed (but not the position since that was the only free spot)
//    TS_ASSERT_EQUALS( c.getFilePosition(), 500);
//    TS_ASSERT_EQUALS( c.getTotalDataSize(), 600);
//    TS_ASSERT_EQUALS( c.getDataMemorySize(), 0);
//    TS_ASSERT_EQUALS( c.getNPoints(), 600);
//
//    // Now let's pretend we re-load that data into another box
//    MDBox<MDLeanEvent<3>,3> c2(bc, 0);
//    c2.setFilePosition(500, 600);
//    // Is that event modified?
//    std::vector<MDLeanEvent<3> > & events2 = c2.getEvents();
//    TS_ASSERT_EQUALS( events2.size(), 600);
//    if (events2.size() != 600) return;
//    TS_ASSERT_DELTA( events2[123].getSignal(), 456.0, 1e-5);
//
//    // Now we GROW the event list
//    events2.resize(1500);
//    events2[1499].setSignal(789.0);
//    // And we finish and write it out
//    c2.releaseEvents();
//    dbuf.flushCache();
//    // The new event list should have ended up at the end of the file
//    TS_ASSERT_EQUALS( c2.getFilePosition(), 2000);
//    TS_ASSERT_EQUALS( c2.getDataMemorySize(), 0);
//    TS_ASSERT_EQUALS( c2.getTotalDataSize(), 1500);
//    // The file has now grown.
//    TS_ASSERT_EQUALS( dbuf.getFileLength(), 3500);
//
//    // This counts the number of events actually in the file.
//    TS_ASSERT_EQUALS( file->getInfo().dims[0], 3500);
//
//    // Now let's pretend we re-load that data into a 3rd box
//    MDBox<MDLeanEvent<3>,3> c3(bc, 0);
//    c3.setFilePosition(2000, 1500);
//    // Is that event modified?
//    const std::vector<MDLeanEvent<3> > & events3 = c3.getEvents();
//    TS_ASSERT_EQUALS( events3.size(), 1500);
//    TS_ASSERT_DELTA( events3[1499].getSignal(), 789.0, 1e-5);
//    c3.releaseEvents();
//
//    file->closeData();
//    file->close();
//    do_deleteNexusFile();
//  }
//
//
//
//  //-----------------------------------------------------------------------------------------
//  /** If a MDBox is file-backed, test that
//   * you can add events to it without having to load the data from disk.
//   */
//  void xest_fileBackEnd_addEvent()
//  {
//    // Create a box with a controller for the back-end
//    BoxController_sptr bc(new BoxController(3));
//    bc->setCacheParameters(sizeof(MDLeanEvent<3>),10000);
//    DiskBuffer & dbuf = bc->getDiskBuffer();
//
//    // Create and open the test NXS file
//    MDBox<MDLeanEvent<3>,3> c(bc, 0);
//    ::NeXus::File * file = do_saveAndOpenNexus(c, "MDBoxTest.nxs", false);
//    TSM_ASSERT_EQUALS("Nothing in memory", c.getDataMemorySize(), 0);
//    TSM_ASSERT_EQUALS("Nothing in memory", c.getTotalDataSize(), 1000);
//    TSM_ASSERT_EQUALS("1000 events on file", c.getFileSize(), 1000);
//    TSM_ASSERT("The data was NOT loaded from disk.", !c.getInMemory());
//    TSM_ASSERT_DELTA("Correct cached signal", c.getSignal(), 1000.0, 1e-3);
//    TSM_ASSERT("Data is not flagged as modified", !c.isDataChanged());
//
//
//    // Add an event to it
//    MDLeanEvent<3> ev(1.2, 3.4);
//    ev.setCenter(0, 1.5);
//    ev.setCenter(1, 2.5);
//    ev.setCenter(2, 3.5);
//    c.addEvent(ev);
//
//    TSM_ASSERT_EQUALS("Still 1000 events on file", c.getFileSize(), 1000);
//    TSM_ASSERT_EQUALS("But now 1001 events total because they are in two places.", c.getNPoints(), 1001);
//    TSM_ASSERT_EQUALS("But only one in memory", c.getDataMemorySize(), 1);
//    TSM_ASSERT_EQUALS("The object size -- number of points in it", c.getTotalDataSize(), 1001);
//    TSM_ASSERT("The data is STILL NOT loaded from disk.", !c.getInMemory());
//    TSM_ASSERT_DELTA("At this point the cached signal is still incorrect - this is normal", c.getSignal(), 1000.0, 1e-3);
//
//    // Get the const vector of events AFTER adding events
//    const std::vector<MDLeanEvent<3> > & events = c.getConstEvents();
//    TSM_ASSERT("The data is ALL in memory right now.", c.getInMemory());
//    TSM_ASSERT("Data is not flagged as modified (const access)", !c.isDataChanged());
//    TSM_ASSERT_EQUALS("The resulting event vector has concatenated both", events.size(), 1001);
//    TSM_ASSERT_DELTA("The first event is the one that was manually added.", events[0].getSignal(), 1.2, 1e-4);
//    c.releaseEvents();
//
//    // Flush the cache to write out the modified data
//    dbuf.flushCache();
//    TSM_ASSERT("Data is not flagged as modified because it was written out to disk.", !c.isDataChanged());
//    TSM_ASSERT_EQUALS("Now there is nothing in memory", c.getDataMemorySize(), 0);
//    TSM_ASSERT_EQUALS("There is 1001 ppoint in total", c.getTotalDataSize(), 1001);
//    TSM_ASSERT_EQUALS("Now there is 1001 event in file", c.getFileSize(), 1001);
//    TSM_ASSERT_EQUALS("And the block must have been moved since it grew", c.getFilePosition(), 2000);
//    TSM_ASSERT("And the data is no longer in memory.", !c.getInMemory());
//    TSM_ASSERT("And the data is on disk.", c.wasSaved());
//    TSM_ASSERT_EQUALS("And the number of points is still accurate.", c.getNPoints(), 1001);
//    TSM_ASSERT_DELTA("The cached signal was updated", c.getSignal(), 1001.2, 1e-3);
//
//    TSM_ASSERT_EQUALS("The size of the file's field matches the last available point", file->getInfo().dims[0], 3001);
//
//    {
//    // Now getEvents in a const way then call addEvent()
//    const std::vector<MDLeanEvent<3> > & events2 = c.getConstEvents();
//    TSM_ASSERT("Data is not flagged as modified because it was accessed as const", !c.isDataChanged());
//    (void) events2;
//    c.addEvent(ev);
//
//    TSM_ASSERT("Data is still not flagged as modified because it was accessed as const", !c.isDataChanged());
//    TSM_ASSERT_EQUALS("Still 1001 events on file", c.getFileSize(), 1001);
//    TSM_ASSERT_EQUALS("And  1002 events in memory ", c.getTotalDataSize(), 1002);
//    TSM_ASSERT_EQUALS("But the number of points had grown.", c.getNPoints(), 1002);
//    c.releaseEvents();
//    dbuf.flushCache();
//    TSM_ASSERT("Data is not flagged as modified because it was written out to disk.", !c.isDataChanged());
//    TSM_ASSERT_EQUALS("Now there are 1002 events on file", c.getFileSize(), 1002);
//    TSM_ASSERT_EQUALS("And the block must have been moved since it grew", c.getFilePosition(), 3001);
//    TSM_ASSERT("And the data is no longer in memory.", !c.getInMemory());
//    TSM_ASSERT_EQUALS("And the number of points is still accurate.", c.getNPoints(), 1002);
//    TSM_ASSERT_DELTA("The cached signal was updated", c.getSignal(), 1002.4, 1e-3);
//    }
//
//    {
//    // Now getEvents in a non-const way then call addEvent()
//    std::vector<MDLeanEvent<3> > & events3 = c.getEvents();
//    (void) events3;
//    c.addEvent(ev);
//    TSM_ASSERT_EQUALS("Still 1002 events on file", c.getFileSize(), 1002);
//    TSM_ASSERT_EQUALS("And 1003 events in memory", c.getTotalDataSize(), 1003);
//    TSM_ASSERT_EQUALS("But the number of points had grown.", c.getNPoints(), 1003);
//    c.releaseEvents();
//    dbuf.flushCache();
//    TSM_ASSERT_EQUALS("Nothing in memory", c.getDataMemorySize(), 0);
//    TSM_ASSERT_EQUALS("1003 events in total", c.getTotalDataSize(), 1003);
//    TSM_ASSERT_EQUALS("1003 events on file", c.getFileSize(), 1003);
//    TSM_ASSERT_EQUALS("And the block must have been moved since it grew", c.getFilePosition(), 2000);
//    TSM_ASSERT("And the data is no longer in memory.", !c.getInMemory());
//    TSM_ASSERT_EQUALS("And the number of points is still accurate.", c.getNPoints(), 1003);
//    TSM_ASSERT_DELTA("The cached signal was updated", c.getSignal(), 1003.6, 1e-3);
//    }
//
//    {
//    // changes have been saved
//    std::vector<MDLeanEvent<3> > & events4 = c.getEvents();
//    TSM_ASSERT("Data  flagged as modified", c.isDataChanged());
//    TSM_ASSERT_DELTA("This was on file",events4[234].getSignal(),1.,1.e-6);
//    events4[234].setSignal(234.);
//    c.releaseEvents();
//    dbuf.flushCache();
//    TSM_ASSERT_EQUALS("Nothing in memory", c.getDataMemorySize(), 0);
//    TSM_ASSERT_EQUALS("All gone ",events4.size(),0);
//    TSM_ASSERT_EQUALS("1003 events on the file", c.getFileSize(), 1003);
//    TSM_ASSERT_EQUALS("The file have not changed ", c.getFilePosition(), 2000);
//    TSM_ASSERT("And the data is no longer in memory.", !c.getInMemory());
//    const std::vector<MDLeanEvent<3> > & events5 = c.getConstEvents();
//    TSM_ASSERT_DELTA("The changes have been stored ",events5[234].getSignal(),234.,1.e-6);
//    }
//
// // changes have been lost
//    {
//    const std::vector<MDLeanEvent<3> > & events6 = c.getConstEvents();
//    TSM_ASSERT("Data  flagged as unmodifiable ", !c.isDataChanged());
//    TSM_ASSERT_DELTA("This was on file",events6[234].getSignal(),234.,1.e-6);
//    // now do nasty thing and modify the signal
//    std::vector<MDLeanEvent<3> > & events6m = const_cast<std::vector<MDLeanEvent<3> > &>(events6);
//    events6m[234].setSignal(0.);
//    c.releaseEvents();
//    dbuf.flushCache();
//    // changes lost; checks that constEvents are not saved back on HDD 
//    const std::vector<MDLeanEvent<3> > & events7 = c.getConstEvents();
//    TSM_ASSERT("Data  flagged as unmodifiable ", !c.isDataChanged());
//    TSM_ASSERT_DELTA("This was on file",events7[234].getSignal(),234.,1.e-6);
//    }
//    // changes forced: save of data to HDD is controlled by isDataChanged parameter
//    {
//    const std::vector<MDLeanEvent<3> > & events6 = c.getConstEvents();
//    TSM_ASSERT("Data  flagged as unmodifiable ", !c.isDataChanged());
//    c.setDataChanged();
//    TSM_ASSERT("Data  flagged as modifiable ", c.isDataChanged());
//    TSM_ASSERT_DELTA("This was on file",events6[234].getSignal(),234.,1.e-6);
//    // now do nasty thing and modify the signal
//    std::vector<MDLeanEvent<3> > & events6m = const_cast<std::vector<MDLeanEvent<3> > &>(events6);
//    events6m[234].setSignal(0.);
//
//    c.releaseEvents();
//    dbuf.flushCache();
//    // changes now saved 
//    const std::vector<MDLeanEvent<3> > & events7 = c.getConstEvents();
//    TSM_ASSERT("Data  flagged as unmodifiable ", !c.isDataChanged());
//    TSM_ASSERT_DELTA("This was on file",events7[234].getSignal(),0.,1.e-6);
//    }
//
//
//    file->close();
//    do_deleteNexusFile();
//  }
//
//
//  //-----------------------------------------------------------------------------------------
//  /** Set up the file back end and test accessing data
//   * by binning and stuff */
//  void do_test_fileBackEnd_binningOperations(bool parallel)
//  {
//    // Create a box with a controller for the back-end
//    BoxController_sptr bc(new BoxController(3));
//    MDBox<MDLeanEvent<3>,3> c(bc, 0);
//
//    // Create and open the test NXS file
//    ::NeXus::File * file = do_saveAndOpenNexus(c, "MDBoxBinningTest.nxs", false);
//
//    PARALLEL_FOR_IF(parallel)
//    for (int i=0; i<20; i++)
//    {
//      //std::cout << "Bin try " << i << "\n";
//      // Try a bin, 2x2x2 so 8 events should be in there
//      MDBin<MDLeanEvent<3>,3> bin;
//      for (size_t d=0; d<3; d++)
//      {
//        bin.m_min[d] = 2.0;
//        bin.m_max[d] = 4.0;
//        bin.m_signal = 0;
//      }
//      c.centerpointBin(bin, NULL);
//      TS_ASSERT_DELTA( bin.m_signal, 8.0, 1e-4);
//      TS_ASSERT_DELTA( bin.m_errorSquared, 8.0, 1e-4);
//    }
//
//    PARALLEL_FOR_IF(parallel)
//    for (int i=0; i<20; i++)
//    {
//      //std::cout << "Sphere try " << i << "\n";
//      // Integrate a sphere in the middle
//      bool dimensionsUsed[3] = {true,true,true};
//      coord_t center[3] = {5,5,5};
//      CoordTransformDistance sphere(3, center, dimensionsUsed);
//
//      signal_t signal = 0;
//      signal_t error = 0;
//      c.integrateSphere(sphere, 1.0, signal, error);
//      TS_ASSERT_DELTA( signal, 8.0, 1e-4);
//      TS_ASSERT_DELTA( error, 8.0, 1e-4);
//    }
//
//    file->close();
//    do_deleteNexusFile("MDBoxBinningxest.nxs");
//  }
//
//  void xest_fileBackEnd_binningOperations()
//  {
//    do_test_fileBackEnd_binningOperations(false);
//  }
//
//  void xest_fileBackEnd_binningOperations_inParallel()
//  {
//    do_test_fileBackEnd_binningOperations(true);
//  }
//
//
//  //-----------------------------------------------------------------------------------------
//  /** Test splitting of a MDBox into a MDGridBox when the
//   * original box is backed by a file. */
//  void xest_fileBackEnd_construction()
//  {
//    // Create a box with a controller for the back-end
//    BoxController_sptr bc(new BoxController(3));
//    bc->setSplitInto(5);
//    // Handle the disk MRU values
//    bc->setCacheParameters(sizeof(MDLeanEvent<3>), 10000);
//    DiskBuffer & dbuf = bc->getDiskBuffer();
//    // Make a box from 0-10 in 3D
//    MDBox<MDLeanEvent<3>,3> * c = new MDBox<MDLeanEvent<3>,3>(bc, 0);
//    for (size_t d=0; d<3; d++) c->setExtents(d, 0, 10);
//
//    // Create and open the test NXS file
//    ::NeXus::File * file = MDBoxTest::do_saveAndOpenNexus(*c, "MDGridBoxTest.nxs");
//    TSM_ASSERT_EQUALS( "1000 events (on file)", c->getNPoints(), 1000);
//
//    // At this point the MDBox is set to be on disk
//    TSM_ASSERT_EQUALS( "No free blocks to start with", dbuf.getFreeSpaceMap().size(), 0);
//
//    // Construct the grid box by splitting the MDBox
//    MDGridBox<MDLeanEvent<3>,3> * gb = new MDGridBox<MDLeanEvent<3>,3>(c);
//    TSM_ASSERT_EQUALS( "Grid box also has 1000 points", gb->getNPoints(), 1000);
//    TSM_ASSERT_EQUALS( "Grid box has 125 children (5x5x5)", gb->getNumChildren(), 125);
//    TSM_ASSERT_EQUALS( "The old spot in the file is now free", dbuf.getFreeSpaceMap().size(), 1);
//
//    // Get a child
//    MDBox<MDLeanEvent<3>,3> * b = dynamic_cast<MDBox<MDLeanEvent<3>,3> *>(gb->getChild(22));
//    TSM_ASSERT_EQUALS( "Child has 8 events", b->getNPoints(), 8);
//    TSM_ASSERT_EQUALS( "Child is NOT on disk", b->wasSaved(), false);
//
//    file->close();
//    MDBoxTest::do_deleteNexusFile("MDGridBoxTest.nxs");
//  }
//
//  //------------------------------------------------------------------------------------------------
//  /** This test splits a large number of events,
//   * for a workspace that is backed by a file (and thus tries to stay below
//   * a certain amount of memory used).
//   */
//  void xest_splitAllIfNeeded_fileBacked()
//  {
//    typedef MDLeanEvent<2> MDE;
//    typedef MDGridBox<MDE,2> gbox_t;
//    typedef MDBox<MDE,2> box_t;
//    typedef MDBoxBase<MDE,2> ibox_t;
//
//    // Make a fake file-backing for the grid box
//    std::string filename = "MDGridBoxTest.nxs";
//    ::NeXus::File * file = new ::NeXus::File(filename, NXACC_CREATE);
//    file->makeGroup("MDEventWorkspaceTest", "NXentry", 1);
//    MDE::prepareNexusData(file, 2000);
//    file->close();
//    file = new ::NeXus::File(filename, NXACC_RDWR);
//    file->openGroup("MDEventWorkspaceTest", "NXentry");
//    API::BoxController::openEventNexusData(file);
//
//    // Create the grid box and make it file-backed.
//    gbox_t * b = MDEventsTestHelper::makeMDGridBox<2>();
//    BoxController_sptr bc = b->getBoxController();
//    bc->setSplitThreshold(100);
//    bc->setMaxDepth(4);
//    bc->setCacheParameters(1, 1000);
//    bc->setFile(file, filename, 0);
//    DiskBuffer & dbuf = bc->getDiskBuffer();
//    dbuf.setFileLength(0);
//
//    size_t num_repeat = 10;
//    if (DODEBUG) num_repeat = 20;
//    Timer tim;
//    if (DODEBUG) std::cout << "Adding " << num_repeat*10000 << " events...\n";
//    MDEventsTestHelper::feedMDBox<2>(b, num_repeat, 100, 0.05f, 0.1f);
//    if (DODEBUG) std::cout << "Adding events done in " << tim.elapsed() << "!\n";
//
//    // Split those boxes in parallel.
//    ThreadSchedulerFIFO * ts = new ThreadSchedulerFIFO();
//    ThreadPool tp(ts);
//    b->splitAllIfNeeded(ts);
//    tp.joinAll();
//
//    if (DODEBUG) std::cout << "Splitting events done in " << tim.elapsed() << " sec.\n";
//
//    // Get all the MDBoxes created
//    std::vector<ibox_t*> boxes;
//    b->getBoxes(boxes, 1000, true);
//    TS_ASSERT_EQUALS(boxes.size(), 10000);
//    size_t numOnDisk = 0;
//    uint64_t eventsOnDisk = 0;
//    uint64_t maxFilePos = 0;
//    for (size_t i=0; i<boxes.size(); i++)
//    {
//      ibox_t * box = boxes[i];
//      TS_ASSERT_EQUALS( box->getNPoints(), num_repeat );
//      box_t * mdbox = dynamic_cast<box_t *>(box);
//      TS_ASSERT( mdbox);
//      if ( mdbox->wasSaved() ) numOnDisk++;
//      eventsOnDisk += mdbox->getFileSize();
//      // Track the last point used in the file
//      uint64_t fileEnd = mdbox->getFilePosition() + mdbox->getFileSize();
//      if (fileEnd > maxFilePos) maxFilePos = fileEnd;
//      //std::cout << mdbox->getFilePosition() << " file pos " << i << std::endl;
//    }
//    TSM_ASSERT_EQUALS("All new boxes were set to be cached to disk.", numOnDisk, 10000);
//    uint64_t minimumSaved = 10000*(num_repeat-2);
//    TSM_ASSERT_LESS_THAN("Length of the file makes sense", minimumSaved, dbuf.getFileLength());
//    TSM_ASSERT_LESS_THAN("Most of the boxes' events were cached to disk (some remain in memory because of the MRU cache)", minimumSaved, eventsOnDisk);
//    TSM_ASSERT_LESS_THAN("And the events were properly saved sequentially in the files.", minimumSaved, maxFilePos);
//    std::cout << dbuf.getMemoryStr() << std::endl;
//    file->close();
//    if (Poco::File(filename).exists()) Poco::File(filename).remove();
//  }
//



};
#endif