#ifndef MDEVENTWORKSPACETEST_H
#define MDEVENTWORKSPACETEST_H

#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/Timer.h"
#include "MantidAPI/BoxController.h"
#include "MantidDataObjects/CoordTransformDistance.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDGridBox.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <cxxtest/TestSuite.h>
#include <map>
#include <memory>
#include <vector>
#include <typeinfo>
#include <boost/math/special_functions/fpclassify.hpp>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;


class MDEventWorkspaceTest :    public CxxTest::TestSuite
{
private:
    /// Helper function to return the number of masked bins in a workspace. TODO: move helper into test helpers
  size_t getNumberMasked(Mantid::API::IMDWorkspace_sptr ws)
  {
    Mantid::API::IMDIterator* it = ws->createIterator(NULL);
    size_t numberMasked = 0;
    size_t counter = 0;
    for(;counter < it->getDataSize(); ++counter)
    {
      if(it->getIsMasked())
      {
        ++numberMasked;
      }
      it->next(1); //Doesn't perform skipping on masked, bins, but next() does.
    }
    delete it;
    return numberMasked;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDEventWorkspaceTest *createSuite() { return new MDEventWorkspaceTest(); }
  static void destroySuite( MDEventWorkspaceTest *suite ) { delete suite; }

  void test_constructor()
  {
    MDEventWorkspace<MDLeanEvent<3>, 3> ew3;
    TS_ASSERT_EQUALS( ew3.getNumDims(), 3);
    TS_ASSERT_EQUALS( ew3.getNPoints(), 0);
    TS_ASSERT_EQUALS( ew3.id(), "MDEventWorkspace<MDLeanEvent,3>");
    // Box controller MUST always be present
    TS_ASSERT(ew3.getBoxController() );
    TS_ASSERT(ew3.getBox());
    TS_ASSERT(ew3.getBox()->getBoxController());
    TS_ASSERT_EQUALS(ew3.getBox()->getID(), 0);

    // Now with the MDEvent type
    MDEventWorkspace<MDEvent<3>, 3> ew3b;
    TS_ASSERT_EQUALS( ew3b.id(), "MDEventWorkspace<MDEvent,3>");
  }

  void test_constructor_IMDEventWorkspace()
  {
    IMDEventWorkspace * ew3 = new MDEventWorkspace<MDLeanEvent<3>, 3>();
    TS_ASSERT_EQUALS( ew3->getNumDims(), 3);
    TS_ASSERT_EQUALS( ew3->getNPoints(), 0);
    delete ew3;
  }

  class TestableMDEventWorkspace : public MDEventWorkspace<MDLeanEvent<3>, 3> {
  public:
    TestableMDEventWorkspace(const MDEventWorkspace<MDLeanEvent<3>, 3> &other)
        : MDEventWorkspace<MDLeanEvent<3>, 3>(other) {}
  };

  void test_copy_constructor()
  {
    MDEventWorkspace<MDLeanEvent<3>, 3> ew3;
    
    for (size_t i=0; i<3; i++) {
      ew3.addDimension( MDHistoDimension_sptr(new MDHistoDimension("x","x","m",-1,1,0)) );
    }
    ew3.initialize();
    ew3.getBoxController()->setSplitThreshold(1);
    const coord_t centers[3] = {1.0f, 2.0f, 3.0f};
    ew3.addEvent( MDLeanEvent<3>(1.0, 1.0, centers) );
    ew3.addEvent( MDLeanEvent<3>(2.0, 2.0, centers) );
    ew3.addEvent( MDLeanEvent<3>(3.0, 3.0, centers) );
    ew3.splitBox();

    ExperimentInfo_sptr ei(new ExperimentInfo);
    TS_ASSERT_EQUALS( ew3.addExperimentInfo(ei), 0);

    TestableMDEventWorkspace copy(ew3);
    TS_ASSERT_EQUALS( copy.getNumDims(), 3);
    TS_ASSERT_EQUALS( copy.getDimension(0)->getName(), "x");
    TS_ASSERT_EQUALS( copy.getNumExperimentInfo(), 1);
    TSM_ASSERT_DIFFERS( "ExperimentInfo's were not deep-copied", copy.getExperimentInfo(0), ew3.getExperimentInfo(0));
    TSM_ASSERT_DIFFERS( "BoxController was not deep-copied", copy.getBoxController(), ew3.getBoxController());
    TSM_ASSERT_DIFFERS( "Dimensions were not deep-copied", copy.getDimension(0), ew3.getDimension(0));

    /*Test that the boxes were deep copied and that their BoxController pointers have been updated too.*/
    std::vector<API::IMDNode *> originalBoxes(0, NULL);
    ew3.getBox()->getBoxes(originalBoxes, 10000, false);

    std::vector<API::IMDNode *> copiedBoxes;
    copy.getBox()->getBoxes(copiedBoxes, 10000, false);

    // Quick check.
    TSM_ASSERT_EQUALS("Number of boxes should be the same before and after the copy.", originalBoxes.size(), copiedBoxes.size());
    for(size_t i = 0; i < originalBoxes.size(); ++i)
    {
       API::IMDNode * originalMDBox = originalBoxes[i];
       API::IMDNode * copiedMDBox = copiedBoxes[i];

       auto originalBoxTypeName = std::string(typeid(*originalMDBox).name());
       auto copiedBoxTypeName = std::string(typeid(*copiedMDBox).name());

       // Check the types
       TSM_ASSERT("Box types are not the same", originalBoxTypeName.compare(copiedBoxTypeName)==0); // Comparing them this way will at least produce a useful error if type matching fails.
       TSM_ASSERT_DIFFERS( "BoxController should be different between original and copied boxes", originalMDBox->getBoxController(), copiedMDBox->getBoxController());
       TSM_ASSERT_EQUALS("BoxController on copied box does not match that in copied workspace", copy.getBoxController().get(), copiedMDBox->getBoxController());
    }
  }

  void test_initialize_throws()
  {
    IMDEventWorkspace * ew = new MDEventWorkspace<MDLeanEvent<3>, 3>();
    TS_ASSERT_THROWS( ew->initialize(), std::runtime_error);
    for (size_t i=0; i<5; i++)
          ew->addDimension( MDHistoDimension_sptr(new MDHistoDimension("x","x","m",-1,1,0)) );
        TS_ASSERT_THROWS( ew->initialize(), std::runtime_error);
    delete ew;
  }

  void test_initialize()
  {
    IMDEventWorkspace * ew = new MDEventWorkspace<MDLeanEvent<3>, 3>();
    TS_ASSERT_THROWS( ew->initialize(), std::runtime_error);
    for (size_t i=0; i<3; i++)
      ew->addDimension( MDHistoDimension_sptr(new MDHistoDimension("x","x","m",-1,1,0)) );
    TS_ASSERT_THROWS_NOTHING( ew->initialize() );
    delete ew;
  }


  //-------------------------------------------------------------------------------------
  /** Split the main box into a grid box */
  void test_splitBox()
  {
    MDEventWorkspace3 * ew = new MDEventWorkspace3();
    BoxController_sptr bc = ew->getBoxController();
    bc->setSplitInto(4);
    TS_ASSERT( !ew->isGridBox() );
    TS_ASSERT_THROWS_NOTHING( ew->splitBox(); )
    TS_ASSERT( ew->isGridBox() );
    delete ew;
  }


  //-------------------------------------------------------------------------------------
  /** MDBox->addEvent() tracks when a box is too big.
   * MDEventWorkspace->splitTrackedBoxes() splits them
   * */
  void test_splitTrackedBoxes()
  {
    return; //FIXME with ticket 5102, if/when it is complete
//    MDEventWorkspace1Lean::sptr ew = MDEventsTestHelper::makeMDEW<1>(2, 0.0, 1.0, 0);
//    BoxController_sptr bc = ew->getBoxController();
//    bc->setSplitInto(2);
//    bc->setSplitThreshold(100);
//    ew->splitBox();
//
//    typedef MDGridBox<MDLeanEvent<1>,1> gbox_t;
//    typedef MDBox<MDLeanEvent<1>,1> box_t;
//    typedef MDBoxBase<MDLeanEvent<1>,1> ibox_t;
//
//    // Make 99 events
//    coord_t centers[1] = {0};
//    for (size_t i=0; i<99; i++)
//    {
//      centers[0] = coord_t(i)*0.001;
//      ew->addEvent(MDEvent<1>(1.0, 1.0, centers) );
//    }
//    TS_ASSERT_EQUALS( bc->getBoxesToSplit().size(), 0);
//
//    // The 100th event triggers the adding to the list
//    ew->addEvent(MDEvent<1>(1.0, 1.0, centers) );
//    TS_ASSERT_EQUALS( bc->getBoxesToSplit().size(), 1);
  }

  //-------------------------------------------------------------------------------------
  /** Create an IMDIterator */
  void test_createIterator()
  {
    MDEventWorkspace3 * ew = new MDEventWorkspace3();
    BoxController_sptr bc = ew->getBoxController();
    bc->setSplitInto(4);
    ew->splitBox();
    IMDIterator * it = ew->createIterator();
    TS_ASSERT(it);
    TS_ASSERT_EQUALS(it->getDataSize(), 4*4*4);
    TS_ASSERT(it->next());
    delete it;
    auto *mdfunction = new MDImplicitFunction();
    it = ew->createIterator(mdfunction);
    TS_ASSERT(it);
    TS_ASSERT_EQUALS(it->getDataSize(), 4*4*4);
    TS_ASSERT(it->next());
    delete mdfunction;
    delete it;
    delete ew;
  }

  //-------------------------------------------------------------------------------------
  /** Create several IMDIterators to run them in parallel */
  void test_createIterators()
  {
    MDEventWorkspace3 * ew = new MDEventWorkspace3();
    BoxController_sptr bc = ew->getBoxController();
    bc->setSplitInto(4);
    ew->splitBox();
    std::vector<IMDIterator *> iterators = ew->createIterators(3);
    TS_ASSERT_EQUALS(iterators.size(), 3);

    TS_ASSERT_EQUALS(iterators[0]->getDataSize(), 21);
    TS_ASSERT_EQUALS(iterators[1]->getDataSize(), 21);
    TS_ASSERT_EQUALS(iterators[2]->getDataSize(), 22);

    for(size_t i = 0; i < 3; ++i) delete iterators[i];
    delete ew;
  }

  //-------------------------------------------------------------------------------------
  /** Method that makes a table workspace for use in MantidPlot */
  void test_makeBoxTable()
  {
    MDEventWorkspace3Lean::sptr ew = MDEventsTestHelper::makeMDEW<3>(4, 0.0, 4.0, 1);
    ITableWorkspace_sptr itab = ew->makeBoxTable(0,0);
    TS_ASSERT_EQUALS( itab->rowCount(), 4*4*4+1);
    TS_ASSERT_EQUALS( itab->cell<int>(3,0), 3);
  }


  //-------------------------------------------------------------------------------------
  /** Get the signal at a given coord */
  void test_getSignalAtCoord()
  {
    MDEventWorkspace3Lean::sptr ew = MDEventsTestHelper::makeMDEW<3>(4, 0.0, 4.0, 1);
    coord_t coords1[3] = {1.5,1.5,1.5};
    coord_t coords2[3] = {2.5,2.5,2.5};
    coord_t coords3[3] = {-0.1f, 2, 2};
    coord_t coords4[3] = {2, 2, 4.1f};
    ew->addEvent(MDLeanEvent<3>(2.0, 2.0, coords2));
    ew->refreshCache();
    TSM_ASSERT_DELTA("A regular box with a single event", ew->getSignalAtCoord(coords1, Mantid::API::NoNormalization), 1.0, 1e-5);
    TSM_ASSERT_DELTA("The box with 2 events", ew->getSignalAtCoord(coords2, Mantid::API::NoNormalization), 3.0, 1e-5);
    TSM_ASSERT("Out of bounds returns NAN", boost::math::isnan( ew->getSignalAtCoord(coords3, Mantid::API::NoNormalization) ) );
    TSM_ASSERT("Out of bounds returns NAN", boost::math::isnan( ew->getSignalAtCoord(coords4, Mantid::API::NoNormalization) ) );
  }


  //-------------------------------------------------------------------------------------
  void test_estimateResolution()
  {
    MDEventWorkspace2Lean::sptr b = MDEventsTestHelper::makeMDEW<2>(10, 0.0, 10.0);
    std::vector<coord_t> binSizes;
    // First, before any splitting
    binSizes = b->estimateResolution();
    TS_ASSERT_EQUALS( binSizes.size(), 2);
    TS_ASSERT_DELTA( binSizes[0], 10.0, 1e-6);
    TS_ASSERT_DELTA( binSizes[1], 10.0, 1e-6);

    // Resolution is smaller after splitting
    b->splitBox();
    binSizes = b->estimateResolution();
    TS_ASSERT_EQUALS( binSizes.size(), 2);
    TS_ASSERT_DELTA( binSizes[0], 1.0, 1e-6);
    TS_ASSERT_DELTA( binSizes[1], 1.0, 1e-6);
  }

  ////-------------------------------------------------------------------------------------

  void checkExtents( std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > & ext, coord_t xmin, coord_t xmax, coord_t ymin, coord_t ymax)
  {
    TS_ASSERT_DELTA( ext[0].getMin(), xmin, 1e-4);
    TS_ASSERT_DELTA( ext[0].getMax(), xmax, 1e-4);
    TS_ASSERT_DELTA( ext[1].getMin(), ymin, 1e-4);
    TS_ASSERT_DELTA( ext[1].getMax(), ymax, 1e-4);
  }

  void addEvent(MDEventWorkspace2Lean::sptr b, double x, double y)
  {
    coord_t centers[2] = {static_cast<coord_t>(x), static_cast<coord_t>(y)};
    b->addEvent(MDLeanEvent<2>(2.0, 2.0, centers));
  }

  void test_getMinimumExtents()
  {
    MDEventWorkspace2Lean::sptr ws = MDEventsTestHelper::makeMDEW<2>(10, 0.0, 10.0);
    

    // If nothing in the workspace, the extents given are the dimensions in the workspace
    auto ext = ws->getMinimumExtents(2);
    TS_ASSERT_DELTA( ext[0].getMin(), 0.0, 1e-5 );
    TS_ASSERT_DELTA( ext[0].getMax(), 10.0, 1e-5 );
    TS_ASSERT_DELTA( ext[1].getMin(), 0.0, 1e-5 );
    TS_ASSERT_DELTA( ext[1].getMax(), 10.0, 1e-5 );

    std::vector< MDLeanEvent<2> > events;
    // Make an event in the middle of each box
    for (double x=4.0005; x < 7; x += 1.0)
      for (double y=4.0005; y < 7; y += 1.0)
      {
        double centers[2] = {x, y};
        events.push_back( MDLeanEvent<2>(2.0, 2.0, centers) );
      }
    // So it doesn't split
    ws->getBoxController()->setSplitThreshold(1000);
    // but split once to get grid box in the centre
    ws->splitBox();
    //ws->addManyEvents( events, NULL );
    ws->addEvents(events);
    ws->refreshCache();

    // Base extents
    ext = ws->getMinimumExtents(2);
    checkExtents(ext, 4, 7,  4, 7);

    // Start adding events to make the extents bigger
    addEvent(ws, 3.5, 5.0);
    ext = ws->getMinimumExtents(2);
    checkExtents(ext, 3, 7,  4, 7);

    addEvent(ws, 8.5, 7.9);
    ext = ws->getMinimumExtents(2);
    checkExtents(ext, 3, 9,  4, 8);

    addEvent(ws, 0.5, 0.9);
    ext = ws->getMinimumExtents(2);
    checkExtents(ext, 0, 9,  0, 8);

  }



  void test_integrateSphere()
  {
    // 10x10x10 eventWorkspace
    MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1 /*event per box*/);
    TS_ASSERT_EQUALS( ws->getNPoints(), 1000);

    // The sphere transformation
    coord_t center[3] = {0,0,0};
    bool dimensionsUsed[3] = {true,true,true};
    CoordTransformDistance sphere(3, center, dimensionsUsed);

    signal_t signal = 0;
    signal_t errorSquared = 0;
    ws->getBox()->integrateSphere(sphere, 1.0, signal, errorSquared);

    //TODO:
//    TS_ASSERT_DELTA( signal, 1.0, 1e-5);
//    TS_ASSERT_DELTA( errorSquared, 1.0, 1e-5);
  }

