#!/usr/bin/env python
# -*- coding: iso-8859-15 -*-

#------------------------------------------------------------------------------
# Python script to analysis cpp-ethereum commits, and filter out duplicates
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# ------------------------------------------------------------------------------
# This file is part of cpp-ethereum.
#
# cpp-ethereum is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cpp-ethereum is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>
#
# (c) 2016 cpp-ethereum contributors.
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
