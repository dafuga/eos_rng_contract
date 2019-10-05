const { Api, JsonRpc } = require('eosjs');
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig');
const fetch = require('node-fetch');
const { TextEncoder, TextDecoder } = require('text-encoding');

module.exports = async function commitNumber(hash, timeBlock, randomNumber) {
  const signatureProvider = new JsSignatureProvider([process.env.ORACLE_PRIVATE_KEY]);
  const rpc = new JsonRpc(process.env.ORACLE_API_URL, {fetch});
  const api = new Api({rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder()});

  const actions = {
    actions: [{
      account: 'rng',
      name: 'commitnumber',
      authorization: [{
        actor: process.env.ORACLE_ACCOUNT_NAME,
        permission: process.env.ORACLE_PERMISSION,
      }],
      data: {
        hash,
        time_block: timeBlock,
        random_number: randomNumber,
      },
    }]
  };

  try {
    return await api.transact(actions, {
      blocksBehind: 3,
      expireSeconds: 30,
    });
  } catch(error) {
    console.log(`\nCaught exception: ${error}`);
  }
};
