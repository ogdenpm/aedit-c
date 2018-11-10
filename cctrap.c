// #COMPACT(EXPORTS cc_trap, excep_handler)
/*********************************************************************
*         INTEL CORPORATION PROPRIETARY INFORMATION                  *
*   This software is supplied under the terms of a license agreement *
*   or nondisclosure agreement with Intel Corporation and may not be *
*   copied or disclosed except in accordance with the terms of that  *
*   agreement.                                                       *
*********************************************************************/

#include <stdbool.h>
#include "lit.h"
#include "type.h"
#include "data.h"
#include "proc.h"
#include <signal.h>


void Cc_trap(int sig) {

    signal(SIGINT, Cc_trap);       // re-enable handler
    have_control_c = 0xff;

} /* cc_trap */


void Excep_handler(word condition_code, word param_code, word reserved, word npx_status) {

    return;

} /* excep_handler */



