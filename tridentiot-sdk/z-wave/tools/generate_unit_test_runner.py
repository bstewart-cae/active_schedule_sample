# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

import sys, getopt
import re
from jinja2 import Template
import os
from pathlib import Path

def get_functions(test_file):
    regexp = '^void (test_[A-Za-z0-9_]+)\(.*void\).*'
    funcs = []
    line_number = 0
    with open(test_file, 'r') as file:
        while line := file.readline():
            line_number = line_number + 1
            m = re.search(regexp, line)
            if(m):
                func = {'name': m.group(1).strip(), 'line_number': line_number}
                funcs.append( func )
    return funcs


def print_help():
    print ('generate_runner.py -i <inputfile> -t <templatefile>')


def main(argv):
    inputfile = ''
    templatefile = ''
    output_dir = ''
    opts, args = getopt.getopt(argv,"hi:t:o:",["ifile=","tfile=","output-dir="])
    for opt, arg in opts:
        if opt == '-h':
            print_help()
            sys.exit()
        elif opt in ("-i", "--ifile"):
            inputfile = arg
        elif opt in ("-t", "--tfile"):
            templatefile = arg
        elif opt in ("-o", "--output-folder"):
            output_dir = arg

    if inputfile == '':
        print("Input file is missing.")
        print_help()
        sys.exit()

    if not os.path.isfile(inputfile):
        print("Input file is not a file.")
        print_help()
        sys.exit()

    if templatefile == '':
        print("Template file is missing.")
        print_help()
        sys.exit()

    if not os.path.isfile(templatefile):
        print("Template file is not a file.")
        print_help()
        sys.exit()

    if output_dir == '':
        print("Output directory is missing.")
        print_help()
        sys.exit()

    if not os.path.isdir(output_dir):
        print("Output directory is not a directory.")
        print_help()
        sys.exit()

    print ('Input file is ', inputfile)
    print ('Template file is ', templatefile)
    print ('Output directory is ', output_dir)

    functions = get_functions(inputfile)

    print("Found " + str(len(functions)) + " functions:")
    for function in functions:
        print(function['name'] + ":" + str(function["line_number"]))

    with open(templatefile) as file_:
        template = Template(file_.read())
        output = template.render(functions=functions)

        input_file_name = Path(inputfile).stem
        outputfile = output_dir + "/" + input_file_name + "_runner.c"
        print("Writing output to " + outputfile)
        with open(outputfile, "w") as fh:
            fh.write(output)


if __name__ == "__main__":
   main(sys.argv[1:])
