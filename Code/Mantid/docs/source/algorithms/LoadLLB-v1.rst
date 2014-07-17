.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an LLB MIBEMOL TOF NeXus file into a `Workspace2D <http://www.mantidproject.org/Workspace2D>`_
with the given name.

This loader calculates the elastic peak position (EPP) on the fly.

To date this algorithm only supports the MIBEMOL instrument.

Usage
-----

**Example - Load a LLB MIBEMOL NeXus file:**
(see :ref:`algm-LoadLLB` for more options)

.. testcode:: Ex

   # Load the MIBEMOL dataset into a workspace2D.
   ws = Load('<LLB MIBEMOL data file>')

   print "This workspace has", ws.getNumDims(), "dimensions and its title is:", ws.getTitle()

Output:

.. testoutput:: Ex

   This workspace has 2 dimensions and its title is: Lysozyme/D2O (c=80 mg/ml) Treg=293 K, Tech=288 K  wl=5.2A  sous pression (4.5 kbar)

.. categories::
