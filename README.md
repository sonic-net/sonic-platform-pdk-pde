#SONiC-Object-Library

##Description
The SONiC Object Library (formerly Control Plane Services API or CPS) provides a data-centric API allowing applications to communicate with each other between threads, processes or diverse locations.

The data model of the Object Library is described through Yang or other constructs.

Applications can use Object Library objects using Python, C, C++ and REST services in the sonic-object-library-REST service.


Applications/threads will register for ownership of CPS objects while other applications/threads will operate and receive events of the registered CPS objects.   Applications can also publish objects through the event service.


A high level list of CPS features are:
* Distributed framework for application interaction
* DB-like API (Get, Commit[add,delete,create,modify])
* Publish/subscribe semantics supported

Lookup and binary to text translation and object introspection available


Applications define objects through (optionally yang based) object models.  These object models are converted into binary (C accessable) object keys and object attributes that can be used in conjunction with the C-based CPS APIs.  There are adaptions on top of CPS that allows these objects and APIs to be converted to different languages like Python.

With the object keys and attributes applications can:
* Get a single or get a multiple objects
Perform transactions consisting of:
* Create
* Delete
* Set
* Action
* Register and publish object messages


For more information see the Object Library developers guide.

Build Instructions
=====================
Please see the instructions in the sonic-nas-manifest repo for more details on the common build tools.  [Sonic-nas-manifest](https://github.com/Azure/sonic-nas-manifest)

Development Dependencies 
- sonic-logging
- sonic-common-utils
- libhiredis >= 0.10

Depenedent packages:
  libsonic-logging1 libsonic-logging-dev libsonic-common1 libsonic-common-dev 

BUILD CMD:  sonic_build libsonic-logging1 libsonic-logging-dev libsonic-common1 libsonic-common-dev -- clean binary


(c) Dell 2016
