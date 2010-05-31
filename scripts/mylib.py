import os
def getNetworkInterfaces():
	# Returns the network interface info on the machine
	# specific to ubunto 9.*** OS
	# uses ifconfig, and parses the output.
	# requires os python package
	netinfo = [];
	for line in os.popen("ifconfig"):
		if line.find("Link encap:") >= 0 and line.find("Loopback") < 0:
			print line
			interfaceinfo = {};
			interfaceinfo['name'] = (line.split(" "))[0].strip()
			interfaceinfo['hwaddr'] = (line.split("HWaddr"))[1].strip()	
			netinfo.append(interfaceinfo)
	
	return netinfo;
