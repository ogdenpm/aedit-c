// #SMALL
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/

/* contains dummy replacements for IOC routines
   for the 286, VAX, etc. versions. */
#include <stdbool.h>
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"

byte memory[memory_leng];

void _MAIN() { }


void Cocom(byte ch) { }

void Codat(byte ch) { }

byte Cidat() { return 0; }

void Send_block(pointer string) { }

void Enable_ioc_io() {}

void Disable_ioc_io() {}

void Port_co(byte ch) { }

byte Port_ci() { return 0; }

byte Port_csts() { return 0; }

pointer Unfold_to_screen(pointer line) { return 0; }

