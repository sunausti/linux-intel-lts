Linux kernel
============

There are several guides for kernel developers and users. These guides can
be rendered in a number of formats, like HTML and PDF. Please read
Documentation/admin-guide/README.rst first.

In order to build the documentation, use ``make htmldocs`` or
``make pdfdocs``.  The formatted documentation can also be read online at:

    https://www.kernel.org/doc/html/latest/

There are various text files in the Documentation/ subdirectory,
several of them using the Restructured Text markup notation.

Please read the Documentation/process/changes.rst file, as it contains the
requirements for building and running the kernel, and information about
the problems which may result by upgrading your kernel.


Build Instruct
=============
In Ubuntu 22.04
```
sudo apt-get install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison lz4
```

generate or copy kernel config to **.config**

```
git clone https://gitlab.com/kernel-firmware/linux-firmware.git firmware
touch firmware/i915/compat.ko
touch firmware/i915/i915_ag.ko
```
need cp dg2 latest guc firmware from internally

```
make menuconfig
make
```
once build pass, please build i915_ag, and cp i915_ag.ko compat.ko from i915 backport module to firmware/i915

```
sudo make modules_install
sudo make install
```
If you are compiling the kernel on Ubuntu, you may receive the following error that interrupts the building process:
```
No rule to make target 'debian/canonical-certs.pem
```
Disable the conflicting security certificates by executing the two commands below:
```
scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS
```
The commands return no output. Start the building process again with **make**, 
and press **Enter** repeatedly to confirm the default options for the generation of new certificates.

