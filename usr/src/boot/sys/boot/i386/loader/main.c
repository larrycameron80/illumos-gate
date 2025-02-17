/*
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

/*
 * MD bootstrap main() and assorted miscellaneous
 * commands.
 */

#include <stand.h>
#include <stddef.h>
#include <string.h>
#include <machine/bootinfo.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <sys/disk.h>
#include <sys/param.h>
#include <sys/reboot.h>

#include "bootstrap.h"
#include "common/bootargs.h"
#include "libi386/libi386.h"
#include "libi386/smbios.h"
#include "btxv86.h"
#include "libzfs.h"

CTASSERT(sizeof (struct bootargs) == BOOTARGS_SIZE);
CTASSERT(offsetof(struct bootargs, bootinfo) == BA_BOOTINFO);
CTASSERT(offsetof(struct bootargs, bootflags) == BA_BOOTFLAGS);
CTASSERT(offsetof(struct bootinfo, bi_size) == BI_SIZE);

/* Arguments passed in from the boot1/boot2 loader */
static struct bootargs *kargs;

static uint32_t	initial_howto;
static uint32_t	initial_bootdev;
static struct bootinfo	*initial_bootinfo;

struct arch_switch	archsw;		/* MI/MD interface boundary */

static void		extract_currdev(void);
static int		isa_inb(int port);
static void		isa_outb(int port, int value);
void			exit(int code);
static void		i386_zfs_probe(void);

/* XXX debugging */
extern char _end[];

static void *heap_top;
static void *heap_bottom;

int
main(void)
{
	int	i;

	/* Pick up arguments */
	kargs = (void *)__args;
	initial_howto = kargs->howto;
	initial_bootdev = kargs->bootdev;
	initial_bootinfo = kargs->bootinfo ?
	    (struct bootinfo *)PTOV(kargs->bootinfo) : NULL;

	/* Initialize the v86 register set to a known-good state. */
	bzero(&v86, sizeof (v86));
	v86.efl = PSL_RESERVED_DEFAULT | PSL_I;

	/*
	 * Initialise the heap as early as possible.
	 * Once this is done, malloc() is usable.
	 */
	bios_getmem();

	if (high_heap_size > 0) {
		heap_top = PTOV(high_heap_base + high_heap_size);
		heap_bottom = PTOV(high_heap_base);
		if (high_heap_base < memtop_copyin)
			memtop_copyin = high_heap_base;
	} else {
		heap_top = (void *)PTOV(bios_basemem);
		heap_bottom = (void *)_end;
	}
	setheap(heap_bottom, heap_top);

	/*
	 * XXX Chicken-and-egg problem; we want to have console output early,
	 * but some console attributes may depend on reading from eg. the boot
	 * device, which we can't do yet.
	 *
	 * We can use printf() etc. once this is done.
	 * If the previous boot stage has requested a serial console,
	 * prefer that.
	 */
	bi_setboothowto(initial_howto);
	if (initial_howto & RB_MULTIPLE) {
		if (initial_howto & RB_SERIAL)
			setenv("console", "ttya text", 1);
		else
			setenv("console", "text ttya", 1);
	} else if (initial_howto & RB_SERIAL) {
		setenv("console", "ttya", 1);
	} else if (initial_howto & RB_MUTE) {
		setenv("console", "null", 1);
	}
	cons_probe();

	/*
	 * Initialise the block cache. Set the upper limit.
	 */
	bcache_init(32768, 512);

	/*
	 * Special handling for PXE and CD booting.
	 */
	if (kargs->bootinfo == 0) {
		/*
		 * We only want the PXE disk to try to init itself in the below
		 * walk through devsw if we actually booted off of PXE.
		 */
		if (kargs->bootflags & KARGS_FLAGS_PXE)
			pxe_enable(kargs->pxeinfo ?
			    PTOV(kargs->pxeinfo) : NULL);
		else if (kargs->bootflags & KARGS_FLAGS_CD)
			bc_add(initial_bootdev);
	}

	archsw.arch_autoload = i386_autoload;
	archsw.arch_getdev = i386_getdev;
	archsw.arch_copyin = i386_copyin;
	archsw.arch_copyout = i386_copyout;
	archsw.arch_readin = i386_readin;
	archsw.arch_isainb = isa_inb;
	archsw.arch_isaoutb = isa_outb;
	archsw.arch_loadaddr = i386_loadaddr;
	archsw.arch_zfs_probe = i386_zfs_probe;

	/*
	 * March through the device switch probing for things.
	 */
	for (i = 0; devsw[i] != NULL; i++)
		if (devsw[i]->dv_init != NULL)
			(devsw[i]->dv_init)();

	printf("BIOS %dkB/%dkB available memory\n", bios_basemem / 1024,
	    bios_extmem / 1024);
	if (initial_bootinfo != NULL) {
		initial_bootinfo->bi_basemem = bios_basemem / 1024;
		initial_bootinfo->bi_extmem = bios_extmem / 1024;
	}

	/* detect ACPI for future reference */
	biosacpi_detect();

	/* detect SMBIOS for future reference */
	smbios_detect(NULL);

	/* detect PCI BIOS for future reference */
	biospci_detect();

	printf("\n%s", bootprog_info);

	extract_currdev();		/* set $currdev and $loaddev */
	autoload_font();		/* Set up the font list for console. */

	bi_isadir();
	bios_getsmap();

	interact(NULL);

	/* if we ever get here, it is an error */
	return (1);
}

