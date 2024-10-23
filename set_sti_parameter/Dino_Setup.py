#!/usr/bin/python
import sys
sys.path.append('/home/maxwell/MaxLab/python/lib/python3.10/site-packages')
import maxlab
import maxlab.system
import maxlab.chip
import maxlab.util
import maxlab.saving

import random
import time

## Setup script to prepare the configurations for the close-loop module
#
# 0. Initialize system
# 
# 1. Load a previously created configuration
# 
# 2. Connect two electrodes to stimulation units and power up stimulation units
# 
#    In rare cases it can happen that the selected electrode cannot be
#    stimulated. So, always check whether the electrode got properly
#    connected. As it is done in this example.  If the electrode is not
#    properly connected, the stimulation has no effect.
#
# 3. Prepare two different sequences of pulse trains
#
#    An almost infinite amount (only limited by the computer memory) of
#    independent stimulation pulse trains can be prepared without actually
#    deliver them yet.
#
# 4. Deliver the two pulse trains randomly
#
#    The previously prepared pulse trains can be delivered whenever
#    seems reasonable, or following a specific stimulation schedule.
# 


name_of_configuration = "closeLoop.cfg"
trigger_electrode = 14471
electrode1 = 13378
electrode2 =  13248
trigger_stimulation_amplitude = 17
close_loop_stimulation_amplitude = 9
data_directory = "."

inter_pulse_interval = 2000 # in ms


######################################################################
# 0. Initialize system
######################################################################

# The next two lines remove the two sequences, in case they were
# already defined in the server. We need to clear them here,
# otherwise the close_loop stimulation would also trigger, when
# we reconfigure the chip, due to the artifacts.
s = maxlab.Sequence('trigger', persistent=False)
del(s)
s = maxlab.Sequence('close_loop1', persistent=False)
del(s)
s = maxlab.Sequence('close_loop2', persistent=False)
del(s)


# Normal initialization of the chip
maxlab.util.initialize()
maxlab.send(maxlab.chip.Core().enable_stimulation_power(True))
maxlab.send(maxlab.chip.Amplifier().set_gain(512))


# For the purpose of this example, we set the spike detection
# threshold a bit higher, to avoid/reduce false positives.
# Set detection threshold to 8.5x std of noise
maxlab.send_raw("stream_set_event_threshold  5")


######################################################################
# 1. Load a previously created configuration
######################################################################

array = maxlab.chip.Array('stimulation')
array.load_config( name_of_configuration )


######################################################################
# 2. Connect two electrodes to stimulation units
#    and power up the stimulation units
######################################################################

array.connect_electrode_to_stimulation( trigger_electrode )
array.connect_electrode_to_stimulation( electrode1 )
array.connect_electrode_to_stimulation( electrode2 )

trigger_stimulation = array.query_stimulation_at_electrode( trigger_electrode )
stimulation1 = array.query_stimulation_at_electrode( electrode1 )
stimulation2 = array.query_stimulation_at_electrode( electrode2 )


if not trigger_stimulation:
    print("Error: electrode: " + str(trigger_electrode) + " cannot be stimulated")

if not stimulation1:
    print("Error: electrode: " + str(electrode1) + " cannot be stimulated")

if not stimulation2:
    print("Error: electrode: " + str(electrode2) + " cannot be stimulated")

# Download the prepared array configuration to the chip
array.download()
maxlab.util.offset()


# Prepare commands to power up and power down the two stimulation units
# cmd_power_stim1 = maxlab.chip.StimulationUnit(stimulation1).power_up(True).connect(True).set_voltage_mode().dac_source(0)
# cmd_power_down_stim1 = maxlab.chip.StimulationUnit(stimulation1).power_up(False)
# cmd_power_stim2 = maxlab.chip.StimulationUnit(stimulation2).power_up(True).connect(True).set_voltage_mode().dac_source(0)
# cmd_power_down_stim2 = maxlab.chip.StimulationUnit(stimulation2).power_up(False)


# Use the electrode next to the stimulation electrode for trigger detection
# amp = array.query_amplifier_at_electrode( trigger_electrode+1 )
# print("This amplifier channel is connected to the electrode next to the first stimulation electrode: " + amp)
# print("Use this channel to detect (simulated) spikes in the C++ close cloop application")



######################################################################
# 3. Prepare two different sequences of pulse trains
######################################################################

def append_stimulation_pulse(seq, amplitude):
    seq.append( maxlab.chip.DAC(0, 512-amplitude) )
    seq.append( maxlab.system.DelaySamples(4) )
    seq.append( maxlab.chip.DAC(0, 512+amplitude) )
    seq.append( maxlab.system.DelaySamples(4) )
    seq.append( maxlab.chip.DAC(0, 512) )
    return seq


def create_sequence(token, stim_unit, amplitude,inter_pulse_interval, nr_of_pulses):
    stim = maxlab.chip.StimulationUnit(stim_unit)
    power_up = stim.power_up(True).connect(True).set_voltage_mode().dac_source(0)  # power up the stimulation unit
    power_down = maxlab.chip.StimulationUnit(stim_unit).power_up(False)            # power down the stimulation unit
    seq = maxlab.Sequence(token, persistent=True)
    seq.append(power_up)
    for i in range(nr_of_pulses-1):
        append_stimulation_pulse(seq, amplitude)
        seq.append(maxlab.system.DelaySamples(inter_pulse_interval * 20))
    append_stimulation_pulse(seq, amplitude)                                    #保证没有delaysample在最后
    seq.append(power_down)
    return seq

# Prepare one pulse called 'trigger', we use this to simulate a spike
# on one of the channels by applying an electrical stimulation pulse
IPI_trigger=inter_pulse_interval
seq_trig = create_sequence('trigger', trigger_stimulation, trigger_stimulation_amplitude, 0,1)



# Create another pulse called 'close_loop' to stimulate the second electrode.
# This sequence needs to be prepared here in python, but it will be triggered
# through the 'close_loop' token in the C++ application
IPI_close_loop1 = inter_pulse_interval
seq1 = create_sequence('close_loop1', stimulation1, trigger_stimulation_amplitude,0, 1)
IPI_close_loop2 = inter_pulse_interval
seq2 = create_sequence('close_loop2', stimulation2, close_loop_stimulation_amplitude,50, 8)


######################################################################
# Deliver 200 stimulation pulses to the trigger electrode
######################################################################

print("Start delivering stimulation pulses to the trigger electrode")

s = maxlab.saving.Saving()
s.open_directory(data_directory)

s.group_delete_all()
s.group_define(0, "routed")

print("Start saving to file")
#s.start_file("close_loop_test")
#s.start_recording([0])

for rep in range(5):
    seq_trig.send()
    time.sleep(2)

print("Stop saving to file")
#s.stop_recording()
#s.stop_file()



