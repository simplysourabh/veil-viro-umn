#!/usr/bin/python
# Sourabh Jain (sourj@cs.umn.edu)

# it generates the click configuration file by learning the interfaces automatically.
# it uses the utility ifconfig to automatically detect the attached interfaces.
# it assumes that "ifconfig | grep HWaddr" will provide the necessary information.
# it then generates the temp.click file.

# command line syntax: python generateClickConf.py <vccinterfacetag, e.g. eth1> <output file>
import sys,time, subprocess

scriptfile = "temp.click"
vccmac = ""
exclude_interfaces = []
if len(sys.argv) < 4:
	print 'Usage: ', sys.argv[0], '<VCC MAC Address>  <Output File name> <speed per link e.g. 1000KBps> <EXCLUDE interfaces as a comma sep list.e.g. eth0,eth1 (no spaces)>'
	sys.exit()

if len(sys.argv) >= 2:
	vccmac = sys.argv[1]

if len(sys.argv) >= 3:
	scriptfile = sys.argv[2]
linespeed = sys.argv[3].strip()

if len(sys.argv) >= 5:
	exclude_interfaces = sys.argv[4].split(',')


print "Output click configuration file: ", scriptfile
print "Excluding the interfaces: ", exclude_interfaces

fout = open(scriptfile, "w")
print>>fout, "/* Automatically generated by the python script:", sys.argv[0],  "at", time.ctime(),"*/ \n"
#print>>fout, "require(veil);\n"

