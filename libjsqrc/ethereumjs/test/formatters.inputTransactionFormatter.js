var chai = require('chai');
var assert = chai.assert;
var formatters = require('../lib/web3/formatters.js');
var BigNumber = require('bignumber.js');

var tests = [{
    input: {
        data: '0x34234bf23bf4234',
        value: new BigNumber(100),
        from: '0x00000',
        to: '0x00000',
        nonce: 1000,
        gas: 1000,
        gasPrice: new BigNumber(1000)
    },
    result: {
        data: '0x34234bf23bf4234',
        value: '0x64',
        from: '0x00000',
        to: '0x00000',
        nonce: '0x3e8',
        gas: '0x3e8',
        gasPrice: '0x3e8'
    }
},{
    input: {
        data: '0x34234bf23bf4234',
        value: new BigNumber(100),
        from: '0x00000',
        to: '0x00000',
    },
    result: {
        data: '0x34234bf23bf4234',
        value: '0x64',
        from: '0x00000',
        to: '0x00000',
    }
},{
    input: {
        data: '0x34234bf23bf4234',
        value: new BigNumber(100),
        from: '0x00000',
        to: '0x00000',
        gas: '1000',
        gasPrice: new BigNumber(1000)
    },
    result: {
        data: '0x34234bf23bf4234',
        value: '0x64',
        from: '0x00000',
        to: '0x00000',
        gas: '0x3e8',
        gasPrice: '0x3e8'
    }
}];

describe('formatters', function () {
    describe('inputTransactionFormatter', function () {
        tests.forEach(function(test){
            it('should return the correct value', function () {
                assert.deepEqual(formatters.inputTransactionFormatter(test.input), test.result);
            });
        });
    });
});
