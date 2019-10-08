const { Api, JsonRpc } = require('eosjs');
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig');
const fetch = require('node-fetch');
const { TextEncoder, TextDecoder } = require('text-encoding');

module.exports = async function commitNumber(randomNumberHash, timeBlock, randomNumber) {
  const signatureProvider = new JsSignatureProvider([process.env.ORACLE_PRIVATE_KEY]);
  const rpc = new JsonRpc(process.env.ORACLE_API_URL, {fetch});
  const api = new Api({rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder()});

  console.log({account: process.env.ORACLE_ACCOUNT_NAME})
  console.log({permission: process.env.ORACLE_ACCOUNT_PERMISSION})
  console.log({timeBlock})
  console.log({randomNumber})
  console.log({randomNumberHash})
  const actions = {
    actions: [{
      account: 'rng',
      name: 'commitnumber',
      authorization: [{
        actor: process.env.ORACLE_ACCOUNT_NAME,
        permission: process.env.ORACLE_ACCOUNT_PERMISSION,
      }],
      data: {
        user: process.env.ORACLE_ACCOUNT_NAME,
        hash: randomNumberHash,
        reveal_time_block: timeBlock,
        revealed_number: randomNumber || 0,
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
