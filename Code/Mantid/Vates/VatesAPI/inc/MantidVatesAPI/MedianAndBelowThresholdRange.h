#ifndef MANTID_PARAVIEW_MEDIAN_AND_BELOW_THRESHOLD_RANGE
#define MANTID_PARAVIEW_MEDIAN_AND_BELOW_THRESHOLD_RANGE

#include "MantidKernel/System.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidAPI/IMDWorkspace.h"

/** Set range selection to cut-out zeros and provide an upper limit equal to the median value in the workspace.

 @author Owen Arnold, Tessella plc
 @date 07/07/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

namespace Mantid
{
namespace VATES
{
class DLLExport MedianAndBelowThresholdRange : public ThresholdRange
{

public:

  MedianAndBelowThresholdRange();

  virtual void calculate();

  virtual signal_t getMinimum() const;

  virtual signal_t getMaximum() const;

  ~MedianAndBelowThresholdRange();

  virtual bool hasCalculated() const;

  virtual MedianAndBelowThresholdRange* clone() const;

  virtual bool inRange(const signal_t& signal);

  virtual void setWorkspace(Mantid::API::Workspace_sptr workspace);

private:
  
  MedianAndBelowThresholdRange(signal_t min, signal_t max, bool isCalculated, Mantid::API::IMDWorkspace_sptr m_workspace);

  signal_t m_min;
  
  signal_t m_max;

  bool m_isCalculated;

  Mantid::API::IMDWorkspace_sptr m_workspace;
};
}
}

#endif