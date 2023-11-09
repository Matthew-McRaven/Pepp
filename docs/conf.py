import sys, os
# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Pepp'
copyright = '2023, Matthew McRaven'
author = 'Matthew McRaven'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration
# The path where you put the only.py file
sys.path.append(os.path.abspath('.'))
extensions = [ "breathe", "scope", "copy_dirs" ]

templates_path = ['_templates']
exclude_patterns = []

breathe_default_project = "all"


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'alabaster'
html_static_path = ['_static']

html_css_files = ['css/overrides.css']
html_theme_options = {
  'page_width': 'auto',
  'body_max_width': 'auto',
}
