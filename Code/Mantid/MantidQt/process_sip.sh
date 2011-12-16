#!/bin/sh
# Perform processing of the .sip file to add docstrings
echo ""
python ../Build/doxygen_to_sip.py -i mantidqt.in.sip -o mantidqt.sip
echo ""
python ../Build/sip_strip_docstring.py -i mantidqt.sip -o mantidqt.rhel5.sip
echo ""

