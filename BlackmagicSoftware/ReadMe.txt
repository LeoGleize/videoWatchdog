About Blackmagic Desktop Video Software
---------------------------------------

The Desktop Video packages provide capture and playback support for a large
range of Blackmagic Design PCIe and thunderbolt products including the
DeckLink, Intensity and UltraStudio product lines.


What's New in Desktop Video 10.5
----------------------------------

 * Adds support for UltraStudio 4K Extreme and Avid Artist DNxIO.
 * General performance and stability improvements for all models.
 

Minimum System Requirements
---------------------------

Basic system requirements:

 * 32 bit x86 running Linux 2.6.18 or higher
 * 64 bit x86_64 running Linux 2.6.18 or higher

A 64 bit kernel and ample memory is strongly recommended.


Thunderbolt Hotplug Requirements
--------------------------------

Thunderbolt devices cannot be plugged in hot on kernels earlier than 2.6.30.

Supported kernels older than 3.12 require an extra step after plugging the
device in. You must tell the kernel to rescan the PCI bus using the following
command:

# sudo sh -c 'echo 1 > /sys/bus/pci/rescan'

Kernel 3.12 and later support hotplug without the need for any extra steps.


Supported Distributions
-----------------------

This release has been tested on:

 * Ubuntu 10.04 - 14.04
 * Debian 6 - 7
 * Fedora 15 - 20
 * CentOS 5 - 6
 * OpenSUSE 12.3 - 13.1

Several different package formats are supplied:

 * Native package (.deb) for Ubuntu and Debian based distributions.
 * Native package (.rpm) for RedHat, Fedora and OpenSUSE based
   distributions.
 * Tarball and packaging files for custom installation.


Software Dependencies
---------------------

The Desktop Video packages require some additional libraries and packages:

 * dkms
 * Linux kernel headers/source (for currently running kernel version)
 * Qt 4.5 or later

All packages above are included in most modern distributions. Consult your
distributions documentation for more information.

CentOS distributions do not ship with dkms. http://linux.dell.com/dkms/
provides RPMs, DEBs and source packages for dkms. Alternatively, the dkms
package can be installed from the RepoForge/RPMForge repository (See
http://wiki.centos.org/AdditionalResources/Repositories/RPMForge).


CentOS 5 Support
----------------

CentOS 5 ships with Qt 4.2. To use the applications installed by the
gui package or Media Express, you will need to install desktopvideo-qt.


Installing Desktop Video Software
---------------------------------

There are four packages provided that you can install:

 * desktopvideo: This package includes the driver and firmware
   utility. This package is required.

 * desktopvideo-gui: This package includes the graphical control panel used
   to configure the device and a firmware update available notification.

 * desktopvideo-qt: (RPM only) This package provides a copy of Qt 4.5 for use
   with the control panel in the desktopvideo-gui package. This is only needed
   if your distribution does not contain its own version of Qt 4.5 or later.

 * mediaexpress: This package includes Media Express, a simple capture and
   playback utility.

Ubuntu and Debian based distributions

    The deb packages are located in the deb/i386 and deb/amd64
    directories.

    Installation from Graphical User Interface

        Double click on the desktopvideo package. Click the install
        button and follow the prompts. This can then be repeated for
        the desktopvideo-gui and mediaexpress packages if desired.

    Installation from Command Line

        At the command prompt type:

         # sudo dpkg -i desktopvideo_*.deb

        Or, to also install the control panel and Media Express, type:

         # sudo dpkg -i desktopvideo_*.deb desktopvideo-gui_*.deb \
           mediaexpress_*.deb

        Then fix up any missing dependencies with:

         # sudo apt-get install -f

RedHat or Fedora based distributions

    The rpm packages are located in the rpm/i386 and rpm/x86_64
    directories.

    Please apply all system updates before installing the packages.

    Installation from Graphical User Interface

        Double click the desktopvideo package. Click the install
        button and follow the prompts. This can then be repeated for
        the desktopvideo-gui and mediaexpress packages if desired.

    Installation from Command Line

        At the command prompt type:

         # sudo yum install --nogpgcheck desktopvideo-10.1.2*.rpm

        Or, to also install the control panel and Media Express, type:

         # sudo yum install --nogpgcheck desktopvideo-10.1.2*.rpm \
           desktopvideo-gui-*.rpm mediaexpress-*.rpm

        CentOS 5 users will also need to install the desktopvideo-qt package:

         # sudo yum install --nogpgcheck desktopvideo-qt-*.rpm

        If upgrading from an older version of the driver, you will need to
        reboot in order to load the new module.

Creating your own .deb or .rpm packages

    The packaging files used to build the .deb and .rpm packages can be
    found in the other/x86_64 and other/i386 directories along with the
    binary tarballs they use as source. The examples below build the
    64-bit desktopvideo packages.

    Building a set of .deb packages

        To build these packages you need to have debhelper and
        devscripts installed.

         # mkdir desktopvideo-deb
         # tar zxf other/x86_64/desktopvideo_*_amd64.debian.tar.gz \
           -C desktopvideo-deb
         # cp other/x86_64/desktopvideo*-x86_64.tar.gz desktopvideo-deb
         # cd desktopvideo-deb
         # debuild -us -uc -b

    Building a set of .rpm packages

        To build these packages you need to have rpm-build and chrpath
        installed.

         # mkdir desktopvideo-rpm
         # ( cd desktopvideo-rpm && mkdir BUILD BUILDROOT RPMS SRPMS \
           SPECS SOURCES )
         # cp other/x86_64/desktopvideo*.x86_64.spec desktopvideo-rpm/SPECS
         # cp other/x86_64/desktopvideo*-x86_64.tar.gz desktopvideo-rpm/SOURCES
         # cd desktopvideo-rpm
         # rpmbuild --define "_topdir ${PWD}" -bb \
           SPECS/desktopvideo*.x86_64.spec

    The same process can be used to build the 32-bit or Media Express packages.