  /*
  Generic masking checking helper method.
  */
  void doTestMasking(MDImplicitFunction* function, size_t expectedNumberMasked)
  {
    // 10x10x10 eventWorkspace
    MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1 /*event per box*/);

    ws->setMDMasking(function);

    size_t numberMasked = getNumberMasked(ws);
    TSM_ASSERT_EQUALS("Didn't perform the masking as expected", expectedNumberMasked, numberMasked);
  }
  
  void test_maskEverything()
  {
    std::vector<coord_t> min;
    std::vector<coord_t> max;

    min.push_back(0);
    min.push_back(0);
    min.push_back(0);
    max.push_back(10);
    max.push_back(10);
    max.push_back(10);

    //Create an function that encompases 1/4 of the total bins.
    MDImplicitFunction* function = new MDBoxImplicitFunction(min, max);

    doTestMasking(function, 1000); //1000 out of 1000 bins masked
  }

  void test_maskNULL()
  {
    //Should do nothing in terms of masking, but should not throw.
    doTestMasking(NULL, 0); //0 out of 1000 bins masked
  }

  void test_maskNothing()
  {
    std::vector<coord_t> min;
    std::vector<coord_t> max;

    //Make the box lay over a non-intersecting region of space.
    min.push_back(-1);
    min.push_back(-1);
    min.push_back(-1);
    max.push_back(-0.01f);
    max.push_back(-0.01f);
    max.push_back(-0.01f);

    //Create an function that encompases 1/4 of the total bins.
    MDImplicitFunction* function = new MDBoxImplicitFunction(min, max);

    doTestMasking(function, 0); //0 out of 1000 bins masked
  }

  void test_maskHalf()
  {
    std::vector<coord_t> min;
    std::vector<coord_t> max;

    //Make the box that covers half the bins in the workspace. 
    min.push_back(0);
    min.push_back(0);
    min.push_back(0);
    max.push_back(10);
    max.push_back(10);
    max.push_back(4.99f);

    //Create an function that encompases 1/4 of the total bins.
    MDImplicitFunction* function = new MDBoxImplicitFunction(min, max);

    doTestMasking(function, 500); //500 out of 1000 bins masked
  }

  void test_clearMasking()
  {
    //Create a function that masks everything.
    std::vector<coord_t> min;
    std::vector<coord_t> max;
    min.push_back(0);
    min.push_back(0);
    min.push_back(0);
    max.push_back(10);
    max.push_back(10);
    max.push_back(10);
    MDImplicitFunction* function = new MDBoxImplicitFunction(min, max);

    MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1 /*event per box*/);
    ws->setMDMasking(function);

    TSM_ASSERT_EQUALS("Everything should be masked.", 1000, getNumberMasked(ws));
    TS_ASSERT_THROWS_NOTHING(ws->clearMDMasking());
    TSM_ASSERT_EQUALS("Nothing should be masked.", 0, getNumberMasked(ws));
  }

  void test_getSpecialCoordinateSystem_default()
  {
    MDEventWorkspace1Lean::sptr ws = MDEventsTestHelper::makeMDEW<1>(10, 0.0, 10.0, 1 /*event per box*/);
    TSM_ASSERT_EQUALS("Should default to no special coordinate system.", Mantid::Kernel::None, ws->getSpecialCoordinateSystem());
  }

  void test_setSpecialCoordinateSystem_default()
  {
     MDEventWorkspace1Lean::sptr ws = MDEventsTestHelper::makeMDEW<1>(10, 0.0, 10.0, 1 /*event per box*/);
    TS_ASSERT_EQUALS(Mantid::Kernel::None, ws->getSpecialCoordinateSystem());

    ws->setCoordinateSystem(Mantid::Kernel::QLab);
    TS_ASSERT_EQUALS(Mantid::Kernel::QLab, ws->getSpecialCoordinateSystem());
  }

};



