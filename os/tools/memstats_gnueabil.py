#!/usr/bin/env python
###########################################################################
#
# Copyright 2016 Samsung Electronics All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
###########################################################################

from __future__ import print_function
from optparse import OptionParser
import sys
import re
if sys.version_info[0] < 3:
    from sets import Set

parser = OptionParser()
parser.add_option("-f", "--file", dest="infilename",
                  help="map FILE needed to be parsed", metavar="INPUT_FILE")
parser.add_option("-o", "--output", dest="output",
                  help="Output written to this file. Default is stdout.", metavar="OUTPUT_FILE")
parser.add_option("-a", "--all", action="store_true", dest="all",
                  help="Print all information.", default=False)
parser.add_option("-s", "--size", action="store_true", dest="totsize",
                  help="Print total size of data segments.", default=False)
parser.add_option("-l", "--libsize", action="store_true",
                  dest="libsize", help="Print Library Sizes.", default=False)
parser.add_option("-d", "--details", action="store_true", dest="details",
                  help="Print details of Object Sizes.", default=False)
#Example: memstats.py <option {-a / -s / -l / -d}> -f tinyara.map -o memory.txt.

(options, args) = parser.parse_args()
if not options.infilename:
    parser.print_help()
    sys.exit(1)

if (not options.all) and (not options.totsize) and (not options.libsize) and (not options.details):
    parser.print_help()
    sys.exit(1)

infile = open(options.infilename, 'r')
if options.output:
    sys.stdout = open(options.output, 'w')
first = 0
currentSymbol = ""
level1 = {}
action = 0
subSymbol = 0
libObject = ""
librarySize = {}
objectSize = {}
allLibraries = {}
CATEGORY_KEYS = [".text", ".data", ".bss"]
Required = dict((key, []) for key in CATEGORY_KEYS)

# GNU ARM ELF images can place loadable read-only content in sections that are
# not literally named ".text". Group those sections with text so the reported
# total lines up better with the linked binary footprint.
TEXT_SECTION_MARKERS = (
    ".text",
    ".rodata",
    ".srodata",
    ".data.rel.ro",
    ".init_array",
    ".fini_array",
    ".preinit_array",
    ".ctors",
    ".dtors",
    ".eh_frame",
    ".eh_frame_hdr",
    ".gcc_except_table",
    ".glue_7",
    ".glue_7t",
    ".ARM.extab",
    ".ARM.exidx",
    ".gnu.linkonce.t",
    ".gnu.linkonce.r"
)

DATA_SECTION_MARKERS = (
    ".data",
    ".sdata",
    ".tdata",
    ".got",
    ".got.plt",
    ".igot.plt",
    ".gnu.linkonce.d"
)

BSS_SECTION_MARKERS = (
    ".bss",
    ".sbss",
    ".tbss",
    ".gnu.linkonce.b",
    "COMMON"
)

allmap = {}


def transform(k):
    i = 0
    while(i < len(k)):
        if k[i] == '*':
            k = k[:i] + '.'+k[i:]
            i = i+1
        i = i+1
    k = '^' + k + '$'
    return k


def isValidSubSymbol(field1, dictKeys):
    for k in dictKeys:
        tk = transform(k)
        if re.search(tk, field1) != None:
            return k
    return None


def containsSectionMarker(sectionName, markers):
    for marker in markers:
        if sectionName.find(marker) != -1:
            return True
    return False


def findParent(sectionName):
    if containsSectionMarker(sectionName, TEXT_SECTION_MARKERS):
        return ".text"
    if containsSectionMarker(sectionName, DATA_SECTION_MARKERS):
        return ".data"
    if containsSectionMarker(sectionName, BSS_SECTION_MARKERS):
        return ".bss"
    return None


def addRequiredSection(sectionName):
    parent = findParent(sectionName)
    if parent != None and sectionName not in Required[parent]:
        Required[parent].append(sectionName)
    return parent


def getListfromString(s):
    res = []
    s = s[s.find('(')+1:s.rfind(')')]
    word = ''
    for x in s:
        if (x == '(' or x == ')' or x == ' '):
            if word != '':
                res.append(word)
            word = ''
        else:
            word = word + x
    if word != '':
        res.append(word)
    return res


