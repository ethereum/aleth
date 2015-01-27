require=(function e(t,n,r){function s(o,u){if(!n[o]){if(!t[o]){var a=typeof require=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);var f=new Error("Cannot find module '"+o+"'");throw f.code="MODULE_NOT_FOUND",f}var l=n[o]={exports:{}};t[o][0].call(l.exports,function(e){var n=t[o][1][e];return s(n?n:e)},l,l.exports,e,t,n,r)}return n[o].exports}var i=typeof require=="function"&&require;for(var o=0;o<r.length;o++)s(r[o]);return s})({1:[function(require,module,exports){
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
/** @file abi.js
 * @authors:
 *   Marek Kotewicz <marek@ethdev.com>
 *   Gav Wood <g@ethdev.com>
 * @date 2014
 */

// TODO: is these line is supposed to be here? 
if ("build" !== 'build') {/*
    var BigNumber = require('bignumber.js'); // jshint ignore:line
*/}

var web3 = require('./web3'); // jshint ignore:line

BigNumber.config({ ROUNDING_MODE: BigNumber.ROUND_DOWN });

var ETH_PADDING = 32;

/// method signature length in bytes
var ETH_METHOD_SIGNATURE_LENGTH = 4;

/// Finds first index of array element matching pattern
/// @param array
/// @param callback pattern
/// @returns index of element
var findIndex = function (array, callback) {
    var end = false;
    var i = 0;
    for (; i < array.length && !end; i++) {
        end = callback(array[i]);
    }
    return end ? i - 1 : -1;
};

/// @returns a function that is used as a pattern for 'findIndex'
var findMethodIndex = function (json, methodName) {
    return findIndex(json, function (method) {
        return method.name === methodName;
    });
};

/// @param string string to be padded
/// @param number of characters that result string should have
/// @param sign, by default 0
/// @returns right aligned string
var padLeft = function (string, chars, sign) {
    return new Array(chars - string.length + 1).join(sign ? sign : "0") + string;
};

/// @param expected type prefix (string)
/// @returns function which checks if type has matching prefix. if yes, returns true, otherwise false
var prefixedType = function (prefix) {
    return function (type) {
        return type.indexOf(prefix) === 0;
    };
};

/// @param expected type name (string)
/// @returns function which checks if type is matching expected one. if yes, returns true, otherwise false
var namedType = function (name) {
    return function (type) {
        return name === type;
    };
};

var arrayType = function (type) {
    return type.slice(-2) === '[]';
};

/// Formats input value to byte representation of int
/// If value is negative, return it's two's complement
/// If the value is floating point, round it down
/// @returns right-aligned byte representation of int
var formatInputInt = function (value) {
    var padding = ETH_PADDING * 2;
    if (value instanceof BigNumber || typeof value === 'number') {
        if (typeof value === 'number')
            value = new BigNumber(value);
        value = value.round();

        if (value.lessThan(0)) 
            value = new BigNumber("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 16).plus(value).plus(1);
        value = value.toString(16);
    }
    else if (value.indexOf('0x') === 0)
        value = value.substr(2);
    else if (typeof value === 'string')
        value = formatInputInt(new BigNumber(value));
    else
        value = (+value).toString(16);
    return padLeft(value, padding);
};

/// Formats input value to byte representation of string
/// @returns left-algined byte representation of string
var formatInputString = function (value) {
    return web3.fromAscii(value, ETH_PADDING).substr(2);
};

/// Formats input value to byte representation of bool
/// @returns right-aligned byte representation bool
var formatInputBool = function (value) {
    return '000000000000000000000000000000000000000000000000000000000000000' + (value ?  '1' : '0');
};

/// Formats input value to byte representation of real
/// Values are multiplied by 2^m and encoded as integers
/// @returns byte representation of real
var formatInputReal = function (value) {
    return formatInputInt(new BigNumber(value).times(new BigNumber(2).pow(128))); 
};

var dynamicTypeBytes = function (type, value) {
    // TODO: decide what to do with array of strings
    if (arrayType(type) || type === 'string')    // only string itself that is dynamic; stringX is static length.
        return formatInputInt(value.length); 
    return "";
};

/// Setups input formatters for solidity types
/// @returns an array of input formatters 
var setupInputTypes = function () {
    
    return [
        { type: prefixedType('uint'), format: formatInputInt },
        { type: prefixedType('int'), format: formatInputInt },
        { type: prefixedType('hash'), format: formatInputInt },
        { type: prefixedType('string'), format: formatInputString }, 
        { type: prefixedType('real'), format: formatInputReal },
        { type: prefixedType('ureal'), format: formatInputReal },
        { type: namedType('address'), format: formatInputInt },
        { type: namedType('bool'), format: formatInputBool }
    ];
};

var inputTypes = setupInputTypes();

/// Formats input params to bytes
/// @param contract json abi
/// @param name of the method that we want to use
/// @param array of params that will be formatted to bytes
/// @returns bytes representation of input params
var toAbiInput = function (json, methodName, params) {
    var bytes = "";
    var index = findMethodIndex(json, methodName);

    if (index === -1) {
        return;
    }

    var method = json[index];
    var padding = ETH_PADDING * 2;

    /// first we iterate in search for dynamic 
    method.inputs.forEach(function (input, index) {
        bytes += dynamicTypeBytes(input.type, params[index]);
    });

    method.inputs.forEach(function (input, i) {
        var typeMatch = false;
        for (var j = 0; j < inputTypes.length && !typeMatch; j++) {
            typeMatch = inputTypes[j].type(method.inputs[i].type, params[i]);
        }
        if (!typeMatch) {
            console.error('input parser does not support type: ' + method.inputs[i].type);
        }

        var formatter = inputTypes[j - 1].format;
        var toAppend = "";

        if (arrayType(method.inputs[i].type))
            toAppend = params[i].reduce(function (acc, curr) {
                return acc + formatter(curr);
            }, "");
        else
            toAppend = formatter(params[i]);

        bytes += toAppend; 
    });
    return bytes;
};

/// Check if input value is negative
/// @param value is hex format
/// @returns true if it is negative, otherwise false
var signedIsNegative = function (value) {
    return (new BigNumber(value.substr(0, 1), 16).toString(2).substr(0, 1)) === '1';
};

/// Formats input right-aligned input bytes to int
/// @returns right-aligned input bytes formatted to int
var formatOutputInt = function (value) {
    // check if it's negative number
    // it it is, return two's complement
    if (signedIsNegative(value)) {
        return new BigNumber(value, 16).minus(new BigNumber('ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff', 16)).minus(1);
    }
    return new BigNumber(value, 16);
};

/// Formats big right-aligned input bytes to uint
/// @returns right-aligned input bytes formatted to uint
var formatOutputUInt = function (value) {
    return new BigNumber(value, 16);
};

/// @returns input bytes formatted to real
var formatOutputReal = function (value) {
    return formatOutputInt(value).dividedBy(new BigNumber(2).pow(128)); 
};

/// @returns input bytes formatted to ureal
var formatOutputUReal = function (value) {
    return formatOutputUInt(value).dividedBy(new BigNumber(2).pow(128)); 
};

/// @returns right-aligned input bytes formatted to hex
var formatOutputHash = function (value) {
    return "0x" + value;
};

/// @returns right-aligned input bytes formatted to bool
var formatOutputBool = function (value) {
    return value === '0000000000000000000000000000000000000000000000000000000000000001' ? true : false;
};

/// @returns left-aligned input bytes formatted to ascii string
var formatOutputString = function (value) {
    return web3.toAscii(value);
};

/// @returns right-aligned input bytes formatted to address
var formatOutputAddress = function (value) {
    return "0x" + value.slice(value.length - 40, value.length);
};

var dynamicBytesLength = function (type) {
    if (arrayType(type) || type === 'string')   // only string itself that is dynamic; stringX is static length.
        return ETH_PADDING * 2;
    return 0;
};

/// Setups output formaters for solidity types
/// @returns an array of output formatters
var setupOutputTypes = function () {

    return [
        { type: prefixedType('uint'), format: formatOutputUInt },
        { type: prefixedType('int'), format: formatOutputInt },
        { type: prefixedType('hash'), format: formatOutputHash },
        { type: prefixedType('string'), format: formatOutputString },
        { type: prefixedType('real'), format: formatOutputReal },
        { type: prefixedType('ureal'), format: formatOutputUReal },
        { type: namedType('address'), format: formatOutputAddress },
        { type: namedType('bool'), format: formatOutputBool }
    ];
};

var outputTypes = setupOutputTypes();

/// Formats output bytes back to param list
/// @param contract json abi
/// @param name of the method that we want to use
/// @param bytes representtion of output 
/// @returns array of output params 
var fromAbiOutput = function (json, methodName, output) {
    var index = findMethodIndex(json, methodName);

    if (index === -1) {
        return;
    }

    output = output.slice(2);

    var result = [];
    var method = json[index];
    var padding = ETH_PADDING * 2;

    var dynamicPartLength = method.outputs.reduce(function (acc, curr) {
        return acc + dynamicBytesLength(curr.type);
    }, 0);
    
    var dynamicPart = output.slice(0, dynamicPartLength);
    output = output.slice(dynamicPartLength);

    method.outputs.forEach(function (out, i) {
        var typeMatch = false;
        for (var j = 0; j < outputTypes.length && !typeMatch; j++) {
            typeMatch = outputTypes[j].type(method.outputs[i].type);
        }

        if (!typeMatch) {
            console.error('output parser does not support type: ' + method.outputs[i].type);
        }

        var formatter = outputTypes[j - 1].format;
        if (arrayType(method.outputs[i].type)) {
            var size = formatOutputUInt(dynamicPart.slice(0, padding));
            dynamicPart = dynamicPart.slice(padding);
            var array = [];
            for (var k = 0; k < size; k++) {
                array.push(formatter(output.slice(0, padding))); 
                output = output.slice(padding);
            }
            result.push(array);
        }
        else if (prefixedType('string')(method.outputs[i].type)) {
            dynamicPart = dynamicPart.slice(padding); 
            result.push(formatter(output.slice(0, padding)));
            output = output.slice(padding);
        } else {
            result.push(formatter(output.slice(0, padding)));
            output = output.slice(padding);
        }
    });

    return result;
};

/// @returns display name for method eg. multiply(uint256) -> multiply
var methodDisplayName = function (method) {
    var length = method.indexOf('('); 
    return length !== -1 ? method.substr(0, length) : method;
};

/// @returns overloaded part of method's name
var methodTypeName = function (method) {
    /// TODO: make it not vulnerable
    var length = method.indexOf('(');
    return length !== -1 ? method.substr(length + 1, method.length - 1 - (length + 1)) : "";
};

/// @param json abi for contract
/// @returns input parser object for given json abi
var inputParser = function (json) {
    var parser = {};
    json.forEach(function (method) {
        var displayName = methodDisplayName(method.name); 
        var typeName = methodTypeName(method.name);

        var impl = function () {
            var params = Array.prototype.slice.call(arguments);
            return toAbiInput(json, method.name, params);
        };
       
        if (parser[displayName] === undefined) {
            parser[displayName] = impl;
        }

        parser[displayName][typeName] = impl;
    });

    return parser;
};

/// @param json abi for contract
/// @returns output parser for given json abi
var outputParser = function (json) {
    var parser = {};
    json.forEach(function (method) {

        var displayName = methodDisplayName(method.name); 
        var typeName = methodTypeName(method.name);

        var impl = function (output) {
            return fromAbiOutput(json, method.name, output);
        };

        if (parser[displayName] === undefined) {
            parser[displayName] = impl;
        }

        parser[displayName][typeName] = impl;
    });

    return parser;
};

/// @param method name for which we want to get method signature
/// @returns (promise) contract method signature for method with given name
var methodSignature = function (name) {
    return web3.sha3(web3.fromAscii(name)).slice(0, 2 + ETH_METHOD_SIGNATURE_LENGTH * 2);
};

module.exports = {
    inputParser: inputParser,
    outputParser: outputParser,
    methodSignature: methodSignature,
    methodDisplayName: methodDisplayName,
    methodTypeName: methodTypeName
};


},{"./web3":7}],2:[function(require,module,exports){
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
/** @file contract.js
 * @authors:
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2014
 */

var web3 = require('./web3'); // jshint ignore:line
var abi = require('./abi');

/**
 * This method should be called when we want to call / transact some solidity method from javascript
 * it returns an object which has same methods available as solidity contract description
 * usage example: 
 *
 * var abi = [{
 *      name: 'myMethod',
 *      inputs: [{ name: 'a', type: 'string' }],
 *      outputs: [{name: 'd', type: 'string' }]
 * }];  // contract abi
 *
 * var myContract = web3.eth.contract('0x0123123121', abi); // creation of contract object
 *
 * myContract.myMethod('this is test string param for call'); // myMethod call (implicit, default)
 * myContract.call().myMethod('this is test string param for call'); // myMethod call (explicit)
 * myContract.transact().myMethod('this is test string param for transact'); // myMethod transact
 *
 * @param address - address of the contract, which should be called
 * @param desc - abi json description of the contract, which is being created
 * @returns contract object
 */

var contract = function (address, desc) {

    desc.forEach(function (method) {
        // workaround for invalid assumption that method.name is the full anonymous prototype of the method.
        // it's not. it's just the name. the rest of the code assumes it's actually the anonymous
        // prototype, so we make it so as a workaround.
        if (method.name.indexOf('(') === -1) {
            var displayName = method.name;
            var typeName = method.inputs.map(function(i){return i.type; }).join();
            method.name = displayName + '(' + typeName + ')';
        }
    });

    var inputParser = abi.inputParser(desc);
    var outputParser = abi.outputParser(desc);

    var result = {};

    result.call = function (options) {
        result._isTransact = false;
        result._options = options;
        return result;
    };

    result.transact = function (options) {
        result._isTransact = true;
        result._options = options;
        return result;
    };

    result._options = {};
    ['gas', 'gasPrice', 'value', 'from'].forEach(function(p) {
        result[p] = function (v) {
            result._options[p] = v;
            return result;
        };
    });


    desc.forEach(function (method) {

        var displayName = abi.methodDisplayName(method.name);
        var typeName = abi.methodTypeName(method.name);

        var impl = function () {
            var params = Array.prototype.slice.call(arguments);
            var signature = abi.methodSignature(method.name);
            var parsed = inputParser[displayName][typeName].apply(null, params);

            var options = result._options || {};
            options.to = address;
            options.data = signature + parsed;
            
            var isTransact = result._isTransact === true || (result._isTransact !== false && !method.constant);
            var collapse = options.collapse !== false;
            
            // reset
            result._options = {};
            result._isTransact = null;

            if (isTransact) {
                // it's used byt natspec.js
                // TODO: figure out better way to solve this
                web3._currentContractAbi = desc;
                web3._currentContractAddress = address;

                // transactions do not have any output, cause we do not know, when they will be processed
                web3.eth.transact(options);
                return;
            }
            
            var output = web3.eth.call(options);
            var ret = outputParser[displayName][typeName](output);
            if (collapse)
            {
                if (ret.length === 1)
                    ret = ret[0];
                else if (ret.length === 0)
                    ret = null;
            }
            return ret;
        };

        if (result[displayName] === undefined) {
            result[displayName] = impl;
        }

        result[displayName][typeName] = impl;

    });

    return result;
};

module.exports = contract;


},{"./abi":1,"./web3":7}],3:[function(require,module,exports){
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
/** @file filter.js
 * @authors:
 *   Jeffrey Wilcke <jeff@ethdev.com>
 *   Marek Kotewicz <marek@ethdev.com>
 *   Marian Oancea <marian@ethdev.com>
 *   Gav Wood <g@ethdev.com>
 * @date 2014
 */

var web3 = require('./web3'); // jshint ignore:line

/// should be used when we want to watch something
/// it's using inner polling mechanism and is notified about changes
var Filter = function(options, impl) {
    this.impl = impl;
    this.callbacks = [];

    this.id = impl.newFilter(options);
    web3.provider.startPolling({call: impl.changed, args: [this.id]}, this.id, this.trigger.bind(this));
};

/// alias for changed*
Filter.prototype.arrived = function(callback) {
    this.changed(callback);
};

/// gets called when there is new eth/shh message
Filter.prototype.changed = function(callback) {
    this.callbacks.push(callback);
};

/// trigger calling new message from people
Filter.prototype.trigger = function(messages) {
    for (var i = 0; i < this.callbacks.length; i++) {
        for (var j = 0; j < messages; j++) {
            this.callbacks[i].call(this, messages[j]);
        }
    }
};

/// should be called to uninstall current filter
Filter.prototype.uninstall = function() {
    this.impl.uninstallFilter(this.id);
    web3.provider.stopPolling(this.id);
};

/// should be called to manually trigger getting latest messages from the client
Filter.prototype.messages = function() {
    return this.impl.getMessages(this.id);
};

/// alias for messages
Filter.prototype.logs = function () {
    return this.messages();
};

module.exports = Filter;

},{"./web3":7}],4:[function(require,module,exports){
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
/** @file httpsync.js
 * @authors:
 *   Marek Kotewicz <marek@ethdev.com>
 *   Marian Oancea <marian@ethdev.com>
 * @date 2014
 */

if ("build" !== 'build') {/*
        var XMLHttpRequest = require('xmlhttprequest').XMLHttpRequest; // jshint ignore:line
*/}

var HttpSyncProvider = function (host) {
    this.handlers = [];
    this.host = host || 'http://localhost:8080';
};

/// Transforms inner message to proper jsonrpc object
/// @param inner message object
/// @returns jsonrpc object
function formatJsonRpcObject(object) {
    return {
        jsonrpc: '2.0',
        method: object.call,
        params: object.args,
        id: object._id
    };
}

/// Transforms jsonrpc object to inner message
/// @param incoming jsonrpc message 
/// @returns inner message object
function formatJsonRpcMessage(message) {
    var object = JSON.parse(message);

    return {
        _id: object.id,
        data: object.result,
        error: object.error
    };
}

HttpSyncProvider.prototype.send = function (payload) {
    var data = formatJsonRpcObject(payload);
    
    var request = new XMLHttpRequest();
    request.open('POST', this.host, false);
    request.send(JSON.stringify(data));
    
    // check request.status
    return request.responseText;
};

module.exports = HttpSyncProvider;


},{}],5:[function(require,module,exports){
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
/** @file providermanager.js
 * @authors:
 *   Jeffrey Wilcke <jeff@ethdev.com>
 *   Marek Kotewicz <marek@ethdev.com>
 *   Marian Oancea <marian@ethdev.com>
 *   Gav Wood <g@ethdev.com>
 * @date 2014
 */

var web3 = require('./web3'); // jshint ignore:line

/**
 * Provider manager object prototype
 * It's responsible for passing messages to providers
 * If no provider is set it's responsible for queuing requests
 * It's also responsible for polling the ethereum node for incoming messages
 * Default poll timeout is 12 seconds
 * If we are running ethereum.js inside ethereum browser, there are backend based tools responsible for polling,
 * and provider manager polling mechanism is not used
 */
var ProviderManager = function() {
    this.polls = [];
    this.provider = undefined;
    this.id = 1;

    var self = this;
    var poll = function () {
        if (self.provider) {
            self.polls.forEach(function (data) {
                data.data._id = self.id;
                self.id++;
                var result = self.provider.send(data.data);
            
                result = JSON.parse(result);
                
                // dont call the callback if result is not an array, or empty one
                if (result.error || !(result.result instanceof Array) || result.result.length === 0) {
                    return;
                }

                data.callback(result.result);
            });
        }
        setTimeout(poll, 1000);
    };
    poll();
};

/// sends outgoing requests
ProviderManager.prototype.send = function(data) {

    data.args = data.args || [];
    data._id = this.id++;

    if (this.provider === undefined) {
        console.error('provider is not set');
        return null; 
    }

    //TODO: handle error here? 
    var result = this.provider.send(data);
    result = JSON.parse(result);
    return result.result;
};

/// setups provider, which will be used for sending messages
ProviderManager.prototype.set = function(provider) {
    this.provider = provider;
};

/// this method is only used, when we do not have native qt bindings and have to do polling on our own
/// should be callled, on start watching for eth/shh changes
ProviderManager.prototype.startPolling = function (data, pollId, callback) {
    this.polls.push({data: data, id: pollId, callback: callback});
};

/// should be called to stop polling for certain watch changes
ProviderManager.prototype.stopPolling = function (pollId) {
    for (var i = this.polls.length; i--;) {
        var poll = this.polls[i];
        if (poll.id === pollId) {
            this.polls.splice(i, 1);
        }
    }
};

module.exports = ProviderManager;


},{"./web3":7}],6:[function(require,module,exports){
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
/** @file qtsync.js
 * @authors:
 *   Marek Kotewicz <marek@ethdev.com>
 *   Marian Oancea <marian@ethdev.com>
 * @date 2014
 */

var QtSyncProvider = function () {
};

QtSyncProvider.prototype.send = function (payload) {
    return navigator.qt.callMethod(JSON.stringify(payload));
};

module.exports = QtSyncProvider;


},{}],7:[function(require,module,exports){
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
/** @file web3.js
 * @authors:
 *   Jeffrey Wilcke <jeff@ethdev.com>
 *   Marek Kotewicz <marek@ethdev.com>
 *   Marian Oancea <marian@ethdev.com>
 *   Gav Wood <g@ethdev.com>
 * @date 2014
 */

if ("build" !== 'build') {/*
    var BigNumber = require('bignumber.js');
*/}

var ETH_UNITS = [ 
    'wei', 
    'Kwei', 
    'Mwei', 
    'Gwei', 
    'szabo', 
    'finney', 
    'ether', 
    'grand', 
    'Mether', 
    'Gether', 
    'Tether', 
    'Pether', 
    'Eether', 
    'Zether', 
    'Yether', 
    'Nether', 
    'Dether', 
    'Vether', 
    'Uether' 
];

/// @returns an array of objects describing web3 api methods
var web3Methods = function () {
    return [
    { name: 'sha3', call: 'web3_sha3' }
    ];
};

/// @returns an array of objects describing web3.eth api methods
var ethMethods = function () {
    var blockCall = function (args) {
        return typeof args[0] === "string" ? "eth_blockByHash" : "eth_blockByNumber";
    };

    var transactionCall = function (args) {
        return typeof args[0] === "string" ? 'eth_transactionByHash' : 'eth_transactionByNumber';
    };

    var uncleCall = function (args) {
        return typeof args[0] === "string" ? 'eth_uncleByHash' : 'eth_uncleByNumber';
    };

    var methods = [
    { name: 'balanceAt', call: 'eth_balanceAt' },
    { name: 'stateAt', call: 'eth_stateAt' },
    { name: 'storageAt', call: 'eth_storageAt' },
    { name: 'countAt', call: 'eth_countAt'},
    { name: 'codeAt', call: 'eth_codeAt' },
    { name: 'transact', call: 'eth_transact' },
    { name: 'call', call: 'eth_call' },
    { name: 'block', call: blockCall },
    { name: 'transaction', call: transactionCall },
    { name: 'uncle', call: uncleCall },
    { name: 'compilers', call: 'eth_compilers' },
    { name: 'flush', call: 'eth_flush' },
    { name: 'lll', call: 'eth_lll' },
    { name: 'solidity', call: 'eth_solidity' },
    { name: 'serpent', call: 'eth_serpent' },
    { name: 'logs', call: 'eth_logs' }
    ];
    return methods;
};

/// @returns an array of objects describing web3.eth api properties
var ethProperties = function () {
    return [
    { name: 'coinbase', getter: 'eth_coinbase', setter: 'eth_setCoinbase' },
    { name: 'listening', getter: 'eth_listening', setter: 'eth_setListening' },
    { name: 'mining', getter: 'eth_mining', setter: 'eth_setMining' },
    { name: 'gasPrice', getter: 'eth_gasPrice' },
    { name: 'account', getter: 'eth_account' },
    { name: 'accounts', getter: 'eth_accounts' },
    { name: 'peerCount', getter: 'eth_peerCount' },
    { name: 'defaultBlock', getter: 'eth_defaultBlock', setter: 'eth_setDefaultBlock' },
    { name: 'number', getter: 'eth_number'}
    ];
};

/// @returns an array of objects describing web3.db api methods
var dbMethods = function () {
    return [
    { name: 'put', call: 'db_put' },
    { name: 'get', call: 'db_get' },
    { name: 'putString', call: 'db_putString' },
    { name: 'getString', call: 'db_getString' }
    ];
};

/// @returns an array of objects describing web3.shh api methods
var shhMethods = function () {
    return [
    { name: 'post', call: 'shh_post' },
    { name: 'newIdentity', call: 'shh_newIdentity' },
    { name: 'haveIdentity', call: 'shh_haveIdentity' },
    { name: 'newGroup', call: 'shh_newGroup' },
    { name: 'addToGroup', call: 'shh_addToGroup' }
    ];
};

/// @returns an array of objects describing web3.eth.watch api methods
var ethWatchMethods = function () {
    var newFilter = function (args) {
        return typeof args[0] === 'string' ? 'eth_newFilterString' : 'eth_newFilter';
    };

    return [
    { name: 'newFilter', call: newFilter },
    { name: 'uninstallFilter', call: 'eth_uninstallFilter' },
    { name: 'getMessages', call: 'eth_filterLogs' }
    ];
};

/// @returns an array of objects describing web3.shh.watch api methods
var shhWatchMethods = function () {
    return [
    { name: 'newFilter', call: 'shh_newFilter' },
    { name: 'uninstallFilter', call: 'shh_uninstallFilter' },
    { name: 'getMessages', call: 'shh_getMessages' }
    ];
};

/// creates methods in a given object based on method description on input
/// setups api calls for these methods
var setupMethods = function (obj, methods) {
    methods.forEach(function (method) {
        obj[method.name] = function () {
            var args = Array.prototype.slice.call(arguments);
            var call = typeof method.call === 'function' ? method.call(args) : method.call;
            return web3.provider.send({
                call: call,
                args: args
            });
        };
    });
};

/// creates properties in a given object based on properties description on input
/// setups api calls for these properties
var setupProperties = function (obj, properties) {
    properties.forEach(function (property) {
        var proto = {};
        proto.get = function () {
            return web3.provider.send({
                call: property.getter
            });
        };

        if (property.setter) {
            proto.set = function (val) {
                return web3.provider.send({
                    call: property.setter,
                    args: [val]
                });
            };
        }
        Object.defineProperty(obj, property.name, proto);
    });
};

/// setups web3 object, and it's in-browser executed methods
var web3 = {
    _callbacks: {},
    _events: {},
    providers: {},

    toHex: function(str) {
        var hex = "";
        for(var i = 0; i < str.length; i++) {
            var n = str.charCodeAt(i).toString(16);
            hex += n.length < 2 ? '0' + n : n;
        }

        return hex;
    },

    /// @returns ascii string representation of hex value prefixed with 0x
    toAscii: function(hex) {
        // Find termination
        var str = "";
        var i = 0, l = hex.length;
        if (hex.substring(0, 2) === '0x')
            i = 2;
        for(; i < l; i+=2) {
            var code = parseInt(hex.substr(i, 2), 16);
            if(code === 0) {
                break;
            }

            str += String.fromCharCode(code);
        }

        return str;
    },

    /// @returns hex representation (prefixed by 0x) of ascii string
    fromAscii: function(str, pad) {
        pad = pad === undefined ? 0 : pad;
        var hex = this.toHex(str);
        while(hex.length < pad*2)
            hex += "00";
        return "0x" + hex;
    },

    /// @returns decimal representaton of hex value prefixed by 0x
    toDecimal: function (val) {
        return (new BigNumber(val.substring(2), 16).toString(10));
    },

    /// @returns hex representation (prefixed by 0x) of decimal value
    fromDecimal: function (val) {
        return "0x" + (new BigNumber(val).toString(16));
    },

    /// used to transform value/string to eth string
    /// TODO: use BigNumber.js to parse int
    toEth: function(str) {
        var val = typeof str === "string" ? str.indexOf('0x') === 0 ? parseInt(str.substr(2), 16) : parseInt(str) : str;
        var unit = 0;
        var units = ETH_UNITS;
        while (val > 3000 && unit < units.length - 1)
        {
            val /= 1000;
            unit++;
        }
        var s = val.toString().length < val.toFixed(2).length ? val.toString() : val.toFixed(2);
        var replaceFunction = function($0, $1, $2) {
            return $1 + ',' + $2;
        };

        while (true) {
            var o = s;
            s = s.replace(/(\d)(\d\d\d[\.\,])/, replaceFunction);
            if (o === s)
                break;
        }
        return s + ' ' + units[unit];
    },

    /// eth object prototype
    eth: {
        contractFromAbi: function (abi) {
            return function(addr) {
                // Default to address of Config. TODO: rremove prior to genesis.
                addr = addr || '0xc6d9d2cd449a754c494264e1809c50e34d64562b';
                var ret = web3.eth.contract(addr, abi);
                ret.address = addr;
                return ret;
            };
        },
        watch: function (params) {
            return new web3.filter(params, ethWatch);
        }
    },

    /// db object prototype
    db: {},

    /// shh object prototype
    shh: {
        watch: function (params) {
            return new web3.filter(params, shhWatch);
        }
    },

    /// @returns true if provider is installed
    haveProvider: function() {
        return !!web3.provider.provider;
    }
};

/// setups all api methods
setupMethods(web3, web3Methods());
setupMethods(web3.eth, ethMethods());
setupProperties(web3.eth, ethProperties());
setupMethods(web3.db, dbMethods());
setupMethods(web3.shh, shhMethods());

var ethWatch = {
    changed: 'eth_changed'
};

setupMethods(ethWatch, ethWatchMethods());

var shhWatch = {
    changed: 'shh_changed'
};

setupMethods(shhWatch, shhWatchMethods());

web3.setProvider = function(provider) {
    //provider.onmessage = messageHandler; // there will be no async calls, to remove
    web3.provider.set(provider);
};

module.exports = web3;


},{}],"web3":[function(require,module,exports){
var web3 = require('./lib/web3');
var ProviderManager = require('./lib/providermanager');
web3.provider = new ProviderManager();
web3.filter = require('./lib/filter');
web3.providers.HttpSyncProvider = require('./lib/httpsync');
web3.providers.QtSyncProvider = require('./lib/qtsync');
web3.eth.contract = require('./lib/contract');
web3.abi = require('./lib/abi');


module.exports = web3;

},{"./lib/abi":1,"./lib/contract":2,"./lib/filter":3,"./lib/httpsync":4,"./lib/providermanager":5,"./lib/qtsync":6,"./lib/web3":7}]},{},["web3"])


//# sourceMappingURL=ethereum.js.map