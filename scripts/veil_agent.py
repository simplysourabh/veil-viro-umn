# An agent for the VEIL switches.
# Requires the port number and dns (or IP) information for the centralized controller.

# This agent (VSA) will run on each VEIL-Switch, and would co-ordinate with the VCC to perform the necessary boot-strapping tasks.
# Below is the list of the tasks performed by the VSA:
#	Listen at a specified port number on local VEIL-Switch.
#	Read the output of IFCONFIG and parse it to extract the local interfaces. 
#	Run Click Elements to learn the neighbors:
#		Run generate hello packets to be sent out from each physical interface
#		Listens for hello packets coming from other hosts. 
#		Perform the above taks periodically 
#		Write the output to a local info file
#	Send the "local-topology" to the centralized controller.
#	Receive the vid-assignment from the VCC (veil centralized controller)
#		Generate the appropriate click script based on the vid assignment
#		Run the configuration and monitor it. 


################# CODE TO RUN THE CLICK SCRIPTS FOR LEARNING THE LOCAL TOPOLOGY
from mylib import *
scriptLearnTopo = 'tmp/topo.click'
learnNeighborTopo(scriptLearnTopo)





 
