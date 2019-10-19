const { Api, JsonRpc } = require('eosjs');
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig');
const fetch = require('node-fetch');
const { TextEncoder, TextDecoder } = require('text-encoding');

module.exports = async function regoracle() {
  const signatureProvider = new JsSignatureProvider([process.env.ORACLE_PRIVATE_KEY]);
  const rpc = new JsonRpc(process.env.NODE_URL, {fetch});
  const api = new Api({rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder()});

  const actions = {
    actions: [{
      account: process.env.CONTRACT_ACCOUNT,
      name: 'regoracle',
      authorization: [{
        actor: process.env.ORACLE_ACCOUNT_NAME,
        permission: process.env.ORACLE_ACCOUNT_PERMISSION,
      }],
      data: {
        user: process.env.ORACLE_ACCOUNT_NAME,
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
    return false;
  }
};
