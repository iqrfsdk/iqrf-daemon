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
ARGS.add_argument("-g", "--gen", action="store", dest="gen",
                  required=True, type=str, help="Platform generator.")
ARGS.add_argument("-l", "--lpath", action="store", dest="lpath",
                  required=True, type=str, help="Root directory.")
ARGS.add_argument("-d", "--debug", action="store", dest="debug",
                  default="no", type=str, help="Debug level.")


IQRF_LIBS = ["clibspi", "clibcdc", "cutils", "clibdpa", "paho.mqtt.c"]


def main():
    """
    Main program function
    """
    args = ARGS.parse_args()
    gen = args.gen.lower()
    libs_dir = args.lpath.lower()
    debug = args.debug.lower()

    # install deps
    install_deps()

    # root dir
    ensure_dir(libs_dir)

    # iqrf libs
    for lib in IQRF_LIBS:
        compile_lib(gen, lib, debug)


def ensure_dir(dir):
    """
    Check if root libs folder exists if so enter
    @param dir directory
    """
    if not os.path.exists(dir):
        print("Root folder does not exists, double check download script.")
        exit()
    else:
        os.chdir(dir)


def compile_lib(gen, lib, debug):
    """
    Build the lib, check if folder exists if so enter
    @param gen platform generator
    @param lib directory
    @param debug
    """
    if os.path.exists(lib):
        print("Building " + lib)
        os.chdir(lib)

        if lib != "paho.mqtt.c":
            out = send_command("python3 build.py -g " + gen + " -d " + debug)
            print(out)
        else:
            if not sys.platform.startswith("win"):
                comp_inst_paho()
            else:
                print("Compile and install paho.mqtt.c for Windows!")

        os.chdir("..")
    else:
        print("Lib folder does not exists, double check download script.")


def comp_inst_paho():
    send_command("make")
    send_command("make install")
    send_command("ldconfig")


def install_deps():
    if not sys.platform.startswith("win"):
        send_command("apt-get install cmake libssl-dev")
    else:
        print("Install cmake and libssl-dev for Windows!")


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
