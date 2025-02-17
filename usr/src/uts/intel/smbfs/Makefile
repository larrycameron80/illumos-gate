#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
#
# uts/intel/smbfs/Makefile
#
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Copyright (c) 2018, Joyent, Inc.

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		= smbfs
OBJECTS		= $(SMBFS_OBJS:%=$(OBJS_DIR)/%)
LINTS		= $(SMBFS_OBJS:%.o=$(LINTS_DIR)/%.ln)
ROOTMODULE	= $(USR_FS_DIR)/$(MODULE)

#
#	Include common rules.
#
include $(UTSBASE)/intel/Makefile.intel

#
#	Define targets
#
ALL_TARGET	= $(BINARY)
LINT_TARGET	= $(MODULE).lint
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE)

#
#	Overrides.
#
MODSTUBS_DIR	= $(OBJS_DIR)
$(MODSTUBS_O)	:= AS_CPPFLAGS += -DSMBFS_MODULE
CLEANFILES	+= $(MODSTUBS_O)
INC_PATH	+= -I$(UTSBASE)/common/fs/smbclnt
INC_PATH	+= -I$(COMMONBASE)/smbclnt
LDFLAGS         += -dy -Ndrv/nsmb

# Until CR 4994570 is fixed...
LINTTAGS	+= -erroff=E_BAD_FORMAT_ARG_TYPE2
CERRWARN	+= -_gcc=-Wno-parentheses
CERRWARN	+= -_gcc=-Wno-uninitialized

# needs work
SMOFF += all_func_returns,signed,deref_check

# The mb_put/md_get functions are intentionally used with and without
# return value checks, so filter those out like LGREP.2 does.
LGREP.nsmb =	egrep -v ' ignored: (mb_init|mb_put|md_get)'
LTAIL	+=	2>&1 | $(LGREP.2) | $(LGREP.nsmb) || true

#
#	Default build targets.
#
.KEEP_STATE:

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS)

clobber:	$(CLOBBER_DEPS)

lint:		$(LINT_DEPS)

modlintlib:	$(MODLINTLIB_DEPS)

clean.lint:	$(CLEAN_LINT_DEPS)

install:	$(INSTALL_DEPS)

#
#	Include common targets.
#
include $(UTSBASE)/intel/Makefile.targ
