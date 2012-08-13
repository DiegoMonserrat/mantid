#ifndef MANTID_API_WORKSPACEGROUP_H
#define MANTID_API_WORKSPACEGROUP_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include <Poco/NObserver.h>

namespace Mantid
{

//----------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace API
{

/** Class to hold a set of workspaces.
    The workspace group can be an entry in the AnalysisDataService. 
    Its constituent workspaces should also have individual ADS entries.
    Workspace groups can be used in algorithms in the same way as single workspaces.

    @author Sofia Antony, ISIS, RAL
    @date 12/06/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_API_DLL WorkspaceGroup : public Workspace
{
public:
  /// Default constructor.
  WorkspaceGroup(const bool observeADS = true);
  /// Destructor
  ~WorkspaceGroup();
  /// Return a string ID of the class
  virtual const std::string id() const { return "WorkspaceGroup"; }
  /// The collection itself is considered to take up no space
  virtual size_t getMemorySize() const { return 0; }
  /// Turn ADS observations on/off
  void observeADSNotifications(const bool observeADS);
  /// Adds a workspace to the group.
  void add(const std::string& wsName);
  /// Adds a workspace to the group.
  void addWorkspace(Workspace_sptr workspace);
  /// Does a workspace exist within the group
  bool contains(const std::string & wsName) const;
  /// Returns the names of workspaces that make up this group. Note that this returns a copy as the internal vector can mutate while the vector is being iterated over.
  std::vector<std::string> getNames() const;
  /// Return the number of entries within the group
  int getNumberOfEntries() const { return static_cast<int>(this->size()); }
  /// Return the size of the group, so it is more like a container
  size_t size() const { return m_workspaces.size(); }
  /// Return the ith workspace
  Workspace_sptr getItem(const size_t index) const;
  /// Return the workspace by name
  Workspace_sptr getItem(const std::string wsName) const;
  /// Prints the group to the screen using the logger at debug
  void print() const;
  /// Remove a name from the group
  void remove(const std::string& name);
  /// Remove all names from the group but do not touch the ADS
  void removeAll();
  /// Remove all names from the group and also from the ADS
  void deepRemoveAll();
  /// This method returns true if the group is empty (no member workspace)
  bool isEmpty() const;
  bool areNamesSimilar() const;
  /// Posts a notification informing the ADS observers that group was modified
  void updated() const;
  /// Inidicates that the workspace group can be treated as multiperiod.
  bool isMultiperiod() const;
 
private:
  /// Private, unimplemented copy constructor
  WorkspaceGroup(const WorkspaceGroup& ref);
  /// Private, unimplemented copy assignment operator
  const WorkspaceGroup& operator=(const WorkspaceGroup&);
  /// Callback when a delete notification is received
  void workspaceDeleteHandle(Mantid::API::WorkspacePostDeleteNotification_ptr notice);
  /// Observer for workspace delete notfications
  Poco::NObserver<WorkspaceGroup, Mantid::API::WorkspacePostDeleteNotification> m_deleteObserver;
  /// The list of workspace pointers in the group
  std::vector<Workspace_sptr> m_workspaces;
  /// Flag as to whether the observers have been added to the ADS
  bool m_observingADS;
  /// Static reference to the logger
  static Kernel::Logger& g_log;
};

/// Shared pointer to a workspace group class
typedef boost::shared_ptr<WorkspaceGroup> WorkspaceGroup_sptr;
/// Shared pointer to a workspace group class (const version)
typedef boost::shared_ptr<const WorkspaceGroup> WorkspaceGroup_const_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACEGROUP_H*/
