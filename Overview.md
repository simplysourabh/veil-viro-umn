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
  * Gowri CP

## Publication ##
  * _VEIL: A ''Plug-&-Play'' Virtual (Ethernet) Id Layer for Below IP Networking_ [pdf](http://www.users.cs.umn.edu/~zhzhang/Papers/veil-final-BIPN.pdf) Sourabh Jain, Yingying Chen, Zhi-Li Zhang; Conference Paper: Published at First IEEE Workshop on Below IP Networking in conjunction with '_GLOBECOM_ 2009
  * _Virtual Id Routing: A scalable routing framework with support for mobility and routing efficiency_ [Link](http://portal.acm.org/citation.cfm?id=1403007.1403025) to paper on ACM Portal.  Guor-Huor Lu, Sourabh Jain, Shanzhen Chen, Zhi-Li Zhang; Published in Third International Workshop on Mobility in the Evolving Internet Architecture, _MobiArch'08_ co-located with _SIGCOMM_ 2008. Paper (ACM Portal)