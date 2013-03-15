"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the DgsReduction class could 
    be used independently of the interface implementation
"""
import os
import time
import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement

def getBooleanElement(instrument_dom, keyname, default):
    """ Get a boolean value from an element. 
    Boolean can be recorded as 
    (1) True/False
    (2) 1/0
    """
    tempbool = BaseScriptElement.getStringElement(instrument_dom,
            keyname, default=default)

    if tempbool == "True":
        tempbool = 1 
    elif tempbool == "False": 
        tempbool = 0

    returnbool = bool(int(tempbool))

    return returnbool

class VanadiumSetupScript(BaseScriptElement):
    """ Run setup script for tab 'Run Setup'
    """

    # Class static variables
    pushdatapositive = "None"
    unwrapref = ""
    lowresref = ""
    cropwavelengthmin = ""
    removepropmppulsewidth = ""
    maxchunksize = ""
    filterbadpulses = True
    stripvanadiumpeaks = False
    vanadiumfwhm = ""
    vanadiumpeaktol = ""
    vanadiumsmoothparams = ""

    def __init__(self, inst_name):
        """ Initialization
        """
        super(VanadiumSetupScript, self).__init__()
        self.createParametersList()

        self.set_default_pars(inst_name)
        self.reset()

        return

    def createParametersList(self):
        """ Create a list of parameter names for SNSPowderReductionPlus()
        """
        self.parnamelist = []
        self.parnamelist.append("UnwrapRef")
        self.parnamelist.append("LowResRef")
        self.parnamelist.append("CropWavelengthMin")
        self.parnamelist.append("RemovePromptPulseWidth")
        self.parnamelist.append("MaxChunkSize")
        self.parnamelist.append("StripVanadiumPeaks")
        self.parnamelist.append("VanadiumFWHM")
        self.parnamelist.append("VanadiumPeakTol")
        self.parnamelist.append("VanadiumSmoothParams")
        self.parnamelist.append("FilterBadPulses")
        self.parnamelist.append("PushDataPositive")

        return
        
    def set_default_pars(self, inst_name):
        """ Set default values
        """

        return
        
    def to_script(self):
        """ 'Public'  method  to save the current GUI to string via str() and general class ReductionScript
        """
        # 1. Form (partial) script
        parnamevaluedict = self.buildParameterDict()
        script = ""
        for parname in self.parnamelist:
            parvalue = parnamevaluedict[parname]
            if parvalue != "" and parname != "Instrument" and parname != "Facility":
                if str(parvalue) == "True":
                    parvalue = "1"
                elif str(parvalue) == "False":
                    parvalue = "0"
                script += "%-10s = \"%s\",\n" % (parname, parvalue)
        #ENDFOR

        return script


    def buildParameterDict(self):
        """ Create a dictionary for parameter and parameter values for SNSPowderReductionPlus()
        """
        pardict = {}
       
        pardict["UnwrapRef"] = self.unwrapref
        pardict["LowResRef"] = self.lowresref
        pardict["CropWavelengthMin"] = self.cropwavelengthmin
        pardict["RemovePromptPulseWidth"] = self.removepropmppulsewidth
        pardict["MaxChunkSize"] = self.maxchunksize
        pardict["FilterBadPulses"] = self.filterbadpulses
        pardict["PushDataPositive"] = self.pushdatapositive
        pardict["StripVanadiumPeaks"] = self.stripvanadiumpeaks
        pardict["VanadiumFWHM"] = self.vanadiumfwhm
        pardict["VanadiumPeakTol"] = self.vanadiumpeaktol
        pardict["VanadiumSmoothParams"] = self.vanadiumsmoothparams

        return pardict
        
    def to_xml(self):
        """ 'Public' method to create XML from the current data.
        """
        pardict = self.buildParameterDict()

        xml = "<VanadiumSetup>\n"
        for parname in self.parnamelist:
            value = pardict[parname]
            keyname = parname.lower()
            if str(value) == "True":
                value = '1'
            elif str(value) == "False":
                value = '0'
            xml += " <%s>%s</%s>\n" % (keyname, str(value), keyname)
        # ENDFOR
        xml += "</VanadiumSetup>\n"

        return xml
    
    def from_xml(self, xml_str):
        """ 'Public' method to read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("VanadiumSetup")
        if len(element_list)>0:
            instrument_dom = element_list[0]



            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "unwrapref", default=VanadiumSetupScript.unwrapref)
            try:
                self.unwrapref = float(tempfloat)
            except ValueError:
                self.unwrapref = ""

            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "lowresref", default=VanadiumSetupScript.lowresref)
            try:
                self.lowresref = float(tempfloat)
            except ValueError:
                self.lowresref = ""

            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "cropwavelengthmin", default=VanadiumSetupScript.cropwavelengthmin)
            try:
                self.cropwavelengthmin = float(tempfloat)
            except ValueError:
                self.cropwavelengthmin = ""

            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "removepromptpulsewidth", default=VanadiumSetupScript.removepropmppulsewidth)
            try:
                self.removepropmppulsewidth = float(tempfloat)
            except ValueError:
                self.removepropmppulsewidth = ""

            tempint = BaseScriptElement.getStringElement(instrument_dom,
                    "maxchunksize", default=VanadiumSetupScript.maxchunksize)
            try:
                self.maxchunksize = int(tempint)
            except ValueError:
                self.maxchunksize = ""


            self.filterbadpulses = getBooleanElement(instrument_dom, 
                    "filterbadpulses", VanadiumSetupScript.filterbadpulses)

            self.pushdatapositive = BaseScriptElement.getStringElement(instrument_dom,
                    "pushdatapositive", default=VanadiumSetupScript.pushdatapositive)

            self.stripvanadiumpeaks = getBooleanElement(instrument_dom, 
                    "stripvanadiumpeaks", VanadiumSetupScript.stripvanadiumpeaks)

            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "vanadiumfwhm", default=VanadiumSetupScript.vanadiumfwhm)
            try:
                self.vanadiumfwhm = float(tempfloat)
            except ValueError:
                self.vanadiumfwhm = ""

            tempfloat = BaseScriptElement.getStringElement(instrument_dom,
                    "vanadiumpeaktol", default=VanadiumSetupScript.vanadiumpeaktol)
            try:
                self.vanadiumpeaktol = float(tempfloat)
            except ValueError:
                self.vanadiumpeaktol = ""

            self.vanadiumsmoothparams = BaseScriptElement.getStringElement(instrument_dom,
                "vanadiumsmoothparams", default=VanadiumSetupScript.vanadiumsmoothparams)

            return

    def reset(self):
        """ 'Public' method to reset state
        """
        # self.sample_file = RunSetupScript.sample_file
        # self.output_wsname = RunSetupScript.output_wsname
        # self.detcal_file = RunSetupScript.detcal_file
        # self.relocate_dets = RunSetupScript.relocate_dets
        # self.incident_energy_guess = RunSetupScript.incident_energy_guess
        # self.use_ei_guess = RunSetupScript.use_ei_guess
        # self.tzero_guess = RunSetupScript.tzero_guess
        # self.monitor1_specid = RunSetupScript.monitor1_specid
        # # self.monitor2_specid = RunSetupScript.monitor2_specid
        # self.rebin_et = RunSetupScript.rebin_et
        # self.et_range_low = RunSetupScript.et_range_low
        # self.et_range_width = RunSetupScript.et_range_width
        # self.et_range_high = RunSetupScript.et_range_high
        # self.et_is_distribution = RunSetupScript.et_is_distribution
        # self.hardmask_file = RunSetupScript.hardmask_file
        # self.grouping_file = RunSetupScript.grouping_file
        # self.show_workspaces = RunSetupScript.show_workspaces

        return
