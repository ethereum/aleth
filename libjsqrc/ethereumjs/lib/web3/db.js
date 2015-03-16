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
/** @file db.js
 * @authors:
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */


/// @returns an array of objects describing web3.db api methods
var methods = function () {
    return [
    { name: 'putString', call: 'db_putString'},
    { name: 'getString', call: 'db_getString'},
    { name: 'putHex', call: 'db_putHex'},
    { name: 'getHex', call: 'db_getHex'}
    ];
};

module.exports = {
    methods: methods
};