def PLO(size, libObj, currSym, subSym):
    currSymParent = addRequiredSection(currSym)
    if currSymParent == None:
        return

    libObj = libObj.strip().split('/')
    libObj = libObj[len(libObj)-1]
    if '(' in libObj:
        library = libObj.split('(')[0]
        dotO = libObj.split('(')[1][:-1]
    else:
        library = "NOLIB"
        dotO = libObj

    if library in list(librarySize.keys()):
        librarySize[library] += size
    else:
        librarySize[library] = size

    if dotO in list(objectSize.keys()):
        objectSize[dotO] += size
    else:
        objectSize[dotO] = size

    if library in list(allmap.keys()):
        if dotO in list(allmap[library].keys()):
            if currSymParent in list(allmap[library][dotO].keys()):
                allmap[library][dotO][currSymParent] += size
            else:
                allmap[library][dotO][currSymParent] = size
        else:
            allmap[library][dotO] = {currSymParent: size}
    else:
        allmap[library] = {dotO: {currSymParent: size}}


def printLibrarySizes():
    nfields = len(CATEGORY_KEYS)
    for r in CATEGORY_KEYS:
        print("\t" + r, end=' ')
    print("\t Total")
    for l in list(allmap.keys()):
        sizearr = [0] * nfields
        for o in allmap[l]:
            for s in list(allmap[l][o].keys()):
                sizearr[CATEGORY_KEYS.index(s)] += allmap[l][o][s]

        for r in CATEGORY_KEYS:
            print("\t" + str(sizearr[CATEGORY_KEYS.index(r)]), end=' ')
        print("\t" + str(librarySize[l]) + "\t" + l)


def sortByTotal(n):
    return int(n[len(n)-2])


def sortPrintLibrarySizes():
    results = []
    nfields = len(CATEGORY_KEYS)
    for r in CATEGORY_KEYS:
        print("\t" + r, end=' ')
    print("\t Total")
    for l in list(allmap.keys()):
        sizearr = [0] * nfields
        for o in list(allmap[l].keys()):
            for s in list(allmap[l][o].keys()):
                sizearr[CATEGORY_KEYS.index(s)] += allmap[l][o][s]
        one_result = []
        for r in CATEGORY_KEYS:
            one_result.append(str(sizearr[CATEGORY_KEYS.index(r)]))
        one_result.append(str(librarySize[l]))
        one_result.append(l)
        results.append(one_result)

    results.sort(key=sortByTotal, reverse=True)
    for r in results:
        for i in range(0, len(r) - 1):
            print("\t" + r[i].strip(), end=' ')
        print("\t" + r[len(r) - 1].strip())


def sortPrintall():
    results = []
    nfields = len(CATEGORY_KEYS)
    for l in list(allmap.keys()):
        print(l + "\t" + str(librarySize[l]))
        for r in CATEGORY_KEYS:
            print("\t" + r, end=' ')
        print("\t Total")
        for o in list(allmap[l].keys()):
            sizearr = [0] * nfields
            for s in CATEGORY_KEYS:
                if s in list(allmap[l][o].keys()):
                    sizearr[CATEGORY_KEYS.index(s)] = allmap[l][o][s]

            one_result = []
            for r in CATEGORY_KEYS:
                one_result.append(str(sizearr[CATEGORY_KEYS.index(r)]))
            one_result.append(str(objectSize[o]))
            one_result.append(o)
            results.append(one_result)

        results.sort(key=sortByTotal, reverse=True)
        for r in results:
            for i in range(0, len(r) - 1):
                print("\t" + r[i].strip(), end=' ')
            print("\t" + r[len(r) - 1].strip())
        results = []


def printall():
    nfields = len(CATEGORY_KEYS)
    for l in list(allmap.keys()):
        print(l + "\t" + str(librarySize[l]))
        for r in CATEGORY_KEYS:
            print("\t" + r, end=' ')
        print("\t Total")
        for o in list(allmap[l].keys()):
            sizearr = [0] * nfields
            for s in CATEGORY_KEYS:
                if s in list(allmap[l][o].keys()):
                    sizearr[CATEGORY_KEYS.index(s)] = allmap[l][o][s]

            for r in CATEGORY_KEYS:
                print(
                    "\t" + str(sizearr[CATEGORY_KEYS.index(r)]), end=' ')
            print("\t" + str(objectSize[o]) + "\t" + o)