/*
 * Set the 'current device' by (if possible) recovering the boot device as
 * supplied by the initial bootstrap.
 *
 * XXX should be extended for netbooting.
 */
static void
extract_currdev(void)
{
	struct i386_devdesc	new_currdev;
	struct zfs_boot_args	*zargs;
	int			biosdev = -1;

	/* Assume we are booting from a BIOS disk by default */
	new_currdev.dd.d_dev = &bioshd;

	/* new-style boot loaders such as pxeldr and cdldr */
	if (kargs->bootinfo == 0) {
		if ((kargs->bootflags & KARGS_FLAGS_CD) != 0) {
			/* we are booting from a CD with cdboot */
			new_currdev.dd.d_dev = &bioscd;
			new_currdev.dd.d_unit = bd_bios2unit(initial_bootdev);
		} else if ((kargs->bootflags & KARGS_FLAGS_PXE) != 0) {
			/* we are booting from pxeldr */
			new_currdev.dd.d_dev = &pxedisk;
			new_currdev.dd.d_unit = 0;
		} else {
			/* we don't know what our boot device is */
			new_currdev.d_kind.biosdisk.slice = -1;
			new_currdev.d_kind.biosdisk.partition = 0;
			biosdev = -1;
		}
	} else if ((kargs->bootflags & KARGS_FLAGS_ZFS) != 0) {
		zargs = NULL;
		/* check for new style extended argument */
		if ((kargs->bootflags & KARGS_FLAGS_EXTARG) != 0)
			zargs = (struct zfs_boot_args *)(kargs + 1);

		if (zargs != NULL &&
		    zargs->size >=
		    offsetof(struct zfs_boot_args, primary_pool)) {
			/* sufficient data is provided */
			new_currdev.d_kind.zfs.pool_guid = zargs->pool;
			new_currdev.d_kind.zfs.root_guid = zargs->root;
		} else {
			/* old style zfsboot block */
			new_currdev.d_kind.zfs.pool_guid = kargs->zfspool;
			new_currdev.d_kind.zfs.root_guid = 0;
		}
		new_currdev.dd.d_dev = &zfs_dev;
	} else if ((initial_bootdev & B_MAGICMASK) != B_DEVMAGIC) {
		/* The passed-in boot device is bad */
		new_currdev.d_kind.biosdisk.slice = -1;
		new_currdev.d_kind.biosdisk.partition = 0;
		biosdev = -1;
	} else {
		new_currdev.d_kind.biosdisk.slice =
		    B_SLICE(initial_bootdev) - 1;
		new_currdev.d_kind.biosdisk.partition =
		    B_PARTITION(initial_bootdev);
		biosdev = initial_bootinfo->bi_bios_dev;

		/*
		 * If we are booted by an old bootstrap, we have to guess at
		 * the BIOS unit number. We will lose if there is more than
		 * one disk type and we are not booting from the
		 * lowest-numbered disk type (ie. SCSI when IDE also exists).
		 */
		if ((biosdev == 0) && (B_TYPE(initial_bootdev) != 2)) {
			/*
			 * biosdev doesn't match major, assume harddisk
			 */
			biosdev = 0x80 + B_UNIT(initial_bootdev);
		}
	}

	/*
	 * If we are booting off of a BIOS disk and we didn't succeed
	 * in determining which one we booted off of, just use disk0:
	 * as a reasonable default.
	 */
	if ((new_currdev.dd.d_dev->dv_type == bioshd.dv_type) &&
	    ((new_currdev.dd.d_unit = bd_bios2unit(biosdev)) == -1)) {
		printf("Can't work out which disk we are booting "
		    "from.\nGuessed BIOS device 0x%x not found by "
		    "probes, defaulting to disk0:\n", biosdev);
		new_currdev.dd.d_unit = 0;
	}

	env_setenv("currdev", EV_VOLATILE, i386_fmtdev(&new_currdev),
	    i386_setcurrdev, env_nounset);
	env_setenv("loaddev", EV_VOLATILE, i386_fmtdev(&new_currdev),
	    env_noset, env_nounset);
}

COMMAND_SET(reboot, "reboot", "reboot the system", command_reboot);

static int
command_reboot(int argc, char *argv[])
{
	int i;

	for (i = 0; devsw[i] != NULL; ++i)
		if (devsw[i]->dv_cleanup != NULL)
			(devsw[i]->dv_cleanup)();

	printf("Rebooting...\n");
	delay(1000000);
	__exit(0);
}

/* provide this for panic, as it's not in the startup code */
void
exit(int code)
{
	__exit(code);
}

COMMAND_SET(heap, "heap", "show heap usage", command_heap);

static int
command_heap(int argc, char *argv[])
{

	mallocstats();
	printf("heap base at %p, top at %p, upper limit at %p\n", heap_bottom,
	    sbrk(0), heap_top);
	return (CMD_OK);
}

/* ISA bus access functions for PnP. */
static int
isa_inb(int port)
{

	return (inb(port));
}

static void
isa_outb(int port, int value)
{

	outb(port, value);
}

static void
i386_zfs_probe(void)
{
	char devname[32];
	struct i386_devdesc dev;

	/*
	 * Open all the disks we can find and see if we can reconstruct
	 * ZFS pools from them.
	 */
	dev.dd.d_dev = &bioshd;
	for (dev.dd.d_unit = 0; bd_unit2bios(&dev) >= 0; dev.dd.d_unit++) {
		snprintf(devname, sizeof (devname), "%s%d:", bioshd.dv_name,
		    dev.dd.d_unit);
		zfs_probe_dev(devname, NULL);
	}
}
