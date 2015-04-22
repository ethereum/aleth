/*
    This file is part of ethereum.js.

    ethereum.js is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ethereum.js is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with ethereum.js.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file event.js
 * @authors:
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2014
 */

var abi = require('../solidity/abi');
var utils = require('../utils/utils');
var signature = require('./signature');

/// filter inputs array && returns only indexed (or not) inputs
/// @param inputs array
/// @param bool if result should be an array of indexed params on not
/// @returns array of (not?) indexed params
var filterInputs = function (inputs, indexed) {
    return inputs.filter(function (current) {
        return current.indexed === indexed;
    });
};

var inputWithName = function (inputs, name) {
    var index = utils.findIndex(inputs, function (input) {
        return input.name === name;
    });
    
    if (index === -1) {
        console.error('indexed param with name ' + name + ' not found');
        return undefined;
    }
    return inputs[index];
};

var indexedParamsToTopics = function (event, indexed) {
    // sort keys?
    return Object.keys(indexed).map(function (key) {
        var inputs = [inputWithName(filterInputs(event.inputs, true), key)];

        var value = indexed[key];
        if (value instanceof Array) {
            return value.map(function (v) {
                return abi.formatInput(inputs, [v]);
            }); 
        }
        return '0x' + abi.formatInput(inputs, [value]);
    });
};

var inputParser = function (address, sign, event) {
    
    // valid options are 'earliest', 'latest', 'offset' and 'max', as defined for 'eth.filter'
    return function (indexed, options) {
        var o = options || {};
        o.address = address;
        o.topics = [];
        o.topics.push(sign);
        if (indexed) {
            o.topics = o.topics.concat(indexedParamsToTopics(event, indexed));
        }
        return o;
    };
};

var getArgumentsObject = function (inputs, indexed, notIndexed) {
    var indexedCopy = indexed.slice();
    var notIndexedCopy = notIndexed.slice();
    return inputs.reduce(function (acc, current) {
        var value;
        if (current.indexed)
            value = indexedCopy.splice(0, 1)[0];
        else
            value = notIndexedCopy.splice(0, 1)[0];

        acc[current.name] = value;
        return acc;
    }, {}); 
};
 
var outputParser = function (event) {
    
    return function (output) {
        var result = {
            event: utils.extractDisplayName(event.name),
            number: output.number,
            hash: output.hash,
            args: {}
        };

        if (!output.topics) {
            return result;
        }
        output.data = output.data || '';
       
        var indexedOutputs = filterInputs(event.inputs, true);
        var indexedData = "0x" + output.topics.slice(1, output.topics.length).map(function (topics) { return topics.slice(2); }).join("");
        var indexedRes = abi.formatOutput(indexedOutputs, indexedData);

        var notIndexedOutputs = filterInputs(event.inputs, false);
        var notIndexedRes = abi.formatOutput(notIndexedOutputs, output.data);

        result.args = getArgumentsObject(event.inputs, indexedRes, notIndexedRes);

        return result;
    };
};

var getMatchingEvent = function (events, payload) {
    for (var i = 0; i < events.length; i++) {
        var sign = signature.eventSignatureFromAscii(events[i].name); 
        if (sign === payload.topics[0]) {
            return events[i];
        }
    }
    return undefined;
};


module.exports = {
    inputParser: inputParser,
    outputParser: outputParser,
    getMatchingEvent: getMatchingEvent
};

