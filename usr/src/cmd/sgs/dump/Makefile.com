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
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Copyright (c) 2018, Joyent, Inc.
# Copyright 2019 OmniOS Community Edition (OmniOSce) Association.
#

PROG=		dump

include		$(SRC)/cmd/Makefile.cmd
include		$(SRC)/cmd/sgs/Makefile.com

COMOBJS=	dump.o		fcns.o

SRCS=		$(COMOBJS:%.o=../common/%.c)

OBJS =		$(COMOBJS)
.PARALLEL:	$(OBJS)

CPPFLAGS +=	-D__EXTENSIONS__
LLDFLAGS =	'$(LDPASS)-R$$ORIGIN/../../lib'
LLDFLAGS64 =	'$(LDPASS)-R$$ORIGIN/../../../lib/$(MACH64)'
LDFLAGS +=	$(LLDFLAGS)

CERRWARN +=	-_gcc=-Wno-uninitialized

SMOFF += indenting

LDLIBS +=	$(CONVLIBDIR) $(CONV_LIB) $(ELFLIBDIR) -lelf
