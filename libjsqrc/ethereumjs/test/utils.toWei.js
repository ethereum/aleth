var chai = require('chai');
var utils = require('../lib/utils/utils');
var assert = chai.assert;

describe('lib/utils/utils', function () {
    describe('toWei', function () {
        it('should return the correct value', function () {
            
            assert.equal(utils.toWei(1, 'wei'),    '1');
            assert.equal(utils.toWei(1, 'kwei'),   '1000');
            assert.equal(utils.toWei(1, 'mwei'),   '1000000');
            assert.equal(utils.toWei(1, 'gwei'),   '1000000000');
            assert.equal(utils.toWei(1, 'szabo'),  '1000000000000');
            assert.equal(utils.toWei(1, 'finney'), '1000000000000000');
            assert.equal(utils.toWei(1, 'ether'),  '1000000000000000000');
            assert.equal(utils.toWei(1, 'kether'), '1000000000000000000000');
            assert.equal(utils.toWei(1, 'grand'),  '1000000000000000000000');
            assert.equal(utils.toWei(1, 'mether'), '1000000000000000000000000');
            assert.equal(utils.toWei(1, 'gether'), '1000000000000000000000000000');
            assert.equal(utils.toWei(1, 'tether'), '1000000000000000000000000000000');

            assert.throws(function () {utils.toWei(1, 'wei1');}, Error);
        });
    });
});
