==========================
Generating Consensus Tests
==========================

Consensus Tests
===============

Consensus tests are test cases for all Ethereum implementations.
The test cases are distributed in the "filled" form, which contains, for example, the expected state root hash after transactions.
The filled test cases are usually not written by hand, but generated from "test filler" files.
``testeth`` executable in cpp-ethereum can convert test fillers into test cases.

When you add a test case in the consensus test suite, you are supposed to push both the filler and the filled test cases into the `tests repository`_.

.. _`tests repository`: https://github.com/ethereum/tests

Generating a GeneralStateTest Case
==================================

Designing a Test Case
---------------------

For creating a new GeneralStateTest case, you need:

* environmental parameters
* a transaction
* a state before the transaction (pre-state)
* some expectations about the state after the transaction

For an idea, peek into `an existing test filler`_ under ``src/GeneralStateFiller`` in the tests repository.

.. _`an existing test filler`: https://github.com/ethereum/tests/blob/develop/src/GeneralStateTestsFiller/stExample/add11Filler.json

Usually, when a test is about an instruction, the pre-state contains a contract with
a code containing the instruction.  Typically, the contract stores a value in the storage,
so that the instruction's behavior is visible in the storage in the expectation.

The code can be written in EVM bytecode or in LLL [#]_.

.. [#] ``testeth`` cannot understand LLL if the system does not have the LLL compiler installed.  The LLL compiler is currently distributed as part of `the Solidity compiler`_.

.. _`the Solidity compiler`: https://github.com/ethereum/solidity

Writing a Test Filler
---------------------

A new test filler needs to be alone in a new test filler file.  A single GeneralStateTest filler file is not supposed to contain multiple tests.  ``testeth`` tool still accepts multiple GeneralStateTest fillers in a single test filler file, but this might change.

In the ``tests`` repository, the test filler files for GeneralStateTests live under ``src/GeneralStateTestsFiller`` directory.
The directory has many subdirectories.  You need to choose one of the subdirectories or create one.  The name of the filler file needs to end with ``Filler.json``.  For example, we might want to create a new directory ``src/GeneralStateTestsFiller/stReturnDataTest`` [#]_ with a new filler file ``returndatacopy_initialFiller.json``.

.. [#] If you create a new directory here, you need to add `one line`__ in ``cpp-ethereum`` and file that change in a pull-request to ``cpp-ethereum``.

__ editcpp_

The easiest way to start is to copy an existing filler file.  The first thing to change is the name of the test in the beginning of the file. The name of the test should coincide with the file name except ``Filler.json`` [#]_. For example, in the file we created above, the filler file contains the name of the test ``returndatacopy_initial``.  The overall structure of ``returndatacopy_initialFiller.json`` should be:

.. code::

   {
       "returndatacopy_initial" : {
          "env" : { ... }
		  "expect" : [ ... ]
		  "pre" " { ... }
		  "transaction" : { ... }
       }
   }

where ``...`` indicates omissions.

.. [#] The file name and the name written in JSON should match because ``testeth`` prints the name written in JSON, but the user needs to find a file.

``env`` field contains some parameters in a straightforward way.

``pre`` field describes the pre-state account-wise:

.. code::

     "pre" : {
        "0x0f572e5295c57f15886f9b263e2f6d2d6c7b5ec6" : {
            "balance" : "0x0de0b6b3a7640000",
            "code" : "{ (MSTORE 0 0x112233445566778899aabbccddeeff) (RETURNDATACOPY 0 0 32) (SSTORE 0 (MLOAD 0)) }",
            "code" : "0x306000526020600060003e600051600055",
            "nonce" : "0x00",
            "storage" : {
                "0x00" : "0x01"
            }
        }
     }


As specified in the Yellow Paper, an account contains a balance, a code, a nonce and a storage.

Notice the ``code`` field is duplicated.  If many fields exist under the same name, the last one is used.
In this particular case, the LLL compiler was not ready to parse the new instruction ``RETURNDATACOPY`` so a compiled runtime bytecode is added as the second ``code`` field [#]_.

.. [#] Unless you are testing malformed bytecode, always try to keep the LLL code in the test filler.  LLL code is easier to understand and to modify.

This particular test expected to see ``0`` in the first slot in the storage.  In order to make this change visible, the pre-state has ``1`` there.

Usually, there is another account that acts as the initial caller of the transaction.

``transaction`` field is somehow interesting because it can describe a multidimensional array of test cases.  Notice that ``data``, ``gasLimit`` and ``value`` fields are lists.

.. code::

   "transaction" : {
		"data" : [
			"", "0xaaaa", "0xbbbb"
		],
		"gasLimit" : [
			"0x0a00000000",
			"0x0"
		],
		"gasPrice" : "0x01",
		"nonce" : "0x00",
		"secretKey" : "0x45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8",
		"to" : "0x0f572e5295c57f15886f9b263e2f6d2d6c7b5ec6",
		"value" : [
			"0x00"
		]
	}

Since ``data`` has three elemenets and ``gasLimit`` has two elements, the above ``transaction`` field specifies six different transactions.  Later, in the ``expect`` section, ``data : 1`` would mean the ``0xaaaa`` as data, and ``gasLimit : 0`` would mean ``0x0a00000000`` as gas limit.

Moreover, these transactions are tested under different versions of the protocol.

``expect`` field of the filler specifies the expected fields of the state after the transaction.  The ``expect`` field does not need to specify a state completely, but it should specify some features of some accounts.  ``expect`` field is a list.  Each element talks about some elements of the multi-dimensional array defined in ``transaction`` field.

.. code::

   "expect" : [
		{
			"indexes" : {
				"data" : 0,
				"gas" : -1,
				"value" : -1
			},
			"network" : ["Frontier", "Homestead"],
			"result" : {
				"095e7baea6a6c7c4c2dfeb977efac326af552d87" : {
					"balance" : "2000000000000000010",
					"storage" : {
						"0x" : "0x01",
						"0x01" : "0x01"
					}
				},
				"2adc25665018aa1fe0e6bc666dac8fc2697ff9ba" : {
					"balance" : "20663"
				},
				"a94f5374fce5edbc8e2a8697c15331677e6ebf0b" : {
					"balance" : "99979327",
					"nonce" : "1"
				}
			}
		},
		{
		    "indexes" : {
			    "data" : 1,
			    "gas" : -1,
			    "value" : -1
		    },
		...
		}
	]

``indexes`` field specifies a subset of the transactions.  ``-1`` means "whichever".  ``"data" : 0`` points to the first element in the ``data`` field in ``transaction``.

``network`` field is somehow similar.  It specifies the versions of the protocol for which the expectation applies.  For expectations common to all versions, say ``"network" : ALL``.

Filling the Test
----------------

The test filler file is not for consumption.  The filler file needs to be filled into a test.  ``testeth`` has the ability to compute the post-state from the test filler, and produce the test.  The advantage of the filled test is that it can catch any post-state difference between clients.

.. _editcpp:

First, if you created a new subdirectory for the filler, you need to edit the source of ``cpp-ethereum`` so that ``testeth`` recognizes the new subdirectory.  The file to edit is ``cpp-ethereum/blob/develop/test/tools/jsontests/StateTests.cpp``, which lists the names of the subdirectories scanned for GeneralStateTest filters.

After building ``testeth``, you are ready to fill the test.

.. code:: bash

   ETHEREUM_TEST_PATH="../../tests" test/testeth -t StateTestsGeneral/stReturnDataTest -- --filltests --checkstate

where the environmental variable ``ETHEREUM_TEST_PATH`` should point to the directory where ``tests`` repository is checked out.  ``stReturnDataTest`` should be replaced with the name of the subdirectory you are working on.  ``--filltests`` option tells ``testeth`` to fill tests.  ``--checkstate`` tells ``testeth`` to look at ``expect`` fields.

Depending on your shell, there are various way to set up ``ETHEREUM_TEST_PATH`` environment variable.  For example, adding ``export ETHEREUM_TEST_PATH=/path/to/tests`` to ``~/.bashrc`` might work for ``bash`` users.

``testeth`` with ``--filltests`` fills every test filler it finds. The command might modify existing test cases. After running ``testeth`` with ``--filltests``, try running ``git status`` in the ``tests`` directory. If ``git status`` indicates changes in unexpected files, that is an indication that the behavior of ``cpp-ethereum`` changed unexpectedly.

git commit
----------

After these succeed, the filler file and the filled test should be added to the ``tests`` repository, and filed as a pull-request.

If changes in the cpp-client was necessary, also file a pull-request there.


Converting a GeneralStateTest Case into a BlockchainTest Case
=============================================================

.. code::

  ETHEREUM_TEST_PATH="../../tests" test/testeth -t StateTestsGeneral/stReturnDataTest -- --filltests --fillchain --checkstate

followed by

.. code::

  ETHEREUM_TEST_PATH="../../tests" test/testeth -t StateTestsGeneral/stReturnDataTest -- --filltests --checkstate

The second command is necessary because the first command modifies the GeneralStateTests in an undesired way.

Generating a BlockchainTest Case
================================

(To be described.)
