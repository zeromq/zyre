/*  =========================================================================
    zre_uuid - UUID support class

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at
    your option) any later version.

    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program. If not, see
    <http://www.gnu.org/licenses/>.
    =========================================================================
*/

#ifndef __ZRE_UUID_H_INCLUDED__
#define __ZRE_UUID_H_INCLUDED__

#define ZRE_UUID_LEN    16

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zre_uuid_t zre_uuid_t;

//  Constructor
zre_uuid_t *
    zre_uuid_new (void);

//  Destructor
void
    zre_uuid_destroy (zre_uuid_t **self_p);

//  Returns UUID as string
char *
    zre_uuid_str (zre_uuid_t *self);

//  Set UUID to new supplied value 
void
    zre_uuid_set (zre_uuid_t *self, byte *source);
    
//  Store UUID blob in target array
void
    zre_uuid_cpy (zre_uuid_t *self, byte *target);

//  Check if UUID is same as supplied value
bool
    zre_uuid_eq (zre_uuid_t *self, byte *compare);

//  Check if UUID is different from supplied value
bool
    zre_uuid_neq (zre_uuid_t *self, byte *compare);

//  Self test of this class
int
    zre_uuid_test (bool verbose);

#ifdef __cplusplus
}
#endif

#endif
