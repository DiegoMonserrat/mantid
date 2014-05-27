"""
    Provides directives for dealing with category pages.

    While parsing the directives a list of the categories and associated pages/subcategories
    is tracked. When the final set of html pages is collected, a processing function
    creates "index" pages that lists the contents of each category. The display of each
    "index" page is controlled by a jinja2 template.
"""
from base import BaseDirective

CATEGORY_INDEX_TEMPLATE = "category.html"
# relative to the "root" directory
CATEGORIES_HTML_DIR = "categories"

class LinkItem(object):
    """
    Defines a linkable item with a name and html reference
    """
    # Name displayed on listing page
    name = None
    # html link
    link = None

    def __init__(self, name, env):
        """
        Arguments:
          env (Sphinx.BuildEnvironment): The current environment processing object
        """
        self.name = str(name)

        rel_path = env.docname  # no suffix
        # Assumes the link is for the current document and that the
        # page that will use this reference is in a single
        # subdirectory from root
        self.link = "../%s.html" % rel_path

    def __eq__(self, other):
        """
        Define comparison for two objects as the comparison of their names

        Arguments:
          other (PageRef): Another PageRef object to compare
        """
        return self.name == other.name

    def __hash__(self):
        return hash(self.name)

    def __repr__(self):
        return self.name

    def html_link(self):
        """
        Returns a link for use as a href to refer to this document from a
        categories page. It assumes that the category pages are in a subdirectory
        of the root and that the item to be referenced is in the algorithms directory
        under the root.

        Returns:
          str: A string containing the link
        """
        return self.link
# endclass

class PageRef(LinkItem):
    """
    Store details of a single page reference
    """

    def __init__(self, name, env):
        super(PageRef, self).__init__(name, env)

#endclass

class Category(LinkItem):
    """
    Store information about a single category
    """
    # Collection of PageRef objects that link to members of the category 
    pages = None
    # Collection of PageRef objects that form subcategories of this category
    subcategories = None

    def __init__(self, name, env):
        super(Category, self).__init__(name, env)

        # override default link
        self.link = "../categories/%s.html" % name
        self.pages = set([])
        self.subcategories = set([])

#endclass

class CategoriesDirective(BaseDirective):
    """
    Records the page as part of the given categories. Index pages for each
    category are then automatically created after all pages are collected
    together.

    Subcategories can be given using the "\\" separator, e.g. Algorithms\\Transforms
    """

    # requires at least 1 category
    required_arguments = 1
    # it can be in many categories and we put an arbitrary upper limit here
    optional_arguments = 25

    def run(self):
        """
        Called by Sphinx when the defined directive is encountered.
        """
        categories = self._get_categories_list()
        display_name = self._get_display_name()
        links = self._create_links_and_track(display_name, categories)

        return self._insert_rest("\n" + links)

    def _get_categories_list(self):
        """
        Returns a list of the category strings

        Returns:
          list: A list of strings containing the required categories
        """
        # Simply return all of the arguments as strings
        return self.arguments

    def _get_display_name(self):
        """
        Returns the name of the item as it should appear in the category
        """
        env = self.state.document.settings.env
        # env.docname returns relative path from doc root. Use name after last "/" separator
        return env.docname.split("/")[-1]

    def _create_links_and_track(self, page_name, category_list):
        """
        Return the reST text required to link to the given
        categories. As the categories are parsed they are
        stored within the current environment for use in the
        "html_collect_pages" function.

        Args:
          page_name (str): Name to use to refer to this page on the category index page
          category_list (list): List of category strings

        Returns:
          str: A string of reST that will define the links
        """
        env = self.state.document.settings.env
        if not hasattr(env, "categories"):
            env.categories = {}

        link_rst = ""
        ncategs = 0
        for item in category_list:
            if r"\\" in item:
                categs = item.split(r"\\")
            else:
                categs = [item]
            # endif

            parent = None
            for index, categ_name in enumerate(categs):
                if categ_name not in env.categories:
                    category = Category(categ_name, env)
                    env.categories[categ_name] = category
                else:
                    category = env.categories[categ_name]
                #endif

                category.pages.add(PageRef(page_name, env))
                if index > 0: # first is never a child
                    parent.subcategories.add(Category(categ_name, env))
                #endif

                link_rst += "`%s <../%s/%s.html>`_ | " % (categ_name, CATEGORIES_HTML_DIR, categ_name)
                ncategs += 1
                parent = category
            # endfor
        # endfor

        link_rst = "**%s**: " + link_rst.rstrip(" | ") # remove final separator
        if ncategs == 1:
            link_rst = link_rst % "Category"
        else:
            link_rst = link_rst % "Categories"
        #endif

        return link_rst
    #end def

#---------------------------------------------------------------------------------

class AlgorithmCategoryDirective(CategoriesDirective):
    """
        Supports the "algm_categories" directive that takes a single
        argument and pulls the categories from an algorithm object.

        In addition to adding the named page to the requested category, it
        also appends it to the "Algorithms" category
    """
    # requires at least 1 argument
    required_arguments = 0
    # no other arguments
    optional_arguments = 0

    def _get_categories_list(self):
        """
        Returns a list of the category strings

        Returns:
          list: A list of strings containing the required categories
        """
        category_list = ["Algorithms"]
        algname, version = self._algorithm_name_and_version()
        alg_cats = self._create_mantid_algorithm(algname, version).categories()
        for cat in alg_cats:
            # double up the category separators so they are not treated as escape characters
            category_list.append(cat.replace("\\", "\\\\"))

        return category_list

    def _get_display_name(self):
        """
        Returns the name of the item as it should appear in the category
        """
        return self._algorithm_name_and_version()[0]

#---------------------------------------------------------------------------------

def html_collect_pages(app):
    """
    Callback for the 'html-collect-pages' Sphinx event. Adds category
    pages + a global Categories.html page that lists the pages included.

    Function returns an iterable (pagename, context, html_template),
    where context is a dictionary defining the content that will fill the template

    Arguments:
      app: A Sphinx application object 
    """
    if not hasattr(app.builder.env, "categories"):
        return # nothing to do

    for name, context, template in create_category_pages(app):
        yield (name, context, template)
# enddef

def create_category_pages(app):
    """
    Returns an iterable of (category_name, context, "category.html")

    Arguments:
      app: A Sphinx application object 
    """
    env = app.builder.env

    # jinja2 html template
    template = CATEGORY_INDEX_TEMPLATE

    categories = env.categories
    for name, category in categories.iteritems():
        context = {}
        context["title"] = category.name
        # sort subcategories & pages by first letter
        context["subcategories"] = sorted(category.subcategories, key = lambda x: x.name[0])
        context["pages"] = sorted(category.pages, key = lambda x: x.name[0])

        yield (CATEGORIES_HTML_DIR + "/" + name, context, template)
# enddef

#------------------------------------------------------------------------------
def setup(app):
    # Add categories directive
    app.add_directive('categories', CategoriesDirective)
    # Add algm_categories directive
    app.add_directive('algm_categories', AlgorithmCategoryDirective)

    # connect event html collection to handler
    app.connect("html-collect-pages", html_collect_pages)