def printTotalSize():
    ksSize = [0] * len(CATEGORY_KEYS)
    for ks in list(level1.keys()):
        parent = findParent(ks)
        if parent != None:
            for s in level1[ks]:
                ksSize[CATEGORY_KEYS.index(parent)] += level1[ks][s]

    for r in CATEGORY_KEYS:
        print("\t"+r, end=' ')
    print("")
    for r in ksSize:
        print("\t"+str(r), end=' ')
    print("\n")


for line in infile:
    if line not in ['\n', '\r\n']:
        # Ignore section before "Linker script and memory map" and after "/DISCARD/"
        if line.strip() == "Linker script and memory map":
            action = 1
            continue
        if line.strip() == "/DISCARD/":
            action = 0

        # Ignore section before "Linker script and memory map" and after "/DISCARD/"
        if action == 0:
            continue

        # Ignore the first line of Linker script and Memory map
        if re.search("__ebase = ORIGIN", line) != None:
            continue

        lsplit = line.split()
        if (line[0] == '.'):
            currentSymbol = lsplit[0]
            subSymbol = 0
            if currentSymbol == ".comment":
                action = 0
            level1[currentSymbol] = {}
            continue
        else:
            if re.search('\*\(.*\)', line.strip()) != None or re.search('\**.o\(', lsplit[0]) != None:
                level2string = getListfromString(line.strip())
                for l2strs in level2string:
                    if l2strs in list(level1[currentSymbol].keys()):
                        continue
                    else:
                        level1[currentSymbol][l2strs] = 0

                continue
            else:
                if currentSymbol == '':
                    continue
                matchedkey = isValidSubSymbol(
                    lsplit[0], list(level1[currentSymbol].keys()))
                if matchedkey != None:
                    subSymbol = matchedkey
                    if len(lsplit) > 2:
                        try:
                            if int(lsplit[1], 16) != 0:
                                size_value = int(lsplit[2], 16)
                                level1[currentSymbol][subSymbol] += size_value
                                libObject = lsplit[len(lsplit)-1]
                                PLO(size_value, libObject,
                                    currentSymbol, subSymbol)
                        except ValueError:
                            # lsplit[1] or lsplit[2] is not a hex number, skip this line
                            continue
                    else:
                        line = next(infile)
                        line = line.strip()
                        lsplit = line.split()
                        try:
                            if int(lsplit[0], 16) != 0:
                                # Check if lsplit[1] is a valid hex number before converting
                                try:
                                    size_value = int(lsplit[1], 16)
                                    level1[currentSymbol][subSymbol] += size_value
                                    libObject = lsplit[len(lsplit)-1]
                                    PLO(size_value, libObject,
                                        currentSymbol, subSymbol)
                                except ValueError:
                                    # lsplit[1] is not a hex number, skip this line
                                    continue
                        except ValueError:
                            # lsplit[0] is not a hex number, skip this line
                            continue
                elif re.search('\*fill\*', lsplit[0]) != None:
                    try:
                        if int(lsplit[1], 16) != 0 and subSymbol != 0:
                            size_value = int(lsplit[2], 16)
                            level1[currentSymbol][subSymbol] += size_value
                            PLO(size_value, libObject,
                                currentSymbol, subSymbol)
                    except ValueError:
                        # lsplit[1] or lsplit[2] is not a hex number, skip this line
                        continue
infile.close()
if options.all:
    options.totsize = options.libsize = options.details = True
if options.totsize:
    print("######################################")
    print("##           Total sizes            ##")
    print("######################################")
    printTotalSize()

    print("######################################")
    print("##      Sizes Calculated Using:     ##")
    print("######################################")
    for r in CATEGORY_KEYS:
        print(r+":\n\t", end=' ')
        t = 1
        for s in Required[r]:
            if t % 4 == 0:
                print(s + "\t\n\t", end=' ')
            else:
                print(s+"\t", end=' ')
            t += 1
        print("")

if options.libsize:
    print("######################################")
    print("##          Library Sizes           ##")
    print("######################################")
    #printLibrarySizes()
    sortPrintLibrarySizes()
if options.details:
    print("######################################")
    print("##              Details             ##")
    print("######################################")
    #printall()
    sortPrintall()
    print("######################################\n")
