[VEIL-VIRO umn networking homepage](http://networking.cs.umn.edu/wiki/index.php/VEIL-VIRO)

[A video demonstration of the VEIL routing architecture](http://www.youtube.com/watch?feature=player_embedded&v=oDb5cflAWL4)

## Motivation ##
Today’s Internet is increasingly strained to meet the demands and requirements of these Internet services and their users, such as scalability to accommodate the increasing number of network components and host devices, high availability (i.e., “always-on” services), robustness, mobility and security. As the universal “glue” that pieces together various heterogeneous physical networks, the Internet Protocol (IP) suffers certain well-known shortcomings, e.g., it requires careful and extensive network configurations – in particular, the need for address management and configuration, inherently reactive approaches for handling network failures, relatively poor support for mobility, and so forth. In addition, despite the potential benefits offered by a larger address space, transition from IPv4 to IPv6 has been difficult and slow; among a variety of other factors, the tight coupling of addressing, routing and other network layer functions clearly make such transition not an easy task. While Ethernet is largely plug-&-play, as it was originally developed for small, local area networks, this traditional layer-2 technology can hardly meet the scale as well as the demanding efficiency and robustness requirements imposed on today’s large, dynamic networks.

## Goal ##
VIRO has following two broad goals:
  1. Support – with minimal manual configuration – for (future) large, dynamic networks which connect tens or hundreds of thousands of diverse devices with rich physical topologies;
  1. Meet the high availability, robustness, mobility, manageability and security requirements for large-scale networks (such as: data-centers, enterprise networks, metro ethernets etc.)

## Key Ideas ##
  1. VIRO provides a more scalable, and yet efficient routing architecture by deploying "DHT-style" routing.
  1. It provides Location/Identity split, by inserting a naming layer, solely used for performing the routing between "VIRO-switches". End-device then dynamically mapped to this "Virtual ID" space to exchange the data with each other. Further it provides a freedom from the "identifiers" that can be used to access the network services and data.  Therefore, we can use any namespace such as MAC addresses, DNS names, or URL etc to exchange between hosts.
  1. It provides much better support for mobility by separating Location and Identity.
  1. In addition, DHT-style substrate used in VIRO, provides a natural way to support Multi-path Routing, Load-balancing and Fast-Failure Rerouting.

## People ##
  * [Zhi-Li Zhang](http://www.cs.umn.edu/~zhzhang)
  * [Sourabh Jain](http://www.cs.umn.edu/~sourj)
  * [Vijay Kumar Adhikari](http://www.cs.umn.edu/~viadhi)
  * [Yingying Chen](http://www.cs.umn.edu/~yingying)
  * Gowri CP

## Publication ##
  * _VEIL: A ''Plug-&-Play'' Virtual (Ethernet) Id Layer for Below IP Networking_ [pdf](http://www.users.cs.umn.edu/~zhzhang/Papers/veil-final-BIPN.pdf) Sourabh Jain, Yingying Chen, Zhi-Li Zhang; Conference Paper: Published at First IEEE Workshop on Below IP Networking in conjunction with '_GLOBECOM_ 2009
  * _Virtual Id Routing: A scalable routing framework with support for mobility and routing efficiency_ [Link](http://portal.acm.org/citation.cfm?id=1403007.1403025) to paper on ACM Portal.  Guor-Huor Lu, Sourabh Jain, Shanzhen Chen, Zhi-Li Zhang; Published in Third International Workshop on Mobility in the Evolving Internet Architecture, _MobiArch'08_ co-located with _SIGCOMM_ 2008. Paper (ACM Portal)


## Overview of design ##
_veil-click_ is based on Click modular framework, where
different functionalities are broken in to individual modules,
which can be developed independently, and plugged-in together
to compose a full fledged _veil-switch_.
In case of _veil-click_, we build several elements (or modules)
for various operations such as ARP handlers, Routing Table manager etc.
as Click elements. These individual elements are connected
together to form various components of _veil-click_.

There are five key groups
of elements in veil-click, namely:

a) **Link Discovery Element** which performs the neighbor discovery via the periodic exchange of HELLO packets.

b) **vcc Manager** Elements in this group are responsible to exchange information with the _vcc_, and get the _vid_-assignment from the _vcc_ using _vcc_ communication protocol.

c) **VIRO Routing Controller** These elements perform the necessary routing table construction  tasks, and perform the packet forwarding tasks.

d) **Data packet handlers** These elements handle the various types of data packets sent by the hosts, such as ARP and IP packets.

e) **Host-device Manager** Elements in this group are responsible for various host related tasks such as, assigning the _vids_ to the _hostdevices_ directly connected to the switch, publish and store the mapping to _accessswitches_ etc.