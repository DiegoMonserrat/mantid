#include "MantidMDEvents/ReflectometryTranformQxQz.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include <stdexcept>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid
{
  namespace MDEvents
  {

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ReflectometryTranformQxQz::~ReflectometryTranformQxQz()
    {
    }

    /*
    Constructor
    @param qxMin: min qx value (extent)
    @param qxMax: max qx value (extent)
    @param qzMin: min qz value (extent)
    @param qzMax; max qz value (extent)
    @param incidentTheta: Predetermined incident theta value
    */
    ReflectometryTranformQxQz::ReflectometryTranformQxQz(double qxMin, double qxMax, double qzMin, double qzMax, double incidentTheta):
    m_qxMin(qxMin), m_qxMax(qxMax), m_qzMin(qzMin), m_qzMax(qzMax), m_incidentTheta(incidentTheta)
    {
      if(qxMin >= qxMax)
      {
        throw std::invalid_argument("min qx bounds must be < max qx bounds");
      }
      if(qzMin >= qzMax)
      {
        throw std::invalid_argument("min qz bounds must be < max qz bounds");
      }
      if(incidentTheta < 0 || incidentTheta > 90)
      {
        throw std::out_of_range("incident theta angle must be > 0 and < 90");
      }
    }

    /*
    Execute the transformtion. Generates an output IMDEventWorkspace.
    @return the constructed IMDEventWorkspace following the transformation.
    @param ws: Input EventWorkspace const shared pointer
    */
    IMDEventWorkspace_sptr ReflectometryTranformQxQz::execute(IEventWorkspace_const_sptr eventWs) const
    {
      const size_t nbinsx = 10;
      const size_t nbinsz = 10;

      auto ws = boost::make_shared<MDEventWorkspace<MDLeanEvent<2>,2> >();
      MDHistoDimension_sptr qxDim = MDHistoDimension_sptr(new MDHistoDimension("Qx","qx","(Ang^-1)", static_cast<Mantid::coord_t>(m_qxMin), static_cast<Mantid::coord_t>(m_qxMax), nbinsx)); 
      MDHistoDimension_sptr qzDim = MDHistoDimension_sptr(new MDHistoDimension("Qz","qz","(Ang^-1)", static_cast<Mantid::coord_t>(m_qzMin), static_cast<Mantid::coord_t>(m_qzMax), nbinsz)); 

      ws->addDimension(qxDim);
      ws->addDimension(qzDim);

      // Set some reasonable values for the box controller
      BoxController_sptr bc = ws->getBoxController();
      bc->setSplitInto(2);
      bc->setSplitThreshold(10);

      // Initialize the workspace.
      ws->initialize();

      // Start with a MDGridBox.
      ws->splitBox();

      auto spectraAxis = eventWs->getAxis(1);
      const double two_pi = 6.28318531;
      const double c_cos_theta_i = cos(m_incidentTheta);
      const double  c_sin_theta_i = sin(m_incidentTheta);

      for(size_t index = 0; index < eventWs->getNumberHistograms(); ++index)
      {
        auto counts = eventWs->readY(index);
        auto wavelengths = eventWs->readX(index);
        auto errors = eventWs->readE(index);
        size_t nInputBins = eventWs->isHistogramData() ? wavelengths.size() -1 : wavelengths.size();
        const double theta_final = spectraAxis->getValue(index)/2;
        const double c_sin_theta_f = sin(theta_final);
        const double c_cos_theta_f = cos(theta_final);
        const double dirQx = (c_cos_theta_f - c_cos_theta_i);
        const double dirQz = (c_sin_theta_f + c_sin_theta_i);
        //Loop over all bins in spectra 
        for(size_t binIndex = 0; binIndex < nInputBins; ++binIndex)
        {
          double lambda = wavelengths[binIndex];
          double wavenumber = two_pi/lambda;
          double _qx = wavenumber * dirQx;
          double _qz = wavenumber * dirQz;

          double centers[2] = {_qx, _qz};

          ws->addEvent(MDLeanEvent<2>(float(counts[binIndex]), float(errors[binIndex]*errors[binIndex]), centers));
        }
        ws->splitAllIfNeeded(NULL);
      }
      return ws;
    }

  } // namespace Mantid
} // namespace MDEvents