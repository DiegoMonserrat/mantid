.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an ILL Back Scattering NeXus file into a `Workspace2D <http://www.mantidproject.org/Workspace2D>`_ with
the given name.

The Units axis is defined with *empty* units. The main purpose of this loader is to be used with the **Indirect Load** interface.

To date this algorithm only supports: IN16B

Usage
-----

**Example - Load ILL IN16B NeXus file:**

.. testcode:: Ex

   # Load ILL IN16B data file into a workspace 2D.
   ws = Load('<ILL IN16B data file>')

   print "This workspace has", ws.getNumDims(), "dimensions and has", ws.getNumberHistograms(), "histograms."

Output:

.. testoutput:: Ex
	
	This workspace has 2 dimensions and has 2057 histograms.

.. categories::
