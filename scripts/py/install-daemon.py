#!/usr/bin/python3
# -*- coding: utf-8 -*-

"""
Copyright 2018 IQRF Tech s.r.o.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

import sys
import subprocess
import os
from pathlib import Path


daemon_dir = os.path.join(os.getcwd(), "..", "..", "daemon")
build_dir = os.path.join(daemon_dir, "build", "Unix_Makefiles")
bin_dir = os.path.join(build_dir, "bin")

service_file = os.path.join("/", "lib", "systemd", "system", "iqrf-daemon.service")


def main():
    """
    Main program function
    """

    # check bin dir
    ensure_dir(bin_dir)

    # check service file
    srv = ensure_srv(service_file)

    # service file is already present
    if srv:
        # stop iqrf-daemon service
        run_service("stop", "iqrf-daemon")

        # iqrf-daemon, iqrfapp, config and service 
        install(bin_dir, srv)

        # restart iqrf-daemon service
        run_service("restart", "iqrf-daemon")
        
        # status iqrf-daemon service
        out = run_service("status", "iqrf-daemon") 
        print(out)

    # service file is missing, install first
    else:
        # iqrf-daemon, iqrfapp, config and service 
        install(bin_dir, srv)

        # enable iqrf-daemon service
        run_service("enable", "iqrf-daemon")

        # restart iqrf-daemon service
        run_service("restart", "iqrf-daemon")

        # status iqrf-daemon service
        out = run_service("status", "iqrf-daemon")
        print(out)


def ensure_dir(dir):
    """
    Check if root libs folder exists if so enter
    @param dir directory
    """
    print("Bin folder " + dir)
    if not os.path.exists(dir):
        print("Bin does not exists, double check download script.")
        exit()


def ensure_srv(file):
    """
    Check if service file exists
    @param file
    @return result
    """
    print("File " + file)

    srv = Path(file)

    if srv.is_file():
      return True
    else:
      return False


def install(dir, srvf):
    """
    Install the iqrf daemon
    @param bin folder
    @param service file exists
    """
    
    os.chdir(dir)
    print("Change to bin folder " + dir)

    daemon_cfg_loc = os.path.join("/", "etc", "iqrf-daemon")
    create_folder(daemon_cfg_loc)
    send_command("cp -a configuration/. " + daemon_cfg_loc)
    send_command("chmod -R 666 " + daemon_cfg_loc)
    send_command("chmod 777 " + daemon_cfg_loc)
    
    if not srvf:
        daemon_srv_loc = os.path.join("/", "lib", "systemd", "system")
        send_command("cp -a service/*.service " + daemon_srv_loc)

    daemon_bin_loc = os.path.join("/" + "usr", "bin")
    send_command("cp -a iqrf* " + daemon_bin_loc)
    send_command("cp -a service/*.sh " + daemon_bin_loc)


def create_folder(directory):
    """
    Create folder
    @param name Name of folder, full path
    """
    if not os.path.exists(directory):
        send_command("mkdir -p " + directory)


def run_service(action, name):
	"""
	Stop systemd service
    @param action Service action 
	@param name Name of service to restart
	"""
	return send_command("systemctl " + action + " " + name + ".service")


def send_command(cmd):
    """
    Execute shell command and return output
    @param cmd Command to exec
    @return string Output
    """
    print(cmd)
    return subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout.read()


if __name__ == "__main__":
    main()
