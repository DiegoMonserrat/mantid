#ifndef DATAHANDING_LOADBBY_H_
#define DATAHANDING_LOADBBY_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"

namespace Mantid
{
namespace DataHandling
{
/**
     Loads a Bilby data file. Implements API::IFileLoader and its file check methods to
     recognise a file as the one containing QUOKKA data.

     @author David Mannicke (ANSTO), Roman Tolchenov (Tessella plc)
     @date 07/02/2014

     Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport LoadBBY : public  API::IFileLoader<Kernel::FileDescriptor>
{
public:
  /// (Empty) Constructor
  LoadBBY() {}
  /// Virtual destructor
  virtual ~LoadBBY() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LoadBBY"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::FileDescriptor & descriptor) const;

  ///Summary of algorithms purpose
  virtual const std::string summary() const {return "Loads a BilBy data file into an workspace.";}

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();
};
}
}
#endif //DATAHANDING_LOADBBY_H_