%module saiut

%define NO_RET_TYPE
%enddef

%include <stdint.i>

%{
#include <sai.h>
#include "saiut.h"
%}

%include ".inc/saitypes.h"
%include ".inc/saistatus.h"
%include ".inc/saiport.h"
%include "saiut.h"