Other Linux 2.6 based distributions (Experts Only)

    No native package exist to support these distributions at the time of
    writing. It is however, possible to install the driver manually by
    downloading the tar.gz archive.

    Before you begin, ensure that your system satisfies external
    dependencies listed above in "Software dependencies".

    The files are structured in the binary tarball as they should be
    installed. It is suggested that you look at the provided rpm spec file
    or the debian rules file for any assistance required.

    Before getting started, you should take note of the following:

     * The Qt libraries under /usr/lib/blackmagic are not required
       if your distribution provides a native set of Qt 4.5 or
       later libraries.

     * If your system places its 64-bit libraries in lib64, there
       are a number of additional changes required. You can find
       these in the install section of the provided x86_64 spec
       file.

     * If you want to install into a prefix other than /usr, you
       will have to make the same set of changes as above for
       lib64.


Frequently Asked Questions
--------------------------

* How do I check that the driver/device was loaded successfully?

    You can check that your computer find your device by entering the
    following from a terminal:

     # lspci | grep Blackmagic
     02:00.0 Multimedia video controller: Blackmagic Design Device a11b

    You should see entries like the above if the device was recognized by the
    system.

    To test if the driver is loaded properly, type:

     # lsmod | grep blackmagic
     blackmagic                   2082944  1

    If you get no output, that means that the driver is not loaded.

* How do I update the firmware on a device?

    The device's firmware can be updated with the BlackmagicFirmwareUpdater.
    See the man page for instructions and examples on how to use the utility.

* The driver crashed my system

    Look for kernel output messages in dmesg and /var/log/messages.

* The package installed, but the driver was not loaded

    Try the following command:

     # dkms status -m desktopvideo -k `uname -r`
     desktopvideo, 10.1.2, 3.5.0-27-generic, x86_64: <STATUS>

    If the status is 'installed', then the module is installed, but probably
    not loaded. It can be loaded with the following command:

     # sudo modprobe desktopvideo

    If the status is 'added', then the module failed to build. You can issue
    the following command to manually run the build and use the output to
    determine the problem.

     # sudo dkms build -m desktopvideo -v 10.1.2

    A common cause is a version mismatch between the installed kernel image,
    and the kernel source/headers. If they do not match, simply bring either
    the source/headers or the image version up to date, and reboot your system.
    Once the system is back up, the driver should be built for you at startup.

    If the status is 'built', then the module has been built, but not
    installed. You can issue the following command to manually install and use
    the output to determine the problem.

     # sudo dkms install -m desktopvideo -v 10.1.2 --all

* BlackmagicControlPanel exits with a symbol lookup error

    You need to have a version of Qt 4.5 or later installed. This can be be
    remedied by installing the desktopvideo-qt package provided.


Known Issues
------------

* Incompatibility with small memory and/or 32 bit systems

    Sufficient memory must be available to the driver to support the number of
    devices installed and their frame buffering requirements. A 64 bit system
    with adequate memory is strongly recommended. In some configurations,
    reserving more memory at boot for vmalloc may be necessary. More memory
    can be allocated by adding 'vmalloc=<SIZE>' option to the kernel command
    line.

    e.g. vmalloc=256M

* Driver fails to build against certain version of linux-rt

    Some versions of the linux-rt patchset may be incompatible with the
    driver.

* ASPM with some PCI-e cards

    Some PCI-e cards (1x lane cards) do not function properly by default when
    ASPM is enabled. When in this state, the device will be listed in lspci
    and the BlackmagicFirmwareUpdater status will report version ff. To work
    around this please add kernel boot parameter "pcie_aspm=off" to your boot
    loader.


Additional Information
----------------------

Please check www.blackmagicdesign.com for additional information on third
party software compatibility and minimum system requirements.

Some applications may use third party code under license. For details please refer to the included "Third Party Licenses.rtf" document.

© 2015 Blackmagic Design Pty. Ltd. All rights reserved. Blackmagic Design, Blackmagic, DeckLink, Multibridge, HDLink, Videohub, 
and "Leading the creative video revolution" are trademarks of Blackmagic Design Pty. Ltd., registered in the U.S.A and other countries.

Updated September 01, 2015.
