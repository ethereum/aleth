# The Whisper Specific Commands

Whisper is a hybrid DHT/point-to-point communications system. It allows for transient publication and subscription of messages using a novel topic-based routing system. It gives exceptional levels of privacy, and can be configured to provide a privacy/efficiency tradeoff per application.

Here follows a few gotchas in the present whisper API on the `eth` interactive console. It will be updated shortly.

Points to note:
- All API for whisper is contained in the `web3.shh` object. Type it in the interactive console to get a list of all functions and variables provided.
- Topic values are user-readable strings and as such should be given as plain text. E.g., topic `"zxcv"` in the console corresponds directly to the topic `"zxcv"`.
- Payload should be in JSONRPC standard data representation (i.e. hexadecimal encoding). E.g., text `"zxcv"` must be represented as `"0x847a786376"`. This can be done through usage of the `web3.fromAscii` function.
- The function `web3.shh.request()` returns the request object instead of sending it to the core (except for some complicated functions). E.g. `web3.shh.post.request({topcis: ["0xC0FFEE"]})`
- When sending/posting a message, don't omit the `ttl` parameter, because default value is zero, and therefore the message will immediately expire, before being sent.
- The functions `filter.get()` and `web3.shh.getMessages()` should return the same messages, but in a different format.

### Examples of usage

```javascript
var f = web3.shh.filter({topics: ["qwerty"]})
f.get()
web3.shh.getMessages("qwerty")
web3.shh.post({topics: ["qwerty"], payload: "0x847a786376", ttl: "0x1E", workToProve: "0x9" })
```