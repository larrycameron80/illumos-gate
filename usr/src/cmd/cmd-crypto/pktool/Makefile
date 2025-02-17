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
# Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Copyright (c) 2018, Joyent, Inc.

PROG =	pktool

OBJS =	pktool.o \
	common.o \
	setpin.o \
	list.o \
	delete.o \
	import.o \
	inittoken.o \
	export.o \
	tokens.o \
	gencert.o \
	gencsr.o \
	download.o \
	genkey.o \
	signcsr.o \
	genkeypair.o

include ../../Makefile.cmd

KMFDIR	=	$(SRC)/lib/libkmf
SRCS =	$(OBJS:%.o=%.c)
POFILES = $(OBJS:%.o=%.po)
POFILE = $(PROG)_msg.po
MSGFILES=$(SRCS:%.c=%.i)

CPPFLAGS += -I. -I$(KMFDIR)/include -I$(ADJUNCT_PROTO)/usr/include/libxml2 \
	-I$(KMFDIR)/ber_der/inc

CFLAGS += $(CCVERBOSE) -DDEBUG
CERRWARN += -_gcc=-Wno-uninitialized
CERRWARN += -_gcc=-Wno-unused-label

# not linted, too much mess, but it should be
SMATCH=off

LDLIBS +=  -lkmf -lpkcs11 -lcryptoutil

.KEEP_STATE:

all : $(PROG)

$(PROG) : $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(DYNFLAGS) $(LDLIBS)
	$(POST_PROCESS)

$(POFILE) : $(POFILES)
	$(RM) $@; $(CAT) $(POFILES) > $@

install : all $(ROOTPROG)

clean :
	$(RM) $(OBJS)

lint : lint_SRCS

include ../../Makefile.targ
