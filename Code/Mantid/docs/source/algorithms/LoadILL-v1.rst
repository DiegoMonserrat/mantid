.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an ILL TOF NeXus file into a `Workspace2D <http://www.mantidproject.org/Workspace2D>`_ with
the given name.

This loader calculates the elastic peak position (EPP) on the fly. In
cases where the dispersion peak might be higher than the EPP, it is good
practice to load a Vanadium file.

The property FilenameVanadium is optional. If it is present the EPP will
be loaded from the Vanadium data.

To date this algorithm only supports: IN4, IN5 and IN6

Usage
-----

Assuming that the files exist for either ILL IN4, 5 or 6:

.. testcode::

   # Regular data file.
   dataRegular = '<... .nxs>'
   # Data file where the dispersion peak is higher than the elastic peak.
   dataDispersionPeak = '<... .nxs>'
   # Vanadium file collected in the same conditions as the dispersion peak dataset.
   vanaDispersionPeak = '<... .nxs>'

**Example - Load a regular histogram Nexus file:**
(see :ref:`algm-LoadILL` for more options)

.. testcode:: Ex1

   # Load ILL dataset
   ws = Load(dataRegular)

**Example - Load a histogram Nexus file where the dispersion peak is higher than the elastic peak.
An auxiliary vanadium file is needed to locate the elastic peak.:**
(see :ref:`algm-LoadILL` for more options)

.. testcode:: Ex2

   # Load ILL dispersion peak dataset and a vanadium dataset
   ws = Load(dataDispersionPeak, FilenameVanadium=vanaDispersionPeak)

**Example - Same example as above, but the vanadium file is loaded in advance. The dataset for the dispersion peak is loaded after, using the auxiliary vanadium workspace.:**
(see :ref:`algm-LoadILL` for more options)

.. testcode:: Ex3

   # Load the Vanadium
   wsVana = Load(dataDispersionPeak)

   # Load ILL dispersion peak dataset and a vanadium dataset
   wsData = Load(dataDispersionPeak, WorkspaceVanadium=wsVana)




.. categories::
