#!/usr/bin/python

# Copyright (c) 2022 Silicon Laboratories Inc.
# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

import sys
import re

def skipline(line, matches):
  for m in matches:
    if bool(re.search(m, line, re.I)):
      return True
  return False


with open(sys.argv[1], 'r') as f:
  skippattern = re.compile(r"(skipline)[ \t]*(\w+\\\(\w+\\\))", re.I)
  matches = []
  snippet = None
  for line in f:
    line = line.lstrip()
    skip = skippattern.search(line)

    if snippet != None:
      if snippet == line:
        snippet = None
      sys.stdout.write(line)
    elif line.startswith('## ['):
      sys.stdout.write(line)
      snippet = line
    elif skipline(line, matches):
      sys.stdout.write('\n')
    elif skip:
      matches.append(skip.group(2))
    elif line.startswith('#'):
      sys.stdout.write(line)
    elif bool(re.match('^function', line, re.I)):
      line = line.lower()
      args = re.sub(r"^function[ \t]*\(([\w ]*)\).*", "\\1", line)
      args = re.sub(r"[\r\n]*", "", args)
      args_array = args.split(' ')

      if len(args_array) == 1:
        definition = ''.join("def " + args_array[0] + "():")
        print definition
      else:
        definition = ''.join("def " + args_array[0] + "(" + args_array[1].upper())
        for arg in args_array[2:]:
          definition = definition + ', ' + arg.upper()

	definition = definition + "):\n"
        sys.stdout.write(definition)
    elif bool(re.match('^macro', line, re.I)):
      line = line.lower()
      args = re.sub(r"^macro[ \t]*\(([\w ]*)\).*", "\\1", line)
      args = re.sub(r"[\r\n]*", "", args)
      args_array = args.split(' ')

      if len(args_array) == 1:
        definition = ''.join("def " + args_array[0] + "():")
        print definition
      else:
        definition = ''.join("def " + args_array[0] + "(" + args_array[1].upper())
        for arg in args_array[2:]:
          definition = definition + ', ' + arg.upper()

	definition = definition + "):\n"
        sys.stdout.write(definition)
    else:
#      if line in ['', '\r\n']:
      sys.stdout.write('\n')
#      else:
#        sys.stdout.write('\n')
	
  sys.stdout.flush()

