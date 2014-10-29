/*  =========================================================================
    zyre - ZYRE wrapper

    Copyright (c) 1991-2012 iMatix Corporation -- http://www.imatix.com     
    Copyright other contributors as noted in the AUTHORS file.              
                                                                            
    This file is part of Zyre, an open-source framework for proximity-based 
    peer-to-peer applications -- See http://zyre.org.                       
                                                                            
    This is free software; you can redistribute it and/or modify it under   
    the terms of the GNU Lesser General Public License as published by the  
    Free Software Foundation; either version 3 of the License, or (at your  
    option) any later version.                                              
                                                                            
    This software is distributed in the hope that it will be useful, but    
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTA-   
    BILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General  
    Public License for more details.                                        
                                                                            
    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see http://www.gnu.org/licenses/.      

    #################################################################
    #   GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY    #
    #   Please read the README.txt file                             #
    #################################################################
    =========================================================================
*/

#ifndef __ZYRE_H_INCLUDED__
#define __ZYRE_H_INCLUDED__

//  External dependencies
#include <zmq.h>
#include <czmq.h>

//  Set up environment for the application
#include "zyre_prelude.h"

//  ZYRE version macros for compile-time API detection

#define ZYRE_VERSION_MAJOR 1
#define ZYRE_VERSION_MINOR 1
#define ZYRE_VERSION_PATCH 0

#define ZYRE_MAKE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))
#define ZYRE_VERSION \
    ZYRE_MAKE_VERSION((ZYRE_VERSION_MAJOR, ZYRE_VERSION_MINOR, ZYRE_VERSION_PATCH)

//  Opaque class structures to allow forward references
typedef struct _zyre_t zyre_t;
typedef struct _zyre_event_t zyre_event_t;

//  Public API classes

#include "zyre_api.h"
#include "zyre_event.h"

#endif
/*
    #################################################################
    #   GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY    #
    #   Please read the README.txt file in the model directory.     #
    #################################################################
*/    
