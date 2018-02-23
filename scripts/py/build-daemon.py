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

ARGS = argparse.ArgumentParser(description="Cross-platform dependency downloader.")
ARGS.add_argument("-g", "--gen", action="store", dest="gen",
                  required=True, type=str, help="Platform generator.")
ARGS.add_argument("-l", "--lpath", action="store", dest="lpath",
                  required=True, type=str, help="Root directory.")
ARGS.add_argument("-d", "--debug", action="store", dest="debug",
                  default="no", type=str, help="Debug level.")


Ddir  = os.path.join(os.getcwd(), "..", "..", "daemon")
Adir = os.path.join(os.getcwd(), "..", "..", "applications", "iqrfapp")


def main():
    """
    Main program function
    """
    args = ARGS.parse_args()
    gen = args.gen.lower()
    lpath = args.lpath.lower()
    libs_dir = os.path.abspath(lpath)
    debug = args.debug.lower()

    # libs dir
    ensure_dir(libs_dir)

    # iqrf daemon
    compile(gen, Ddir, libs_dir, debug)

    # iqrf daemon app
    compile(gen, Adir, libs_dir, debug)


def ensure_dir(dir):
    """
    Check if root libs folder exists if so enter
    @param dir directory
    """
    print("Root libs folder " + dir)
    if not os.path.exists(dir):
        print("Root folder does not exists, double check download script.")
        exit()


def compile(gen, dir, ldir, debug):
    """
    Build the iqrf daemon
    @param gen platform generator
    @param dir daemon and app directory
    @param ldir libs directory
    @param debug
    """
    if os.path.exists(dir):
        print("Directory " + dir)
        os.chdir(dir)
        out = send_command("python3 build.py -g " + gen + " -l " + ldir + " -d " + debug)
        print(out)
    else:
        print("Daemon or app folder does not exists!")
        exit()


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
