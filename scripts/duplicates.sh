#!/usr/bin/env python
# -*- coding: iso-8859-15 -*-

#------------------------------------------------------------------------------
# Python script to analysis cpp-ethereum commits, and filter out duplicates
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# ------------------------------------------------------------------------------
# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2019 Aleth Authors.
# Licensed under the GNU General Public License, Version 3.
#------------------------------------------------------------------------------

import operator
import re

authorRegex = re.compile('Author: (.*) <(.*)>')
dateRegex = re.compile('Date:   (.*)')

authorAliases = {}
authorAliases['arkpar'] = 'Arkadiy Paronyan'
authorAliases['arkady.paronyan@gmail.com'] = 'Arkadiy Paronyan'
authorAliases['Arkady Paronyan'] = 'Arkadiy Paronyan'
authorAliases['artur-zawlocki'] = 'Artur Zawłocki'
authorAliases['Artur Zawlocki'] = 'Artur Zawłocki'
authorAliases['Artur Zawłocki'] = 'Artur Zawłocki'
authorAliases['caktux'] = 'Vincent Gariepy'
authorAliases['chriseth'] = 'Christian Reitwiessner'
authorAliases['Christian'] = 'Christian Reitwiessner'
authorAliases['CJentzsch'] = 'Christoph Jentzsch'
authorAliases['debris'] = 'Marek Kotewicz'
authorAliases['debris-berlin'] = 'Marek Kotewicz'
authorAliases['Dimitry'] = 'Dimitry Khokhlov'
authorAliases['Dmitry K'] = 'Dimitry Khokhlov'
authorAliases['ethdev'] = 'Marek Kotewicz'
authorAliases['gluk256'] = 'Vlad Gluhovsky'
authorAliases['Greg'] = 'Greg Colvin'
authorAliases['Marian OANCΞA'] = 'Marian Oancea'
authorAliases['ethdev zug'] = 'Marek Kotewicz'
authorAliases['Gav Wood'] = 'Gavin Wood'
authorAliases['U-SVZ13\Arkady'] = 'Arkadiy Paronyan'
authorAliases['liana'] = 'Liana Husikyan'
authorAliases['LianaHus'] = 'Liana Husikyan'
authorAliases['subtly'] = 'Alex Leverington'
authorAliases['unknown'] = 'Marek Kotewicz'
authorAliases['vbuterin'] = 'Vitalik Buterin'
authorAliases['winsvega'] = 'Dimitry Khokhlov'
authorAliases['yann300'] = 'Yann Levreau'


commitCounts = {}

commitAlreadySeen = {}

with open('log.txt') as logFile:
    author = ""
    for line in logFile:
        match = authorRegex.match(line)
        if match:
            author = match.group(1)
            if authorAliases.has_key(author):
                author = authorAliases[author]

        match = dateRegex.match(line)
        if match:
            date = match.group(1)
            if commitAlreadySeen.has_key(author + date):
                print "Filtering out .... " + author + " - " + date
            else:
                commitAlreadySeen[author + date] = 1
                if not commitCounts.has_key(author):
                    commitCounts[author] = 1
                else:
                    commitCounts[author] = commitCounts[author] + 1


for key in sorted(commitCounts, key=commitCounts.get):  #sorted(commitCounts.items()):
    print key + " has " + str(commitCounts[key]) + " commits"
