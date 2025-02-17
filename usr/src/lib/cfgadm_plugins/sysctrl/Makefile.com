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
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Copyright (c) 2018, Joyent, Inc.

include $(SRC)/lib/cfgadm_plugins/Makefile.com

PLATFORM=	sun4u
LIBRARY= sysctrl.a
VERS= .1

OBJECTS= cfga.o

# include library definitions
include $(SRC)/lib/Makefile.lib

USR_PLAT_DIR		= $(ROOT)/usr/platform
USR_PSM_DIR		= $(USR_PLAT_DIR)/sun4u
USR_PSM_LIB_DIR		= $(USR_PSM_DIR)/lib
USR_PSM_LIB_CFG_DIR	= $(USR_PSM_LIB_DIR)/cfgadm
USR_PSM_LIB_CFG_DIR_64	= $(USR_PSM_LIB_CFG_DIR)/$(MACH64)

ROOTLIBDIR=     $(USR_PSM_LIB_CFG_DIR)
ROOTLIBDIR64=   $(USR_PSM_LIB_CFG_DIR_64)

SRCDIR =	../common

LIBS = $(DYNLIB)

CFLAGS +=	$(CCVERBOSE)
LDLIBS +=	-lc

CPPFLAGS +=	-I$(ROOT)/usr/platform/$(PLATFORM)/include

CERRWARN +=	-_gcc=-Wno-switch
CERRWARN +=	-_gcc=-Wno-parentheses
CERRWARN +=	-_gcc=-Wno-uninitialized

# not linted
SMATCH=off

.KEEP_STATE:

all: $(LIBS)

lint:   lintcheck

# Create target directories
$(USR_PSM_DIR):		$(LINKED_DIRS)
	-$(INS.dir)

$(USR_PSM_LIB_DIR):	$(USR_PSM_DIR)	$(LINKED_LIB_DIRS)
	-$(INS.dir)

$(USR_PSM_LIB_CFG_DIR):	$(USR_PSM_LIB_DIR)	$(LINKED_CFG_DIRS)
	-$(INS.dir)

$(USR_PSM_LIB_CFG_DIR_64):	$(USR_PSM_LIB_CFG_DIR)
	-$(INS.dir)

$(USR_PSM_LIB_CFG_DIR)/%: % $(USR_PSM_LIB_CFG_DIR)
	-$(INS.file)

$(USR_PSM_LIB_CFG_DIR_64)/%: % $(USR_PSM_LIB_CFG_DIR_64)
	-$(INS.file)

# include library targets
include $(SRC)/lib/Makefile.targ

pics/%.o: ../common/%.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)
