#!/bin/sh -e

pip3 install jinja2

rm -rf gdbus-codegen-glibmm
git clone https://github.com/Pelagicore/gdbus-codegen-glibmm.git

cd gdbus-codegen-glibmm && python3 ./setup.py install
