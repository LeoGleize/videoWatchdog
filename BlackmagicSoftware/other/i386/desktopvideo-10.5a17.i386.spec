%define _unpackaged_files_terminate_build 1
%define __find_provides sh -c "/usr/lib/rpm/find-provides | sed -e /libQt5.*/d -e /libq.*\.so/d"
%define __find_requires sh -c "/usr/lib/rpm/find-requires | sed -e /libQt5.*/d -e /libq.*\.so/d"
#
# RPM spec file for Blackmagic Drivers
#
Name:          desktopvideo
Summary:       Blackmagic Design Desktop Video 10.5 - Driver and Firmware Update Utility
Version:       10.5
Release:       a17
License:       Proprietary

Group:         Applications/Multimedia
Vendor:        Blackmagic Design Inc.
Provides:      blackmagic-driver
Conflicts:     DeckLink, Multibridge, Intensity
Packager:      Blackmagic Design Inc. <developer@blackmagicdesign.com>
Url:           http://blackmagicdesign.com
BuildRequires: chrpath
Requires:      dkms, make, gcc
Buildarch:     i386
BuildRoot:     %{_topdir}/BUILDROOT/%{name}-%{version}-%{release}.%{buildarch}

%description
Everything you need to set up your PCIe or thunderbolt Blackmagic DesktopVideo
product.

The DeckLink, Intensity, UltraStudio and Multibridge product lines are
supported.

%debug_package

