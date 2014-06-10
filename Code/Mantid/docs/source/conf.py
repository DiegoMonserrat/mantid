# -*- coding: utf-8 -*-
# All configuration values have a default; values that are commented out
# serve to show the default.

import sys
import os
import sphinx_bootstrap_theme

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
sys.path.insert(0, os.path.abspath('../sphinxext'))

# -- General configuration ------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
     # we use pngmath over mathjax so that the the offline help isn't reliant on
     # anything external and we don't need to include the large mathjax package
    'sphinx.ext.pngmath',
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.doctest',
    'mantiddoc.directives'
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix of source filenames.
source_suffix = '.rst'

# The master toctree document.
master_doc = 'index'

# General information about the project.
project = u'MantidProject'
copyright = u'2014, Mantid'

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.
#
# The short X.Y version.
version = '0.01'
# The full version, including alpha/beta/rc tags.
release = '0.01'

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# -- Options for doctest --------------------------------------------------

# Run this before each test is executed
doctest_global_setup = """
from mantid.simpleapi import *
"""

# Run this after each test group has executed
doctest_global_cleanup = """
from mantid.api import FrameworkManager
FrameworkManager.Instance().clear()
"""

# -- Options for pngmath --------------------------------------------------

# Load the preview package into latex
pngmath_latex_preamble=r'\usepackage[active]{preview}'

# Ensures that the vertical alignment of equations is correct.
# See http://sphinx-doc.org/ext/math.html#confval-pngmath_use_preview
pngmath_use_preview = True

# -- Options for HTML output ----------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = 'bootstrap'

# Theme-specific options to customize the look and feel of a theme.
# We config the bootstrap settings here, and apply CSS changes in
# custom.css rather than here.
html_theme_options = {
    # Navigation bar title.
    'navbar_title': " ", # deliberate single space so it's not visible
    # Tab name for entire site.
    'navbar_site_name': "Mantid",
    # Add links to the nav bar. Second param of tuple is true to create absolute url.
    'navbar_links': [
        ("Home", "/"),
        ("Download", "http://download.mantidproject.org", True),
        ("Documentation", "/documentation"),
        ("Contact Us", "/contact-us"),
    ],
    # Do not show the "Show source" button.
    'source_link_position': "no",
    # Remove the local TOC from the nav bar
    'navbar_pagenav': False,
    # Hide the next/previous in the nav bar.
    'navbar_sidebarrel': False,
    # Use the latest version.
    'bootstrap_version': "3",
    # Ensure the nav bar always stays on top of page.
    'navbar_fixed_top': "true",
}

# Add any paths that contain custom themes here, relative to this directory.
html_theme_path = sphinx_bootstrap_theme.get_html_theme_path()

# The name of an image file (relative to this directory) to place at the top
# of the sidebar.
html_logo = os.path.relpath('../../Images/Mantid_Logo_Transparent_Cropped.png')

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# Add any extra paths that contain custom files (such as robots.txt or
# .htaccess) here, relative to this directory. These files are copied
# directly to the root of the documentation.
#html_extra_path = []

# If true, SmartyPants will be used to convert quotes and dashes to
# typographically correct entities.
html_use_smartypants = True

# Hide the Sphinx usage as we reference it on github instead.
html_show_sphinx = False

# If true, "(C) Copyright ..." is shown in the HTML footer. Default is True.
html_show_copyright = True
