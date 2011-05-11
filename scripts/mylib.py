#LICENSE START
# Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
# Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
#LICENSE END
import os
import subprocess

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
	
	return netinfo

def learnNeighborTopo(filename):
	# the script file for the click:
	script = open(filename,'w');

	# header for the click script:
	header = 'require(veil);\n\nc::Classifier(12/9876 26/0001, // 0. VEIL_PHELLO\n -);//1. Everything Else (DISCARD)\n\n'

	body = 'neighbors::VEILNeighborTable(PRINTDEBUG false);\n'

	# get the network info:
	netdevices = getNetworkInterfaces()

	if len(netdevices) == 0:
		print 'Warning: No physical network interfaces found!'
		sys.exit(0)
	
	# write down the interface table and initialize the element first
	body = body + 'interfaces::VEILInterfaceTable('
	for i in range(0, len(netdevices)):
		body = body + netdevices[i]['hwaddr'].replace(':','')+','
	body = body + ' PRINTDEBUG false);\n\n'

	# write down the hello generators:
	for i in range(0, len(netdevices)):
		body = body + 'VEILGeneratePHello('+ netdevices[i]['hwaddr'].replace(':','') +', PRINTDEBUG false) -> Queue ->  ToDevice(' + netdevices[i]['name']+ ');\n'

	body = body + '\n'

	# send the input packets to classifier:
	for i in range(0, len(netdevices)):
		body = body + 'FromDevice(' +netdevices[i]['name'] + ') -> VEILSetPortAnnotation(' + str(i) + ') -> c;\n'

	body = body + '\nc[0] -> VEILProcessPHello(neighbors, interfaces,PRINTDEBUG false);\n'
	body = body + 'c[1] -> Discard;\n'

	script.write(header+body)
	script.close()
	
	# run the script now
	#pid = os.fork()

	cmd = 'click ' + filename
	print 'Running command: ' + cmd
	subprocess.call('uname -a', shell=True)
	proc = subprocess.call(cmd, shell=True )
	return proc