%install
[ -d "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ]  && rm -rf "$RPM_BUILD_ROOT"
mkdir -p "$RPM_BUILD_ROOT"
tar zxf $RPM_SOURCE_DIR/$RPM_PACKAGE_NAME-$RPM_PACKAGE_VERSION$RPM_PACKAGE_RELEASE-$RPM_ARCH.tar.gz --strip=1 -C "$RPM_BUILD_ROOT"
mkdir -p $RPM_BUILD_ROOT/%{_localstatedir}/lib/rpm-state/desktopvideo/skip-stop-daemons
if [ "%{_lib}" != lib ]; then
    for FILE in $RPM_BUILD_ROOT/usr/src/*/dkms.conf $RPM_BUILD_ROOT/etc/udev/rules.d/55-blackmagic.rules; do
        sed -i 's/\/lib/\/%{_lib}/' $FILE
    done
    chrpath -r /usr/%{_lib}/blackmagic $RPM_BUILD_ROOT/usr/bin/BlackmagicDesktopVideoUtility
    chrpath -r /usr/%{_lib}/blackmagic $RPM_BUILD_ROOT/usr/bin/BlackmagicFirmwareUpdaterGui
    ln -sf ../%{_lib}/blackmagic/BlackmagicDesktopVideoUtility $RPM_BUILD_ROOT/usr/bin/BlackmagicDesktopVideoUtility
    ln -sf ../%{_lib}/blackmagic/BlackmagicFirmwareUpdaterGui $RPM_BUILD_ROOT/usr/bin/BlackmagicFirmwareUpdaterGui
    mkdir -p $RPM_BUILD_ROOT/usr/%{_lib}
    mv $RPM_BUILD_ROOT/usr/lib/blackmagic $RPM_BUILD_ROOT/usr/%{_lib}
    mv $RPM_BUILD_ROOT/usr/lib/lib* $RPM_BUILD_ROOT/usr/%{_lib}
fi

%clean
[ -n "$RPM_BUILD_ROOT" ] || export RPM_BUILD_ROOT=%{_topdir}/BUILDROOT
[ -d "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ]  && rm -rf "$RPM_BUILD_ROOT"

%pre
DKMS_EXTRA_FLAGS=--rpm_safe_upgrade

# Config
VERSION="10.5a17"

DRIVERS="blackmagic blackmagic-io"
DRIVER_MODULES_blackmagic="blackmagic"
DRIVER_MODULES_blackmagic_io="snd_blackmagic_io blackmagic_io"

DAEMONS="DesktopVideoHelper"

MESSAGES="********************************************************************************"
DISPLAY_MESSAGES=0

# Utility functions
log_msg()
{
	local FMT="$1"
	shift

	printf "$FMT" $*
}

log_err()
{
	log_msg $* 1>&1
}

run()
{
	local LABEL="$1"
	local COMMAND="$2"
	local ERROR_CLASS="$3"

	log_msg "  $LABEL"
	OUTPUT="$(eval "$COMMAND" 2>&1)"
	if [ "$?" -ne "0" ]; then
		log_msg " (failed)\n"
		if [ -n "$ERROR_CLASS" ]; then
			eval 'ERRORS_'$ERROR_CLASS'="$ERRORS_'$ERROR_CLASS'
$COMMAND
---
$OUTPUT
"'
		fi
		return 1
	else
		log_msg "\n"
	fi
}

run_for_each_daemon()
{
	CMD="$1"; shift

	for DAEMON in $*; do
		run "$DAEMON" "$CMD" "daemon" || true
	done
}

is_function()
{
	type -t $1 | grep -q "function"
}

has_prog()
{
	[ -x "$(which $1 2> /dev/null)" ]
}

get_modules()
{
	for DRIVER in $*; do
		local DRIVER_UNDERSCORE="$(echo $DRIVER | sed 's/-/_/g')"
		eval 'echo "$DRIVER_MODULES_'$DRIVER_UNDERSCORE'"'
	done
}

detect_init()
{
	if [ -e "/tmp/.resolve_installer" ]; then
		echo "upstart"
	else
		local VERSION_INFO="$(/proc/1/exe --version 2> /dev/null | head -1)"

		if echo "$VERSION_INFO" | grep upstart > /dev/null 2>&1; then
			echo "upstart"
		elif echo "$VERSION_INFO" | grep systemd > /dev/null 2>&1; then
			echo "systemd"
		else
			echo "sysvinit"
		fi
	fi
}

do_service()
{
	OPERATION=$1; shift

	INIT="$(detect_init)"
	FUNC="service_${OPERATION}_${INIT}"

	if is_function $FUNC; then
		$FUNC $*
	fi
}

add_message()
{
	MESSAGES="$MESSAGES
$1
"
	DISPLAY_MESSAGES=1
}

display_messages()
{
	if [ "$DISPLAY_MESSAGES" -eq "1" ]; then
		echo "$MESSAGES********************************************************************************" 1>&2
	fi
}


remove_driver()
{
	local DRIVER=$1
	local DKMS_FLAGS="-m $DRIVER -v $VERSION $DKMS_EXTRA_FLAGS"

	if [ -n "$(dkms status $DKMS_FLAGS)" ]; then
		run "$DRIVER" "dkms remove $DKMS_FLAGS --all" "dkms" || return 1
	fi
}

unload_drivers()
{
	log_msg "Unloading modules...\n"
	for MODULE in $(get_modules $DRIVERS); do
		if [ -n "$(lsmod | grep "^$MODULE ")" ]; then
			local MODULE_VERSION=$MODULE-$VERSION
			run "$MODULE" "rmmod $MODULE" || true
		fi
	done
}

remove_drivers()
{
	if ! has_prog dkms; then
		add_message "Could not find DKMS installed

You will need to uninstall the kernel modules manually
"
		return 0
	fi

	log_msg "Removing old drivers...\n"
	for DRIVER in $DRIVERS; do
		remove_driver $DRIVER || true
	done

	if [ -n "$ERRORS_dkms" ]; then
		add_message "Failed to remove driver(s)

Error messages:$ERRORS_dkms"
	fi
}

service_stop_upstart()
{
	if has_prog initctl; then
		log_msg "Stopping $INIT services...\n"
		run_for_each_daemon "initctl --quiet stop \$DAEMON" $*
	else
		add_message "Unable to find initctl to stop service(s)
"
	fi
}

service_stop_systemd()
{
	if has_prog systemctl; then
		log_msg "Stopping $INIT services...\n"
		run_for_each_daemon "systemctl --system --quiet stop \$DAEMON.service" $*
	else
		add_message "Unable to find systemctl to stop service(s)
"
	fi
}

service_stop_sysvinit()
{
	log_msg "Stopping $INIT services...\n"

	if has_prog invoke-rc.d; then
		run_for_each_daemon "invoke-rc.d \$DAEMON stop" $*
	elif has_prog service; then
		run_for_each_daemon "service \$DAEMON stop" $*
	else
		run_for_each_daemon "/etc/init.d/\$DAEMON stop" $*
	fi
}

service_disable_pre_systemd()
{
	if has_prog systemctl; then
		log_msg "Disabling $INIT services...\n"
		run_for_each_daemon "systemctl --system --quiet disable \$DAEMON.service" $*
	else
		add_message "Unable to find systemctl to disable service(s)
"
	fi
}

service_disable_pre_sysvinit()
{
	if has_prog update-rc.d; then
		true
	elif has_prog chkconfig; then
		log_msg "Deleting $INIT services...\n"
		run_for_each_daemon "chkconfig --del \$DAEMON" $*
	else
		add_message "Unable to find chkconfig or update-rc.d
"
	fi
}

service_disable_post_systemd()
{
	if has_prog systemctl; then
		systemctl --system --quiet daemon-reload > /dev/null 2>&1 || true
	fi
}

service_disable_post_sysvinit()
{
	if has_prog update-rc.d; then
		log_msg "Removing $INIT services...\n"
		run_for_each_daemon "update-rc.d \$DAEMON remove" $*
	fi
}


if [ "$1" -gt 1 ]; then
	do_service stop DesktopVideoHelper
	do_service disable_pre DesktopVideoHelper
	remove_drivers
	unload_drivers
fi

%post
DKMS_EXTRA_FLAGS=--rpm_safe_upgrade

# Config
VERSION="10.5a17"

DRIVERS="blackmagic blackmagic-io"
DRIVER_MODULES_blackmagic="blackmagic"
DRIVER_MODULES_blackmagic_io="snd_blackmagic_io blackmagic_io"

DAEMONS="DesktopVideoHelper"

MESSAGES="********************************************************************************"
DISPLAY_MESSAGES=0

# Utility functions
log_msg()
{
	local FMT="$1"
	shift

	printf "$FMT" $*
}

log_err()
{
	log_msg $* 1>&1
}

run()
{
	local LABEL="$1"
	local COMMAND="$2"
	local ERROR_CLASS="$3"

	log_msg "  $LABEL"
	OUTPUT="$(eval "$COMMAND" 2>&1)"
	if [ "$?" -ne "0" ]; then
		log_msg " (failed)\n"
		if [ -n "$ERROR_CLASS" ]; then
			eval 'ERRORS_'$ERROR_CLASS'="$ERRORS_'$ERROR_CLASS'
$COMMAND
---
$OUTPUT
"'
		fi
		return 1
	else
		log_msg "\n"
	fi
}

run_for_each_daemon()
{
	CMD="$1"; shift

	for DAEMON in $*; do
		run "$DAEMON" "$CMD" "daemon" || true
	done
}

is_function()
{
	type -t $1 | grep -q "function"
}

has_prog()
{
	[ -x "$(which $1 2> /dev/null)" ]
}

get_modules()
{
	for DRIVER in $*; do
		local DRIVER_UNDERSCORE="$(echo $DRIVER | sed 's/-/_/g')"
		eval 'echo "$DRIVER_MODULES_'$DRIVER_UNDERSCORE'"'
	done
}

detect_init()
{
	if [ -e "/tmp/.resolve_installer" ]; then
		echo "upstart"
	else
		local VERSION_INFO="$(/proc/1/exe --version 2> /dev/null | head -1)"

		if echo "$VERSION_INFO" | grep upstart > /dev/null 2>&1; then
			echo "upstart"
		elif echo "$VERSION_INFO" | grep systemd > /dev/null 2>&1; then
			echo "systemd"
		else
			echo "sysvinit"
		fi
	fi
}

do_service()
{
	OPERATION=$1; shift

	INIT="$(detect_init)"
	FUNC="service_${OPERATION}_${INIT}"

	if is_function $FUNC; then
		$FUNC $*
	fi
}

add_message()
{
	MESSAGES="$MESSAGES
$1
"
	DISPLAY_MESSAGES=1
}

display_messages()
{
	if [ "$DISPLAY_MESSAGES" -eq "1" ]; then
		echo "$MESSAGES********************************************************************************" 1>&2
	fi
}


ensure_config_perms()
{
	# Ensure everyone has write permissions to the preferences file
	if [ -e "/etc/blackmagic/BlackmagicPreferences.xml" ]; then
		chmod 666 /etc/blackmagic/BlackmagicPreferences.xml
		chmod 755 /etc/blackmagic
	fi
}

build_driver()
{
	local DRIVER=$1
	local DKMS_FLAGS="-m $DRIVER -v $VERSION $DKMS_EXTRA_FLAGS"

	export SKIP_DKMS_POST_INSTALL=1

	log_msg "Preparing new $DRIVER driver for $(uname -r) kernel...\n"

	run "Adding to DKMS" "dkms add $DKMS_FLAGS" "dkms" || return 1
	run "Building" "dkms build $DKMS_FLAGS" "dkms" || return 1
	run "Installing" "dkms install --force $DKMS_FLAGS" "dkms" || return 1
}

build_drivers()
{
	local DRIVERS_BUILT=""
	local MODULES_TO_LOAD=""
	local REBOOT_REQUIRED=0

	if ! has_prog dkms; then
		add_message "Could not find DKMS installed

You will need to build and install the kernel modules manually. Alternatively,
install dkms and then reinstall the desktopvideo package.
"
		return 0
	fi

	for DRIVER in $DRIVERS; do
		if build_driver $DRIVER; then
			local DRIVERS_BUILT="$DRIVERS_BUILT $DRIVER"
		fi
	done

	log_msg "Loading modules...\n"
	for MODULE in $(get_modules $DRIVERS_BUILT); do
		if [ "$MODULE" != "snd_blackmagic_io" ]; then
			if [ -n "$(lsmod | grep "^$MODULE ")" ]; then
				log_msg "  $MODULE (failed)\n"
				REBOOT_REQUIRED=1
			else
				run "$MODULE" "modprobe $MODULE" || REBOOT_REQUIRED=1
			fi
		fi
	done

	if [ -n "$ERRORS_dkms" ]; then
		add_message "Failed to build driver(s)

Possible causes:
 1. Driver is incompatible with your kernel version
 2. Kernel headers/gcc/make/etc. is not installed
 3. Kernel header version does not match the running kernel ($(uname -r))

Error messages:$ERRORS_dkms
"
	fi

	if [ "$REBOOT_REQUIRED" -eq "1" ]; then
		add_message "Reboot required to reload driver!

The driver failed to load. The most likely cause is that the driver is
in use. Please try rebooting to load the new driver
"
	fi
}

service_enable_systemd()
{
	if has_prog systemctl; then
		systemctl --system --quiet daemon-reload > /dev/null 2>&1 || true
		log_msg "Enabling $INIT services...\n"
		run_for_each_daemon "systemctl --system --quiet enable \$DAEMON.service" $*
	else
		add_message "Unable to find initctl to enable service(s)
"
	fi
}

service_enable_sysvinit()
{
	if has_prog update-rc.d; then
		log_msg "Adding $INIT services...\n"
		run_for_each_daemon "update-rc.d \$DAEMON defaults" $*
	elif has_prog chkconfig; then
		log_msg "Adding $INIT services...\n"
		run_for_each_daemon "chkconfig --add \$DAEMON" $*
	else
		add_message "Unable to find chkconfig or update-rc.d
"
	fi
}

service_start_upstart()
{
	if has_prog initctl; then
		log_msg "Starting $INIT services...\n"
		run_for_each_daemon "initctl --quiet start \$DAEMON" $*
	else
		add_message "Unable to find initctl to start service(s)
"
	fi
}

service_start_systemd()
{
	if has_prog systemctl; then
		log_msg "Starting $INIT services...\n"
		run_for_each_daemon "systemctl --system --quiet start \$DAEMON.service" $*
	else
		add_message "Unable to find systemctl to start service(s)
"
	fi
}

service_start_sysvinit()
{
	log_msg "Starting $INIT services...\n"

	if has_prog invoke-rc.d; then
		run_for_each_daemon "invoke-rc.d \$DAEMON start" $*
	elif has_prog service; then
		run_for_each_daemon "service \$DAEMON start" $*
	else
		run_for_each_daemon "/etc/init.d/\$DAEMON start" $*
	fi
}

check_firmware()
{
	if BlackmagicFirmwareUpdater -gui status | grep -q "^1:.*"; then
		add_message "Software Update Required!

Your blackmagic devices need a software update. Please do this by running the
\"BlackmagicFirmwareUpdater\" command
"
	fi
}


remove_driver()
{
	local DRIVER=$1
	local DKMS_FLAGS="-m $DRIVER -v $VERSION $DKMS_EXTRA_FLAGS"

	if [ -n "$(dkms status $DKMS_FLAGS)" ]; then
		run "$DRIVER" "dkms remove $DKMS_FLAGS --all" "dkms" || return 1
	fi
}

unload_drivers()
{
	log_msg "Unloading modules...\n"
	for MODULE in $(get_modules $DRIVERS); do
		if [ -n "$(lsmod | grep "^$MODULE ")" ]; then
			local MODULE_VERSION=$MODULE-$VERSION
			run "$MODULE" "rmmod $MODULE" || true
		fi
	done
}

remove_drivers()
{
	if ! has_prog dkms; then
		add_message "Could not find DKMS installed

You will need to uninstall the kernel modules manually
"
		return 0
	fi

	log_msg "Removing old drivers...\n"
	for DRIVER in $DRIVERS; do
		remove_driver $DRIVER || true
	done

	if [ -n "$ERRORS_dkms" ]; then
		add_message "Failed to remove driver(s)

Error messages:$ERRORS_dkms"
	fi
}

service_stop_upstart()
{
	if has_prog initctl; then
		log_msg "Stopping $INIT services...\n"
		run_for_each_daemon "initctl --quiet stop \$DAEMON" $*
	else
		add_message "Unable to find initctl to stop service(s)
"
	fi
}

service_stop_systemd()
{
	if has_prog systemctl; then
		log_msg "Stopping $INIT services...\n"
		run_for_each_daemon "systemctl --system --quiet stop \$DAEMON.service" $*
	else
		add_message "Unable to find systemctl to stop service(s)
"
	fi
}

service_stop_sysvinit()
{
	log_msg "Stopping $INIT services...\n"

	if has_prog invoke-rc.d; then
		run_for_each_daemon "invoke-rc.d \$DAEMON stop" $*
	elif has_prog service; then
		run_for_each_daemon "service \$DAEMON stop" $*
	else
		run_for_each_daemon "/etc/init.d/\$DAEMON stop" $*
	fi
}

service_disable_pre_systemd()
{
	if has_prog systemctl; then
		log_msg "Disabling $INIT services...\n"
		run_for_each_daemon "systemctl --system --quiet disable \$DAEMON.service" $*
	else
		add_message "Unable to find systemctl to disable service(s)
"
	fi
}

service_disable_pre_sysvinit()
{
	if has_prog update-rc.d; then
		true
	elif has_prog chkconfig; then
		log_msg "Deleting $INIT services...\n"
		run_for_each_daemon "chkconfig --del \$DAEMON" $*
	else
		add_message "Unable to find chkconfig or update-rc.d
"
	fi
}

service_disable_post_systemd()
{
	if has_prog systemctl; then
		systemctl --system --quiet daemon-reload > /dev/null 2>&1 || true
	fi
}

service_disable_post_sysvinit()
{
	if has_prog update-rc.d; then
		log_msg "Removing $INIT services...\n"
		run_for_each_daemon "update-rc.d \$DAEMON remove" $*
	fi
}


ensure_config_perms
ldconfig
if [ "$1" -gt 1 ]; then
	echo 1 > "%{_localstatedir}/lib/rpm-state/desktopvideo/desktopvideo-10-post.done"
	do_service disable_post DesktopVideoHelper
fi
build_drivers
do_service enable DesktopVideoHelper
do_service start DesktopVideoHelper
check_firmware
display_messages

%preun
DKMS_EXTRA_FLAGS=--rpm_safe_upgrade

# Config
VERSION="10.5a17"

DRIVERS="blackmagic blackmagic-io"
DRIVER_MODULES_blackmagic="blackmagic"
DRIVER_MODULES_blackmagic_io="snd_blackmagic_io blackmagic_io"

DAEMONS="DesktopVideoHelper"

MESSAGES="********************************************************************************"
DISPLAY_MESSAGES=0

# Utility functions
log_msg()
{
	local FMT="$1"
	shift

	printf "$FMT" $*
}

log_err()
{
	log_msg $* 1>&1
}

run()
{
	local LABEL="$1"
	local COMMAND="$2"
	local ERROR_CLASS="$3"

	log_msg "  $LABEL"
	OUTPUT="$(eval "$COMMAND" 2>&1)"
	if [ "$?" -ne "0" ]; then
		log_msg " (failed)\n"
		if [ -n "$ERROR_CLASS" ]; then
			eval 'ERRORS_'$ERROR_CLASS'="$ERRORS_'$ERROR_CLASS'
$COMMAND
---
$OUTPUT
"'
		fi
		return 1
	else
		log_msg "\n"
	fi
}

run_for_each_daemon()
{
	CMD="$1"; shift

	for DAEMON in $*; do
		run "$DAEMON" "$CMD" "daemon" || true
	done
}

is_function()
{
	type -t $1 | grep -q "function"
}

has_prog()
{
	[ -x "$(which $1 2> /dev/null)" ]
}

get_modules()
{
	for DRIVER in $*; do
		local DRIVER_UNDERSCORE="$(echo $DRIVER | sed 's/-/_/g')"
		eval 'echo "$DRIVER_MODULES_'$DRIVER_UNDERSCORE'"'
	done
}

detect_init()
{
	if [ -e "/tmp/.resolve_installer" ]; then
		echo "upstart"
	else
		local VERSION_INFO="$(/proc/1/exe --version 2> /dev/null | head -1)"

		if echo "$VERSION_INFO" | grep upstart > /dev/null 2>&1; then
			echo "upstart"
		elif echo "$VERSION_INFO" | grep systemd > /dev/null 2>&1; then
			echo "systemd"
		else
			echo "sysvinit"
		fi
	fi
}

do_service()
{
	OPERATION=$1; shift

	INIT="$(detect_init)"
	FUNC="service_${OPERATION}_${INIT}"

	if is_function $FUNC; then
		$FUNC $*
	fi
}

add_message()
{
	MESSAGES="$MESSAGES
$1
"
	DISPLAY_MESSAGES=1
}

display_messages()
{
	if [ "$DISPLAY_MESSAGES" -eq "1" ]; then
		echo "$MESSAGES********************************************************************************" 1>&2
	fi
}


remove_driver()
{
	local DRIVER=$1
	local DKMS_FLAGS="-m $DRIVER -v $VERSION $DKMS_EXTRA_FLAGS"

	if [ -n "$(dkms status $DKMS_FLAGS)" ]; then
		run "$DRIVER" "dkms remove $DKMS_FLAGS --all" "dkms" || return 1
	fi
}

unload_drivers()
{
	log_msg "Unloading modules...\n"
	for MODULE in $(get_modules $DRIVERS); do
		if [ -n "$(lsmod | grep "^$MODULE ")" ]; then
			local MODULE_VERSION=$MODULE-$VERSION
			run "$MODULE" "rmmod $MODULE" || true
		fi
	done
}

remove_drivers()
{
	if ! has_prog dkms; then
		add_message "Could not find DKMS installed

You will need to uninstall the kernel modules manually
"
		return 0
	fi

	log_msg "Removing old drivers...\n"
	for DRIVER in $DRIVERS; do
		remove_driver $DRIVER || true
	done

	if [ -n "$ERRORS_dkms" ]; then
		add_message "Failed to remove driver(s)

Error messages:$ERRORS_dkms"
	fi
}

service_stop_upstart()
{
	if has_prog initctl; then
		log_msg "Stopping $INIT services...\n"
		run_for_each_daemon "initctl --quiet stop \$DAEMON" $*
	else
		add_message "Unable to find initctl to stop service(s)
"
	fi
}

service_stop_systemd()
{
	if has_prog systemctl; then
		log_msg "Stopping $INIT services...\n"
		run_for_each_daemon "systemctl --system --quiet stop \$DAEMON.service" $*
	else
		add_message "Unable to find systemctl to stop service(s)
"
	fi
}

service_stop_sysvinit()
{
	log_msg "Stopping $INIT services...\n"

	if has_prog invoke-rc.d; then
		run_for_each_daemon "invoke-rc.d \$DAEMON stop" $*
	elif has_prog service; then
		run_for_each_daemon "service \$DAEMON stop" $*
	else
		run_for_each_daemon "/etc/init.d/\$DAEMON stop" $*
	fi
}

service_disable_pre_systemd()
{
	if has_prog systemctl; then
		log_msg "Disabling $INIT services...\n"
		run_for_each_daemon "systemctl --system --quiet disable \$DAEMON.service" $*
	else
		add_message "Unable to find systemctl to disable service(s)
"
	fi
}

service_disable_pre_sysvinit()
{
	if has_prog update-rc.d; then
		true
	elif has_prog chkconfig; then
		log_msg "Deleting $INIT services...\n"
		run_for_each_daemon "chkconfig --del \$DAEMON" $*
	else
		add_message "Unable to find chkconfig or update-rc.d
"
	fi
}

service_disable_post_systemd()
{
	if has_prog systemctl; then
		systemctl --system --quiet daemon-reload > /dev/null 2>&1 || true
	fi
}

service_disable_post_sysvinit()
{
	if has_prog update-rc.d; then
		log_msg "Removing $INIT services...\n"
		run_for_each_daemon "update-rc.d \$DAEMON remove" $*
	fi
}


if [ ! -f "%{_localstatedir}/lib/rpm-state/desktopvideo/desktopvideo-10-post.done" ]; then
	do_service stop DesktopVideoHelper
	do_service disable_pre DesktopVideoHelper
fi
if [ "$1" -eq 0 ]; then
	unload_drivers
fi
remove_drivers
display_messages

%postun
DKMS_EXTRA_FLAGS=--rpm_safe_upgrade

# Config
VERSION="10.5a17"

DRIVERS="blackmagic blackmagic-io"
DRIVER_MODULES_blackmagic="blackmagic"
DRIVER_MODULES_blackmagic_io="snd_blackmagic_io blackmagic_io"

DAEMONS="DesktopVideoHelper"

MESSAGES="********************************************************************************"
DISPLAY_MESSAGES=0

# Utility functions
log_msg()
{
	local FMT="$1"
	shift

	printf "$FMT" $*
}

log_err()
{
	log_msg $* 1>&1
}

run()
{
	local LABEL="$1"
	local COMMAND="$2"
	local ERROR_CLASS="$3"

	log_msg "  $LABEL"
	OUTPUT="$(eval "$COMMAND" 2>&1)"
	if [ "$?" -ne "0" ]; then
		log_msg " (failed)\n"
		if [ -n "$ERROR_CLASS" ]; then
			eval 'ERRORS_'$ERROR_CLASS'="$ERRORS_'$ERROR_CLASS'
$COMMAND
---
$OUTPUT
"'
		fi
		return 1
	else
		log_msg "\n"
	fi
}

run_for_each_daemon()
{
	CMD="$1"; shift

	for DAEMON in $*; do
		run "$DAEMON" "$CMD" "daemon" || true
	done
}

is_function()
{
	type -t $1 | grep -q "function"
}

has_prog()
{
	[ -x "$(which $1 2> /dev/null)" ]
}

get_modules()
{
	for DRIVER in $*; do
		local DRIVER_UNDERSCORE="$(echo $DRIVER | sed 's/-/_/g')"
		eval 'echo "$DRIVER_MODULES_'$DRIVER_UNDERSCORE'"'
	done
}

detect_init()
{
	if [ -e "/tmp/.resolve_installer" ]; then
		echo "upstart"
	else
		local VERSION_INFO="$(/proc/1/exe --version 2> /dev/null | head -1)"

		if echo "$VERSION_INFO" | grep upstart > /dev/null 2>&1; then
			echo "upstart"
		elif echo "$VERSION_INFO" | grep systemd > /dev/null 2>&1; then
			echo "systemd"
		else
			echo "sysvinit"
		fi
	fi
}

do_service()
{
	OPERATION=$1; shift

	INIT="$(detect_init)"
	FUNC="service_${OPERATION}_${INIT}"

	if is_function $FUNC; then
		$FUNC $*
	fi
}

add_message()
{
	MESSAGES="$MESSAGES
$1
"
	DISPLAY_MESSAGES=1
}

display_messages()
{
	if [ "$DISPLAY_MESSAGES" -eq "1" ]; then
		echo "$MESSAGES********************************************************************************" 1>&2
	fi
}


remove_driver()
{
	local DRIVER=$1
	local DKMS_FLAGS="-m $DRIVER -v $VERSION $DKMS_EXTRA_FLAGS"

	if [ -n "$(dkms status $DKMS_FLAGS)" ]; then
		run "$DRIVER" "dkms remove $DKMS_FLAGS --all" "dkms" || return 1
	fi
}

unload_drivers()
{
	log_msg "Unloading modules...\n"
	for MODULE in $(get_modules $DRIVERS); do
		if [ -n "$(lsmod | grep "^$MODULE ")" ]; then
			local MODULE_VERSION=$MODULE-$VERSION
			run "$MODULE" "rmmod $MODULE" || true
		fi
	done
}

remove_drivers()
{
	if ! has_prog dkms; then
		add_message "Could not find DKMS installed

You will need to uninstall the kernel modules manually
"
		return 0
	fi

	log_msg "Removing old drivers...\n"
	for DRIVER in $DRIVERS; do
		remove_driver $DRIVER || true
	done

	if [ -n "$ERRORS_dkms" ]; then
		add_message "Failed to remove driver(s)

Error messages:$ERRORS_dkms"
	fi
}

service_stop_upstart()
{
	if has_prog initctl; then
		log_msg "Stopping $INIT services...\n"
		run_for_each_daemon "initctl --quiet stop \$DAEMON" $*
	else
		add_message "Unable to find initctl to stop service(s)
"
	fi
}

service_stop_systemd()
{
	if has_prog systemctl; then
		log_msg "Stopping $INIT services...\n"
		run_for_each_daemon "systemctl --system --quiet stop \$DAEMON.service" $*
	else
		add_message "Unable to find systemctl to stop service(s)
"
	fi
}

service_stop_sysvinit()
{
	log_msg "Stopping $INIT services...\n"

	if has_prog invoke-rc.d; then
		run_for_each_daemon "invoke-rc.d \$DAEMON stop" $*
	elif has_prog service; then
		run_for_each_daemon "service \$DAEMON stop" $*
	else
		run_for_each_daemon "/etc/init.d/\$DAEMON stop" $*
	fi
}

service_disable_pre_systemd()
{
	if has_prog systemctl; then
		log_msg "Disabling $INIT services...\n"
		run_for_each_daemon "systemctl --system --quiet disable \$DAEMON.service" $*
	else
		add_message "Unable to find systemctl to disable service(s)
"
	fi
}

service_disable_pre_sysvinit()
{
	if has_prog update-rc.d; then
		true
	elif has_prog chkconfig; then
		log_msg "Deleting $INIT services...\n"
		run_for_each_daemon "chkconfig --del \$DAEMON" $*
	else
		add_message "Unable to find chkconfig or update-rc.d
"
	fi
}

service_disable_post_systemd()
{
	if has_prog systemctl; then
		systemctl --system --quiet daemon-reload > /dev/null 2>&1 || true
	fi
}

service_disable_post_sysvinit()
{
	if has_prog update-rc.d; then
		log_msg "Removing $INIT services...\n"
		run_for_each_daemon "update-rc.d \$DAEMON remove" $*
	fi
}


if [ ! -f "%{_localstatedir}/lib/rpm-state/desktopvideo/desktopvideo-10-post.done" ]; then
	do_service disable_post DesktopVideoHelper
	ldconfig
	display_messages
fi

%posttrans
if [ -f "%{_localstatedir}/lib/rpm-state/desktopvideo/desktopvideo-10-post.done" ]; then
	rm -f "%{_localstatedir}/lib/rpm-state/desktopvideo/desktopvideo-10-post.done"
fi

%files
%defattr(-,root,root,-)
%config(noreplace) /etc/udev/rules.d/55-blackmagic.rules
%dir /etc/blackmagic
%dir %{_localstatedir}/lib/rpm-state/desktopvideo
%{_bindir}/BlackmagicFirmwareUpdater
%{_sbindir}/DesktopVideoHelper
%{_usrsrc}/*-10.5a17/*
%{_libdir}/lib*
%{_libdir}/blackmagic/BlackmagicPreferencesStartup
%{_libdir}/blackmagic/blackmagic-loader
%{_docdir}/desktopvideo
%{_mandir}/man1/BlackmagicFirmwareUpdater.1.gz
%{_mandir}/man1/DesktopVideoHelper.1.gz
%config(noreplace) /etc/init/DesktopVideoHelper.conf
/etc/init.d/DesktopVideoHelper
/usr/lib/systemd/system/DesktopVideoHelper.service


%package gui
Summary:   Blackmagic Design Desktop Video 10.5 - GUI Utilities
Group:     Applications/Multimedia
Requires:  desktopvideo >= 10.5, hicolor-icon-theme
Conflicts: DeckLink, Multibridge, Intensity
Obsoletes: desktopvideo < 10.0

%postun gui
if [ $1 -eq 0 ]; then
	/usr/bin/gtk-update-icon-cache --force --quiet /usr/share/icons/hicolor || :
fi

%posttrans gui
/usr/bin/gtk-update-icon-cache --force --quiet /usr/share/icons/hicolor || :

%files gui
%config(noreplace) /etc/xdg/autostart/*
%{_bindir}/BlackmagicDesktopVideoUtility
%{_bindir}/BlackmagicFirmwareUpdaterGui
%{_datadir}/applications/*
%{_datadir}/icons/hicolor/*/apps/*
%{_libdir}/blackmagic/BlackmagicDesktopVideoUtility
%{_libdir}/blackmagic/BlackmagicFirmwareUpdaterGui
%{_libdir}/blackmagic/libQt5*
%{_libdir}/blackmagic/platforms/*
%{_libdir}/blackmagic/platformthemes/*
%{_mandir}/man1/BlackmagicFirmwareUpdaterGui.1.gz
%{_mandir}/man1/BlackmagicDesktopVideoUtility.1.gz
%{_docdir}/desktopvideo-gui

%description gui
A set of graphical utilities for working with your DesktopVideo product.

It includes the following utilities:
 * BlackmagicDesktopVideoUtility: Allows configuration of your DesktopVideo device
 * BlacmagicFirmwareUpdaterGui: Checks for and performs any firmware updates
   required

%changelog
* Mon Apr 8 2013 Stephen Buck <developer@blackmagicdesign.com>
  Split package into several subpackages
* Mon Jun 15 2009 Ole Henry Halvorsen <developer@blackmagicdesign.com>
  Initial spec file for Blackmagic Linux Drivers.
