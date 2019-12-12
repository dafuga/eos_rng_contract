const { Numeric } = require('eosjs');
const { randomBytes } = require('crypto');

module.exports = function random64() {
  return Numeric.binaryToDecimal(randomBytes(8))
};
