var assert = require('assert');
var web3 = require('../index.js');
var u = require('./utils.js');

describe('web3', function() {
    describe('eth', function() {
        u.methodExists(web3.eth, 'balanceAt');
        u.methodExists(web3.eth, 'stateAt');
        u.methodExists(web3.eth, 'storageAt');
        u.methodExists(web3.eth, 'countAt');
        u.methodExists(web3.eth, 'codeAt');
        u.methodExists(web3.eth, 'transact');
        u.methodExists(web3.eth, 'call');
        u.methodExists(web3.eth, 'block');
        u.methodExists(web3.eth, 'transaction');
        u.methodExists(web3.eth, 'uncle');
        u.methodExists(web3.eth, 'compilers');
        u.methodExists(web3.eth, 'lll');
        u.methodExists(web3.eth, 'solidity');
        u.methodExists(web3.eth, 'serpent');
        u.methodExists(web3.eth, 'logs');

        u.propertyExists(web3.eth, 'coinbase');
        u.propertyExists(web3.eth, 'listening');
        u.propertyExists(web3.eth, 'mining');
        u.propertyExists(web3.eth, 'gasPrice');
        u.propertyExists(web3.eth, 'account');
        u.propertyExists(web3.eth, 'accounts');
        u.propertyExists(web3.eth, 'peerCount');
        u.propertyExists(web3.eth, 'defaultBlock');
        u.propertyExists(web3.eth, 'number');
    });
});


