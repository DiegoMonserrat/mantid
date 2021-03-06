#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidPythonInterface/kernel/Policies/RemoveConst.h"

#include <boost/python/class.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::TimeSeriesProperty;
using Mantid::Kernel::FilteredTimeSeriesProperty;
using Mantid::PythonInterface::Policies::RemoveConst;
using namespace boost::python;

namespace {
/// Macro to reduce copy-and-paste
#define EXPORT_FILTEREDTIMESERIES_PROP(TYPE, Prefix)                           \
  register_ptr_to_python<FilteredTimeSeriesProperty<TYPE> *>();                \
                                                                               \
  class_<FilteredTimeSeriesProperty<TYPE>, bases<TimeSeriesProperty<TYPE>>,    \
         boost::noncopyable>(#Prefix "FilteredTimeSeriesProperty", no_init)    \
      .def(init<TimeSeriesProperty<TYPE> *, const TimeSeriesProperty<bool> &,  \
                const bool>("Constructor", (arg("source"), arg("filter"),      \
                                            arg("transferOwner"))))            \
      .def("unfiltered", &FilteredTimeSeriesProperty<TYPE>::unfiltered,        \
           return_value_policy<RemoveConst>(),                                 \
           "Returns a time series containing the unfiltered data");
}

void export_FilteredTimeSeriesProperty() {
  EXPORT_FILTEREDTIMESERIES_PROP(double, Float);
  EXPORT_FILTEREDTIMESERIES_PROP(bool, Bool);
  EXPORT_FILTEREDTIMESERIES_PROP(int32_t, Int32);
  EXPORT_FILTEREDTIMESERIES_PROP(int64_t, Int64);
  EXPORT_FILTEREDTIMESERIES_PROP(std::string, String);
}
