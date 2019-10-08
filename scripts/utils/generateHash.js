const crypto = require('crypto');

module.exports = function generateHash(data) {
  const hash = crypto.createHash('sha256');

  hash.update(data.toString());

  return hash.digest('hex');
};
