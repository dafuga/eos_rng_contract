const { Numeric } = require('eosjs');
const { randomBytes } = require('crypto');

module.exports = function random64() {
  console.log({bytes: randomBytes(8)})
  console.log({randomNumber: Numeric.binaryToDecimal(randomBytes(8))})

  return Numeric.binaryToDecimal(randomBytes(8))
};
