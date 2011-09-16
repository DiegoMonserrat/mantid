"""
    Load EQSANS data file and perform TOF corrections, move detector to beam center, etc...
"""
import os
import sys
import pickle
import math
from reduction import ReductionStep
from reduction import extract_workspace_name, find_file, find_data
from eqsans_config import EQSANSConfig
from sans_reduction_steps import BaseBeamFinder

# Mantid imports
from mantidsimple import *

class LoadRun(ReductionStep):
    """
        Load a data file, move its detector to the right position according
        to the beam center and normalize the data.
    """
    def __init__(self, datafile=None, keep_events=False):
        super(LoadRun, self).__init__()
        self._data_file = datafile
        
        # TOF range definition
        self._use_config_cutoff = False
        self._low_TOF_cut = 0
        self._high_TOF_cut = 0
        
        # TOF flight path correction
        self._correct_for_flight_path = False
        
        # Use mask defined in configuration file
        self._use_config_mask = False
        self._use_config = True
        
        # Workspace on which to apply correction that should be done
        # independently of the pixel. If False, all correction will be 
        # applied directly to the data workspace.
        self._separate_corrections = False
        
        self._sample_det_dist = None
        self._sample_det_offset = 0
        
        # Flag to tell us whether we should do the full reduction with events
        self._keep_events = keep_events
        self._is_new = True
   
    def clone(self, data_file=None, keep_events=None):
        if data_file is None:
            data_file = self._data_file
        if keep_events is None:
            keep_events = self._keep_events
        loader = LoadRun(datafile=data_file, keep_events=keep_events)
        loader._use_config_cutoff = self._use_config_cutoff
        loader._low_TOF_cut = self._low_TOF_cut
        loader._high_TOF_cut = self._high_TOF_cut
        loader._correct_for_flight_path = self._correct_for_flight_path
        loader._use_config_mask = self._use_config_mask
        loader._use_config = self._use_config
        return loader

    def set_sample_detector_distance(self, distance):
        # Check that the distance given is either None of a float
        if distance is not None and type(distance) != int and type(distance) != float:
            raise RuntimeError, "LoadRun.set_sample_detector_distance expects a float: %s" % str(distance)
        self._sample_det_dist = distance
        
    def set_sample_detector_offset(self, offset):
        # Check that the offset given is either None of a float
        if offset is not None and type(offset) != int and type(offset) != float:
            raise RuntimeError, "LoadRun.set_sample_detector_offset expects a float: %s" % str(offset)
        self._sample_det_offset = offset
        
    def set_flight_path_correction(self, do_correction=False):
        """
            Set the flag to perform the TOF correction to take into
            account the different in flight path at larger angle.
            @param do_correction: if True, correction will be made
        """
        self._correct_for_flight_path = do_correction
        
    def set_TOF_cuts(self, low_cut=0, high_cut=0):
        """
            Set the range of TOF to be cut on each side of the frame.
            @param low_cut: TOF to be cut from the low-TOF end
            @param high_cut: TOF to be cut from the high-TOF end
        """
        self._low_TOF_cut = low_cut
        self._high_TOF_cut = high_cut
        
    def use_config_cuts(self, use_config=False):
        """
            Set the flag to cut the TOF tails on each side of the
            frame according to the cuts found in the configuration file.
            @param use_config: if True, the configuration file will be used
        """
        self._use_config_cutoff = use_config

    def use_config(self, use_config=True):
        """
            Set the flag to use the configuration file or not.
            Only used for test purposes
        """
        self._use_config = use_config

    def use_config_mask(self, use_config=False):
        """
            Set the flag to use the mask defined in the
            configuration file.
            @param use_config: if True, the configuration file will be used
        """
        self._use_config_mask = use_config
        
    def set_beam_center(self, beam_center):
        """
            Sets the beam center to be used when loading the file
            @param beam_center: [pixel_x, pixel_y]
        """
        pass
    
    @classmethod
    def delete_workspaces(cls, workspace):
        """
            Delete all workspaces related to the loading of the given workspace
            @param workspace: workspace to clean
        """
        # Delete the beam hole transmission workspace if it exists
        if mtd[workspace].getRun().hasProperty("transmission_ws"):
            trans_ws = mtd[workspace].getRun().getProperty("transmission_ws").value
            if mtd.workspaceExists(trans_ws):
                mtd.deleteWorkspace(trans_ws)
        if mtd.workspaceExists(workspace):
            mtd.deleteWorkspace(workspace)
        
    def execute(self, reducer, workspace, force=False):
        output_str = ""      
        # If we don't have a data file, look up the workspace handle
        # Only files that are used for computing data corrections have
        # a path that is passed directly. Data files that are reduced
        # are simply found in reducer._data_files 
        if self._data_file is None:
            if workspace in reducer._data_files:
                data_file = reducer._data_files[workspace]
            elif workspace in reducer._extra_files:
                data_file = reducer._extra_files[workspace]
                force = True
            else:
                raise RuntimeError, "SNSReductionSteps.LoadRun doesn't recognize workspace handle %s" % workspace
        else:
            data_file = self._data_file

        # Load data
        use_config_beam = False
        [pixel_ctr_x, pixel_ctr_y] = reducer.get_beam_center()
        if pixel_ctr_x == 0.0 and pixel_ctr_y == 0.0:
            use_config_beam = True            
            
        def _load_data_file(file_name, wks_name):
            filepath = find_data(file_name, instrument=reducer.instrument.name())
            l = EQSANSLoad(Filename=filepath, OutputWorkspace=wks_name,
                       UseConfigBeam=use_config_beam,
                       BeamCenterX=pixel_ctr_x,
                       BeamCenterY=pixel_ctr_y,
                       UseConfigTOFCuts=self._use_config_cutoff,
                       UseConfigMask=self._use_config_mask,
                       UseConfig=self._use_config,
                       CorrectForFlightPath=self._correct_for_flight_path,
                       SampleDetectorDistance=self._sample_det_dist,
                       SampleDetectorDistanceOffset=self._sample_det_offset,
                       PreserveEvents=self._keep_events,
                       ReductionTableWorkspace=reducer.get_reduction_table_name()
                       )            
            return l.getPropertyValue("OutputMessage")
        
        # Check whether we have a list of files that need merging
        if type(data_file)==list:
            for i in range(len(data_file)):
                output_str += "Loaded %s\n" % data_file[i]
                if i==0:
                    output_str += _load_data_file(data_file[i], workspace)
                else:
                    output_str += _load_data_file(data_file[i], '__tmp_wksp')
                    Plus(LHSWorkspace=workspace,
                         RHSWorkspace='__tmp_wksp',
                         OutputWorkspace=workspace)
            if mtd.workspaceExists('__tmp_wksp'):
                mtd.deleteWorkspace('__tmp_wksp')
        else:
            output_str += "Loaded %s\n" % data_file
            output_str += _load_data_file(data_file, workspace)
        
        mantid[workspace].getRun().addProperty_str("event_ws", workspace, True)
        
        if mtd[workspace].getRun().hasProperty("beam_center_x") and \
            mtd[workspace].getRun().hasProperty("beam_center_y"):
            beam_center_x = mtd[workspace].getRun().getProperty("beam_center_x").value
            beam_center_y = mtd[workspace].getRun().getProperty("beam_center_y").value
            if type(reducer._beam_finder) is BaseBeamFinder:
                reducer.set_beam_finder(BaseBeamFinder(beam_center_x, beam_center_y))
                mantid.sendLogMessage("No beam finding method: setting to default [%-6.1f, %-6.1f]" % (beam_center_x, beam_center_y))
        
        # Remove the dirty flag if it existed
        reducer.clean(workspace)
        mtd[workspace].getRun().addProperty_int("loaded_by_eqsans_reduction", 1, True)
        
        return output_str
