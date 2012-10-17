# ZyRE - ZeroMQ Realtime Environment

ZyRE comes from [Chapter 8 of the 0MQ Guide](http://zguide.zeromq.org/page:all#toc205). It is aimed to be a framework for real-time interoperability. An answer to the "Internet of Things".

## Use Cases

* We go on a road-trip, so I load up some tablets with movies for the kids to watch.

* My kids are in the back seat during the road-trip, and they're playing a game, each on their tablets.

* The car, a rental, streams music from our phones. The kids don't like the dubstep, so they play Gangnam Style.

* As I take pictures with my camera, I can review, delete, edit them immediately on my tablet.

* I tap my phone to a reader in the hotel room, and we get Internet access on all our devices.

* In the hotel bar, my wife and I can monitor the kids sleeping, with a secure video stream from the hotel room.

* After the trip I share the photos with my cousin, sending them from my phone to his tablet.

* In the office, on Monday, I make an interactive presentation where everyone in the room can participate.

* That evening at the game, we follow the shared photo feed and vote on the game pictures we like best.

* My photos are automatically copied to my laptop, and via the hard cable connection to my cloud storage.

## Vision

ZyRE will run on everything that speaks TCP/IP and has space for code. This means smartphones, tablets, laptops, servers, TVs, toasters, ebooks, smart lightbulbs, cameras, and dongles and devices of every denomination.

## Project Organization

ZyRE is a protocol (ZRE) plus a set of implementations. We've structured the project as a collection of interoperable implementations in different languages, starting with C and Java. All protocols are on the [0MQ protocol site](http://rfc.zeromq.org).

## Who owns ZyRE?

ZyRE is owned by all its authors and contributors. This is an open source project licensed under the LGPLv3. To contribute to ZyRE please read the [C4 process](http://rfc.zeromq.org/spec:16), it's what we use.

## Background Story

Some people use the phrase the "Internet of Things", suggesting that we'll see a new category of devices that are more numerous but rather stupider than our current smart phones and tablets and laptops and servers. However, I don't think the data points this way at all. Yes, more and more devices, but they're not stupid at all. They're smart and powerful and getting more so all the time.

The mechanism at work is something I call "Cost Gravity" and it has the effect of cutting the cost of technology by half every 18-24 months. Or, put another way, our global computing capacity doubles every two years, over and over and over. The future is filled with trillions of devices that are fully powerful multi-core computers: they don't run some cut-down "operating system for things" but full operating systems and full applications.

And this is the world we're aiming at with 0MQ. When we talk of "scale" we don't mean hundreds of computers, or even thousands. Think of clouds of tiny smart and perhaps self-replicating machines surrounding every person, filling every space, covering every wall, filling the cracks and eventually, becoming so much a part of us that we get them before birth and they follow us to death.

These clouds of tiny machines talk to each other, all the time, over short-range wireless links, using the Internet Protocol. They create mesh networks, pass information and tasks around like nervous signals. They augment our memory, vision, every aspect of our communications, and physical functions. And it's 0MQ that powers their conversations and events and exchanges of work and information.

Now, to make even a thin imitation of this come true today, we need to solve a set of technical problems (how do peers discover each other, how do they talk to existing networks like the Web, how do they protect the information they carry, how do we track and monitor them, to get some idea of what they're doing). Then we need to do what most engineers forget about: package this solution into a framework that is dead easy for ordinary developers to use.

This is what we'll attempt to build: a framework for distributed applications, as an API, protocols, and implementations. It's not a small challenge but I've claimed often that 0MQ makes such problems simple, so let's see if that's still true.

Whether we're connecting a roomful of mobile devices over WiFi, or a cluster of virtual boxes over simulated Ethernet, we will hit the same kinds of problems. These are:

* *Discovery* - how do we learn about other nodes on the network? Do we use a discovery service, centralized mediation, or some kind of broadcast beacon?

* *Presence* - how do we track when other nodes come and go? Do we use some kind of central registration service, or heartbeating or beacons?

* *Connectivity* - how do we actually connect one node to another? Do we use local networking, wide-area networking, or do we use a central message broker to do the forwarding?

* *Point-to-point messaging* - how do we send a message from one node to another? Do we send this to the node's network address, or do we use some indirect addressing via a centralized message broker?

* *Group messaging* - how do we send a message from one node to a group of others? Do we work via centralized message broker, or do we use a publish-subscribe model like 0MQ?

* *File transfer* - how do we send files from one node to another? Do we use server-centric protocols like FTP or HTTP, or do we use decentralized protocols like FILEMQ?

* *Event synchronization* - how do we ensure that many nodes receive the same unique stream of events? Do we send events via single central point, or do we use a distributed eventually consistent model?

* *Reliability* - how do we recover from network failures that cause messages to be dropped? Do we use a central message server with proper transactions, or do we allow nodes to recover from each other?

* *Security* - how do we protect the confidentiality of information, and make sure people are who they claim to be? Do we use a centralised trust network, or do we use use some kind of distributed key management?

* *Bridging* - how do we connect our networks across the Internet? Do we use the cloud as our central point for all messaging or do we create bridges that join groups to other groups?

* *Logging* - how do we track what this cloud of nodes is doing so we can detect performance problems and failures? Do we create a centralized logging service, or do we allow every device to log the world around it?

* *Simulation* - how do we simulate large numbers of nodes so we can test performance properly? Do we have to buy two dozen Android tablets, or can we use pure software simulation?

If we can solve these dozen problems reasonably well, we get something like a framework for what I might call "Really Cool Distributed Applications", or as my grandkids call it, "the software our world runs on".

-- Pieter Hintjens, "0MQ - The Guide", October 2012