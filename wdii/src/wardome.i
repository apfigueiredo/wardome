%module wardome

%{
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "buffer.h"

%}

%include "comm.h"
%include "structs.h"
%include "interpreter.h"
%include "handler.h"


struct char_data *get_char(char *name);
