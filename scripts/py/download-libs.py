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

import argparse
import sys
import subprocess
import os
import errno
import datetime

ARGS = argparse.ArgumentParser(description="Cross-platform dependency downloader.")
ARGS.add_argument("-l", "--lpath", action="store", dest="lpath",
                  required=True, type=str, help="Dependency downloader.")

IQRF_LIBS = ["clibspi", "clibcdc", "cutils", "clibdpa", "paho.mqtt.c"]


def main():
    """
    Main program function
    """
    args = ARGS.parse_args()
    libs_dir = args.lpath.lower()

    ensure_dir(libs_dir)
    os.chdir(libs_dir)

    for lib in IQRF_LIBS:
        get_lib(lib)


def ensure_dir(dir):
    """
    Check an libs folder and create it if not exists
    @param libs directory
    """
    if not os.path.exists(dir):
        try:
            os.makedirs(dir)
        except OSError as exception:
            if exception.errno != errno.EEXIST:
                raise


def get_lib(lib):
    """
    Check an lib folder and pull it if not exists
    @param lib directory
    """
    if not os.path.exists(lib):
        if not lib == "paho.mqtt.c":
            send_command("git clone https://github.com/iqrfsdk/" + lib + ".git")
            if lib == "clibdpa":
                send_command("git checkout release/v1.1.0")    
        else:
            send_command("git clone https://github.com/eclipse/" + lib + ".git")
            os.chdir(lib)
            send_command("git checkout v1.2.0")
            os.chdir("..")
    else:
        os.chdir(lib)
        send_command("git pull origin")
        os.chdir("..")


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
