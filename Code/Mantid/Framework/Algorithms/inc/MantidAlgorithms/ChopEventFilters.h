#ifndef MANTID_ALGORITHMS_CHOPEVENTFILTERS_H_
#define MANTID_ALGORITHMS_CHOPEVENTFILTERS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
    namespace Algorithms
    {
        
        /** 
         * Add a peak to a PeaksWorkspace.
         
         @date 2012-10-16
         
         Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
         
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
        class DLLExport ChopEventFilters  : public API::Algorithm
        {
        public:
            ChopEventFilters();
            ~ChopEventFilters();
            
            /// Algorithm's name for identification 
            virtual const std::string name() const { return "ChopEventFilters";};
            /// Algorithm's version for identification 
            virtual int version() const { return 1;};
            /// Algorithm's category for identification
            virtual const std::string category() const { return "Events\\EventFiltering";}
            
        private:
            /// Sets documentation strings for this algorithm
            virtual void initDocs();
            /// Initialise the properties
            void init();
            /// Run the algorithm
            void exec();

            void processAlgProperties();
            void chopEventFilterByTime();
            
            
            /// Input (source) time splitters workspace in MatrixWorkspace format
            API::MatrixWorkspace_const_sptr m_inputWS;
            
            /// Output time splitters workspace in MatrixWorkspace format
            API::MatrixWorkspace_sptr m_outputWS;
            
            /// Number of total time slots for chopping
            int m_numSlots;
            
            /// Index of the time slot for output
            int m_slotIndex;
            
            /// Target worskpace group to be chopped
            int m_wsGroup;
           
            std::vector<double> m_outX;
            std::vector<double> m_outY;
            
        };
        
        
    } // namespace Mantid
} // namespace Algorithms


#endif  /* MANTID_ALGORITHMS_CHOPEVENTFILTERS_H_ */
