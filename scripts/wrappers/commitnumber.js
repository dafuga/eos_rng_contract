const { Api, JsonRpc } = require('eosjs');
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig');
const fetch = require('node-fetch');
const { TextEncoder, TextDecoder } = require('text-encoding');

module.exports = async function commitNumber(randomNumberHash, revealTimeBlock) {
  const signatureProvider = new JsSignatureProvider([process.env.ORACLE_PRIVATE_KEY]);
  const rpc = new JsonRpc(process.env.NODE_URL, {fetch});
  const api = new Api({rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder()});

  const actions = {
    actions: [{
      account: process.env.CONTRACT_ACCOUNT_NAME,
      name: 'commitnumber',
      authorization: [{
        actor: process.env.ORACLE_ACCOUNT_NAME,
        permission: process.env.ORACLE_ACCOUNT_PERMISSION,
      }],
      data: {
        user: process.env.ORACLE_ACCOUNT_NAME,
        hash: randomNumberHash,
        reveal_time_block: revealTimeBlock
      },
    }]
  };

  try {
    const response = await api.transact(actions, {
      blocksBehind: 3,
      expireSeconds: 30,
    });

    return response;
  } catch(error) {
    console.log(`\nCaught exception: ${error}`);
    return false;
  }
};
