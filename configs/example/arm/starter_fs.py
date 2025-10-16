# Copyright (c) 2016-2017, 2020, 2022 Arm Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""This script is the full system example script from the ARM
Research Starter Kit on System Modeling. More information can be found
at: http://www.arm.com/ResearchEnablement/SystemModeling
"""

import argparse
import os

import m5
from m5.objects import *
from m5.options import *
from m5.util import addToPath


m5.util.addToPath("../../")
from common import Options

m5.util.addToPath("../..")

import devices
from common import (
    MemConfig,
    ObjectList,
    SysPaths,
)
from common.cores.arm import (
    HPI,
    O3_ARM_v7a,
)



# default_disk = "/shares/eslfiler1/scratch/gem5_shared/full_system_gem5/disks/arm64-ubuntu-20.04-new-img"
# default_kernel = "vmlinux.5.15.36.arm"
# default_disk = "arm64-ubuntu-20.04-new-img"

default_disk = "arm64-ubuntu-20.04-new-img"
# default_disk = "arm64-ubuntu-20.04-new-img-mod-new"
default_kernel = "vmlinux.5.15.36.arm"
default_root_device = "/dev/vda1"


# Pre-defined CPU configurations. Each tuple must be ordered as : (cpu_class,
# l1_icache_class, l1_dcache_class, l2_Cache_class). Any of
# the cache class may be 'None' if the particular cache is not present.
cpu_types = {
    "atomic": (AtomicSimpleCPU, None, None, None),
    "minor": (MinorCPU, devices.L1I, devices.L1D, devices.L2),
    "hpi": (HPI.HPI, HPI.HPI_ICache, HPI.HPI_DCache, HPI.HPI_L2),
    "o3": (
        O3_ARM_v7a.O3_ARM_v7a_3,
        O3_ARM_v7a.O3_ARM_v7a_ICache,
        O3_ARM_v7a.O3_ARM_v7a_DCache,
        O3_ARM_v7a.O3_ARM_v7aL2,
    ),
}


def create_cow_image(name):
    """Helper function to create a Copy-on-Write disk image"""
    image = CowDiskImage()
    image.child.image_file = SysPaths.disk(name)

    return image


def get_processes(cmd):
    """Interprets commands to run and returns a list of processes"""

    cwd = os.getcwd()
    multiprocesses = []
    for idx, c in enumerate(cmd):
        argv = shlex.split(c)

        process = Process(pid=100 + idx, cwd=cwd, cmd=argv, executable=argv[0])
        process.gid = os.getgid()

        print("info: %d. command and arguments: %s" % (idx + 1, process.cmd))
        multiprocesses.append(process)

    return multiprocesses


def create(args):
    """Create and configure the system object."""

    if args.script and not os.path.isfile(args.script):
        print(f"Error: Bootscript {args.script} does not exist")
        sys.exit(1)

    cpu_class = cpu_types[args.cpu][0]
    mem_mode = cpu_class.memory_mode()
    # Only simulate caches when using a timing CPU (e.g., the HPI model)
    want_caches = True if mem_mode == "timing" else False

    system = devices.SimpleSystem(
        want_caches,
        args.mem_size,
        mem_mode=mem_mode,
        workload=ArmFsLinux(object_file=SysPaths.binary(args.kernel)),
        readfile=args.script,
    )

    MemConfig.config_mem(args, system)

    # Add the PCI devices we need for this system. The base system
    # doesn't have any PCI devices by default since they are assumed
    # to be added by the configuration scripts needing them.
    system.pci_devices = [
        # Create a VirtIO block device for the system's boot
        # disk. Attach the disk image using gem5's Copy-on-Write
        # functionality to avoid writing changes to the stored copy of
        # the disk image.
        PciVirtIO(vio=VirtIOBlock(image=create_cow_image(args.disk_image)))
    ]

    # Attach the PCI devices to the system. The helper method in the
    # system assigns a unique PCI bus ID to each of the devices and
    # connects them to the IO bus.
    for dev in system.pci_devices:
        system.attach_pci(dev)

    # Wire up the system's memory system
    system.connect()

    print("======================")
    print(args.l2_size)
    print("======================")

    # Add CPU clusters to the system
    system.cpu_cluster = [
        devices.ArmCpuCluster(
            system,
            args.num_cores,
            args.cpu_freq,
            "1.0V",
            *cpu_types[args.cpu],
            args.l1d_size,
            args.l2_size,
            args.l2_PF_degree,
            args.l2_PF_conf_bits,
            args.l2_PF_start_conf,
            tarmac_gen=args.tarmac_gen,
            tarmac_dest=args.tarmac_dest,
        )
    ]

    # Create a cache hierarchy for the cluster. We are assuming that
    # clusters have core-private L1 caches and an L2 that's shared
    # within the cluster.
    system.addCaches(want_caches, last_cache_level=2)

    # Setup gem5's minimal Linux boot loader.
    system.realview.setupBootLoader(system, SysPaths.binary)

    if args.dtb:
        system.workload.dtb_filename = args.dtb
    else:
        # No DTB specified: autogenerate DTB
        system.workload.dtb_filename = os.path.join(
            m5.options.outdir, "system.dtb"
        )
        system.generateDtb(system.workload.dtb_filename)

    if args.initrd:
        system.workload.initrd_filename = args.initrd

    # Linux boot command flags
    kernel_cmd = [
        # Tell Linux to use the simulated serial port as a console
        "console=ttyAMA0",
        # Hard-code timi
        "lpj=19988480",
        # Disable address space randomisation to get a consistent
        # memory layout.
        "norandmaps",
        # Tell Linux where to find the root disk image.
        f"root={args.root_device}",
        # Mount the root disk read-write by default.
        "rw",
        # Tell Linux about the amount of physical memory present.
        f"mem={args.mem_size}",
    ]
    system.workload.command_line = " ".join(kernel_cmd)

    if args.with_pmu:
        for cluster in system.cpu_cluster:
            interrupt_numbers = [args.pmu_ppi_number] * len(cluster)
            cluster.addPMUs(interrupt_numbers)

    return system


def run(args):
    cptdir = m5.options.outdir
    if args.checkpoint:
        print(f"Checkpoint directory: {cptdir}")

    while True:
        event = m5.simulate()
        exit_msg = event.getCause()
        if exit_msg == "checkpoint":
            print("Dropping checkpoint at tick %d" % m5.curTick())
            cpt_dir = os.path.join(m5.options.outdir, "cpt.%d" % m5.curTick())
            m5.checkpoint(os.path.join(cpt_dir))
            print("Checkpoint done.")
        else:
            print(f"{exit_msg} ({event.getCode()}) @ {m5.curTick()}")
            break


def arm_ppi_arg(int_num: int) -> int:
    """Argparse argument parser for valid Arm PPI numbers."""
    # PPIs (1056 <= int_num <= 1119) are not yet supported by gem5
    int_num = int(int_num)
    if 16 <= int_num <= 31:
        return int_num
    raise ValueError(f"{int_num} is not a valid Arm PPI number")


def create_and_set_outdir(outdir_path):
    if not os.path.exists(outdir_path):
        os.mkdir(outdir_path)
    m5.options.outdir = outdir_path

def set_terminal_port(r, term_port):
    r.system.terminal = m5.objects.Terminal(port=term_port)


def main():
    parser = argparse.ArgumentParser(epilog=__doc__)

    parser.add_argument(
        "--dtb", type=str, default=None, help="DTB file to load"
    )
    parser.add_argument(
        "--kernel", type=str, default=default_kernel, help="Linux kernel"
    )
    parser.add_argument(
        "--initrd",
        type=str,
        default=None,
        help="initrd/initramfs file to load",
    )
    parser.add_argument(
        "--disk-image",
        type=str,
        default=default_disk,
        help="Disk to instantiate",
    )
    parser.add_argument(
        "--root-device",
        type=str,
        default=default_root_device,
        help=f"OS device name for root partition (default: {default_root_device})",
    )
    parser.add_argument(
        "--script", type=str, default="", help="Linux bootscript"
    )
    parser.add_argument(
        "--cpu",
        type=str,
        choices=list(cpu_types.keys()),
        default="atomic",
        help="CPU model to use",
    )
    parser.add_argument("--cpu-freq", type=str, default="4GHz")
    parser.add_argument(
        "--num-cores", type=int, default=1, help="Number of CPU cores"
    )
    # parser.add_argument(
    #     "--mem-type",
    #     default="DDR3_1600_8x8",
    #     choices=ObjectList.mem_list.get_names(),
    #     help="type of memory to use",
    # )
    # parser.add_argument(
    #     "--mem-channels", type=int, default=1, help="number of memory channels"
    # )
    # parser.add_argument(
    #     "--mem-ranks",
    #     type=int,
    #     default=None,
    #     help="number of memory ranks per channel",
    # )
    # parser.add_argument(
    #     "--mem-size",
    #     action="store",
    #     type=str,
    #     default="2GB",
    #     help="Specify the physical memory size",
    # )
    parser.add_argument(
        "--tarmac-gen",
        action="store_true",
        help="Write a Tarmac trace.",
    )
    parser.add_argument(
        "--tarmac-dest",
        choices=TarmacDump.vals,
        default="stdoutput",
        help="Destination for the Tarmac trace output. [Default: stdoutput]",
    )
    parser.add_argument(
        "--with-pmu",
        action="store_true",
        help="Add a PMU to each core in the cluster.",
    )
    parser.add_argument(
        "--pmu-ppi-number",
        type=arm_ppi_arg,
        default=23,
        help="The number of the PPI to use to connect each PMU to its core. "
        "Must be an integer and a valid PPI number (16 <= int_num <= 31).",
    )
    parser.add_argument("--checkpoint", action="store_true")
    parser.add_argument("--restore", type=str, default=None)

    parser.add_argument(
        "--outdir",
        type=str,
        default=m5.options.outdir,
        help="The output directory where to save the statistics of the simulation.",
    )

    parser.add_argument(
        "--term_port",
        type=int,
        default=3456,
        help="Serial port number to connect to the terminal.",
    )

    parser.add_argument(
        "--l2_PF_degree",
        type=int,
        default=8,
        help="Degree of the L2 stride prefetcher",
    )

    parser.add_argument(
        "--l2_PF_conf_bits",
        type=int,
        default=3,
        help="Number of bits for the confidence counter of the L2 stride prefetcher",
    )

    parser.add_argument(
        "--l2_PF_start_conf",
        type=int,
        default=4,
        help="Initial confidence for the L2 stride prefetcher.",
    )

    parser.add_argument(
        "--sve_vl",
        type=int,
        default=1,
        help="Initial Vector Lenght for the SVE extension.",
    )

    Options.addNoISAOptions(parser)

    args = parser.parse_args()

    create_and_set_outdir(args.outdir)

    
    # print("===================")
    # print("Output directory: {}".format(m5.options.outdir))
    # print("===================")

    root = Root(full_system=True)
    root.system = create(args)

    root.system.sve_vl = args.sve_vl

    if args.term_port is not None:
        set_terminal_port(root, args.term_port)

    # print(root.system._clusters)
    for c in root.system._clusters:
        print(type(c)) 
        print(c.cpus) 
        print(c._l1d_type) 
        print()
    
    if args.cpu == "minor":
        print("-------")
        print(root.system.cpu_cluster)
        print(type(root.system.cpu_cluster[0]), " - ", root.system.cpu_cluster[0])
        print(type(root.system.cpu_cluster[0].cpus[0]), " - ", root.system.cpu_cluster[0].cpus[0])
        print(type(root.system.cpu_cluster[0].cpus[0].dcache), " - ", root.system.cpu_cluster[0].cpus[0].dcache)
        print("L1D cache size: {} B | {} kB".format(root.system.cpu_cluster[0].cpus[0].dcache.size, root.system.cpu_cluster[0].cpus[0].dcache.size / 1024))

        print("OPTION: ", args.l1d_size)


        # root.system.cpu_cluster[0].cpus[0].dcache.size = str(512 *1024)
        print("\n==========================")
        # root.system.sve_vl = 16 
        print("L1D cache size: {} kB".format(root.system.cpu_cluster[0].cpus[0].dcache.size))
        print("L1I assoc = {}".format(root.system.cpu_cluster[0].cpus[0].icache.assoc))
        # print("L1I numSets = {}".format(root.system.cpu_cluster[0].cpus[0].icache.numSets))
        print("L2 cache size: {} kB".format(root.system.cpu_cluster[0].l2.size))
        print("SVE vector size: ", root.system.sve_vl)
        print("Output directory: ", m5.options.outdir)
        print("Terminal port: ", root.system.terminal.port)
        print("==========================")


        print("=================")
        print("L2 Prefetcher: ", root.system.cpu_cluster[0].l2.prefetcher.__class__.__name__)
        print("L2 PF confidence bits: ", root.system.cpu_cluster[0].l2.prefetcher.confidence_counter_bits)
        print("L2 PF init confidence: ", root.system.cpu_cluster[0].l2.prefetcher.initial_confidence)
        print("=================")

    # exit(0)

    # print(">>>>>>>>>>>>>>>..")
    # print(root.system.terminal.port)
    # root.system.terminal = m5.objects.Terminal(port=7777)
    # print(root.system.terminal.port)
    # print(">>>>>>>>>>>>>>>..")

    if args.restore is not None:
        m5.instantiate(args.restore)
    else:
        m5.instantiate()

    run(args)


if __name__ == "__m5_main__":
    # print(m5.options.outdir)
    # m5.options.outdir = "new_outdir"
    # os.mkdir(m5.options.outdir)
    # print(m5.options.outdir)
    # print("END")
    # exit()
    # print(dir())
    main()