class MDEventWorkspaceTestPerformance :    public CxxTest::TestSuite
{

public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDEventWorkspaceTestPerformance *createSuite() { return new MDEventWorkspaceTestPerformance(); }
  static void destroySuite( MDEventWorkspaceTestPerformance *suite ) { delete suite; }

  MDEventWorkspaceTestPerformance()
  { 
  }


private:
  MDEventWorkspace3Lean::sptr m_ws;
  size_t nBoxes;
public: 
    void setUp()
    {
      size_t dim_size = 20;
      size_t sq_dim_size = dim_size*dim_size;
      m_ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, (Mantid::coord_t)dim_size, 10 /*event per box*/);
      m_ws->getBoxController()->setSplitThreshold(10);
      nBoxes = dim_size*dim_size*dim_size;
      std::vector<MDLeanEvent<3> > vecEvents(nBoxes);
    
      for(size_t i = 0; i < dim_size; ++i)
      {
        for(size_t j = 0; j < dim_size; ++j)
        {
          for(size_t k = 0; k < dim_size; ++k)
          {
            double centers[3] = {(double)i, (double)j, (double)k};
            vecEvents[i + j*dim_size + k*sq_dim_size] = MDLeanEvent<3>(1, 1, centers);
          }
        }
      }
      m_ws->addEvents(vecEvents);
    }

    void teadDown()
    {
      m_ws.reset();
    }
   void test_splitting_performance_single_threaded()
  {
    std::cout<<"Starting Workspace splitting performance test, single threaded with "<<nBoxes <<" events \n";
    Kernel::Timer clock;
    m_ws->splitAllIfNeeded(NULL);
    std::cout<<"Finished Workspace splitting performance test, single threaded in "<< clock.elapsed()<<" sec\n";
  }

  void test_splitting_performance_parallel()
  {
    auto ts_splitter = new ThreadSchedulerFIFO();
    ThreadPool tp_splitter(ts_splitter,4);
    std::cout<<"Starting Workspace splitting performance test, 4 thread with "<<nBoxes <<" events \n";
    Kernel::Timer clock;
    m_ws->splitAllIfNeeded(ts_splitter);
    tp_splitter.joinAll();
    std::cout<<"Finished Workspace splitting performance test, 4 threads in "<< clock.elapsed()<<" sec\n";
  }
};

#endif