# now extract the interfaces and the mac addresses for each of them.
# I assume that "ifconfig | grep HWaddr" will return all the relavant
# interfaces on the host.
cmd = ['ifconfig', '-a']
p = subprocess.Popen(cmd, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
s = p.communicate()
if s[1].strip() != '':
	print "Error occurred: ",s[1]

interfaces = {}
rinterfaces = {}
outline = s[0].split('\n')
for l in outline:
	if l.find("HWaddr") >= 0:
		intname = (l.split(" "))[0].strip()
		intname.replace(':', '')

		hwaddr = (l.split("HWaddr"))[1].strip()
		if intname not in exclude_interfaces:
			interfaces[intname] = hwaddr
			rinterfaces[hwaddr] = intname
ninterfaces = len(interfaces)
print "Interfaces: ", interfaces
print "Reverse interfaces: ", rinterfaces
print "Total Interfaces: ", ninterfaces

# bring up all the interfaces and put them into the promisc mode.
for intname in interfaces:
	cmd = ['ifconfig', intname, 'up', 'promisc']
	p = subprocess.Popen(cmd, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
	p.wait()
	print 'setting up the interface', intname, 'in promisc mode.', cmd


# update the mtu for all the interfaces.
for intname in interfaces:
	cmd = ['ifconfig', intname, 'mtu', '1600']
	p = subprocess.Popen(cmd, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
	p.wait()
	print 'setting up the mtu for interface', intname, 'as 1600 bytes!'


# first initialize the interface element.
print>>fout,"interfaces::VEILInterfaceTable("
for eth in interfaces:
	print>>fout,"                   000000000000, "+interfaces[eth]+", "

print>>fout, '                   UseStatic false,\n                   PRINTDEBUG false\n);\n'
print>>fout, 'hosts::VEILHostTable(PRINTDEBUG false);'
print>>fout, 'neighbors::VEILNeighborTable(PRINTDEBUG false);'
print>>fout, 'mapping::VEILMappingTable(PRINTDEBUG false);'
print>>fout, 'rendezvouspoints::VEILRendezvousTable(PRINTDEBUG false);'
print>>fout, 'routes::VEILRouteTable(INTERFACETABLE interfaces, PRINTDEBUG false);'

#create the topo assignment element.
print>>fout, 'topo::VEILNetworkTopoVIDAssignment(VCCMAC '+vccmac+', PRINTDEBUG false);\n'

# now the output devices.
print>>fout, '// output devices'
i = 0
for eth in interfaces:
	print>>fout, 'out'+str(i)+'::ToDevice('+eth+');'
	print>>fout, 'q'+str(i)+'::Queue -> BandwidthShaper('+linespeed+')-> out'+str(i)+';'
	i = i +1
i = 0
print>>fout, '\n'

# now the input devices.
print>>fout, '// Input devices'
for eth in interfaces:
	print>>fout, 'in'+str(i)+'::FromDevice('+eth+');'
	i = i+1

i = 0
print>>fout,'\n'
print>>fout,'hellogen::VEILGenerateHelloNew (interfaces, PRINTDEBUG false);'

for i in range(0, ninterfaces):
	print>>fout, 'hellogen['+str(i)+']->q'+str(i)+';'

print>>fout,'\n//Classifier'
print>>fout,'c::Classifier(12/9876 26/0000%FFFC, 	// 0. VEIL_HELLO'
print>>fout,'              12/9876 26/0403, 		// 1. VEIL_RDV_PUBLISH'
print>>fout,'              12/9876 26/0404, 		// 2. VEIL_RDV_QUERY'
print>>fout,'              12/9876 26/0402, 		// 3. VEIL_RDV_REPLY'
print>>fout,'              12/9876 26/0602, 		// 4. VEIL_ENCAP_ARP'
print>>fout,'              12/9876 26/0201, 		// 5. VEIL_MAP_PUBLISH'
print>>fout,'              12/9876 26/0202, 		// 6. VEIL_MAP_UPDATE'
print>>fout,'              12/9876 26/0203, 		// 7. NO_VID_TO_ACCESS_SWITCH (should go to ProcessAccessInfo.cc)'
print>>fout,'              12/9876 26/0601, 		// 8. VEIL_ENCAP_IP'
print>>fout,'              12/9876 26/0801, 		// 9. VEIL_ENCAP_MULTIPATH_IP'
print>>fout,'              12/9878,	       		// 10. ETHERTYPE_VEIL_IP'
print>>fout,'              12/0806,         		// 11. ETHERTYPE_ARP'
print>>fout,'              12/0800,         		// 12. ETHERTYPE_IP'
print>>fout,'              12/9876 26/0100%FF00, 	// 13. VCC Packets.'
print>>fout,'              -);    	       		//14. Everything Else (DISCARD)'
print>>fout,''

print>>fout,'//Port Annotations'
for i in range(0, ninterfaces):
	print>>fout,'in'+str(i)+' -> VEILSetPortAnnotation('+str(i)+') -> c;'

print>>fout,''
print>>fout,'router::VEILRoutePacket(hosts, routes, interfaces, neighbors, PRINTDEBUG false);'

for i in range(0,ninterfaces):
	print>>fout,'router['+str(i)+'] -> q'+str(i)+';'

i = i+1
print>>fout,'router['+str(i)+'] -> c;'
i = i+1
print>>fout,'router['+str(i)+'] -> Discard;'

if vccmac in rinterfaces:
	print>>fout,'\n//I am the VCC'
	print>>fout,'\n//Writing the initial advertisement generator.'
	vccstr = vccmac.replace(':','')
	print>>fout,'\ndatasAd::TimedSource(DATA \<ffffffffffff '+vccstr +' 9876 '+vccstr+' 000000000000 0101 0200 '+vccstr+' 0000>, INTERVAL 5) -> c;'
	print>>fout,'vccstate::VEILSpanningTreeState('+vccmac+' '+vccmac+' 0, PRINTDEBUG false);'
else:
	print>>fout,'vccstate::VEILSpanningTreeState(PRINTDEBUG false);'

print>>fout,'vccgenerator::VEILGenerateVCCSTAdSub(interfaces, neighbors, vccstate, PRINTDEBUG false);'
print>>fout,'vccprocessor::VEILProcessVCCSTAdSub(INTERFACETABLE interfaces, NEIGHBORTABLE neighbors, SPANNINGTREESTATE vccstate, NETWORKTOPO topo, PRINTDEBUG false);'
print>>fout,'vidgenerator::VEILGenerateVIDAssignmentPackets(INTERFACETABLE interfaces, NEIGHBORTABLE neighbors, SPANNINGTREESTATE vccstate, NETWORKTOPO topo, PRINTDEBUG false);'

for i in range(0, ninterfaces):
	print>>fout,'vccgenerator['+str(i)+'] -> q'+str(i)+';'
i=i+1	
print>>fout,'vccgenerator['+str(i)+'] -> c;\n'


if vccmac in rinterfaces:
	for i in range(0, ninterfaces):
		print>>fout,'vidgenerator['+str(i)+'] -> q'+str(i)+';'
	i=i+1
	print>>fout,'vidgenerator['+str(i)+'] -> c;\n'

for i in range(0, ninterfaces):
	print>>fout,'vccprocessor['+str(i)+'] -> q'+str(i)+';'

print>>fout,'\nc[13] -> vccprocessor;'

print>>fout,'\nphello::VEILProcessHelloNew(neighbors, interfaces,PRINTDEBUG false);'
print>>fout,'c[0] -> phello;'


for i in range(0, ninterfaces):
	print>>fout,'phello['+str(i)+'] -> q'+str(i)+';'
print>>fout,''

print>>fout,'prdv::VEILProcessRDV(routes, rendezvouspoints, interfaces, PRINTDEBUG false) -> router;\n'

print>>fout,'c[1] -> prdv;'
print>>fout,'c[2] -> prdv;'
print>>fout,'c[3] -> prdv;\n'

print>>fout,'parp::VEILProcessARP(hosts, mapping, interfaces,PRINTDEBUG false) ->  router;\n'

print>>fout,'c[11]  -> ARPPrint ->  parp;'
print>>fout,'c[4]   -> Print (VEIL_ARP, MAXLENGTH 100) ->  parp;\n'

print>>fout,'paci::VEILProcessAccessInfo(hosts,mapping, interfaces,PRINTDEBUG false) -> router;'
print>>fout,'c[5] -> paci;'
print>>fout,'c[6] -> paci;'
print>>fout,'c[7] -> paci;\n'

print>>fout,'pip::VEILProcessIP(hosts, mapping, interfaces,FORWARDING_TYPE 2, PRINTDEBUG false)-> router;'

print>>fout,'c[8]  -> pip;'
print>>fout,'c[9]  -> pip;'
print>>fout,'c[10] -> pip;'
print>>fout,'c[12] -> pip;\n'


print>>fout,'VEILBuildRouteTable(neighbors, routes, interfaces,PRINTDEBUG false) -> router;\n'

print>>fout,'VEILPublishAccessInfo(hosts, PRINTDEBUG false) -> router;\n'

print>>fout,'c[14] -> Discard;\n'


print>>fout,'Script(wait 0s, print interfaces.table, wait 60s, loop);'
print>>fout,'Script(wait 1s, print routes.table, wait 59s, loop);'
print>>fout,'Script(wait 2s, print neighbors.table, wait 58s, loop);'
print>>fout,'Script(wait 3s, print rendezvouspoints.table, wait 57s, loop);'
print>>fout,'Script(wait 4s, print hosts.table, wait 16s, loop);'
print>>fout,'Script(wait 5s, print mapping.table, wait 15s, loop);'
print>>fout,'Script(wait 6s, print vccstate.state, wait 54s, loop);'
print>>fout,'Script(wait 7s, print topo.table, wait 53s, loop);'


fout.close()

