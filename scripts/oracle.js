const { Api, JsonRpc } = require('eosjs');
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig');
const fetch = require('node-fetch');
const { TextEncoder, TextDecoder } = require('text-encoding');

let timeBlockAwaited;
let randomNumberToReveal;
let randomNumberHash;
let secondsInterval;

(async () => {
  secondsInterval = process.argv.seconds_interval;

  console.log(`Starting Oracle with "${secondsInterval}" seconds interval.`);

  require('dotenv').config();

  await tick()
})();

async function tick() {
  if (timeBlockAwaited) {
    await commitNewNumber();
  } else {
    await updateCommittedNumber();
  }
}

async function commitNewNumber() {
  // Generate a new random number and store it and it's hash in memory
  randomNumberToReveal = Math.floor(Math.random() * 999);
  randomNumberHash = generateHash(randomNumberToReveal);

  const currentTimeBlock = currentTimeBlock();
  const revealTimeBlock = currentTimeBlock + (secondsInterval * 2 - 1);

  await sendCommitTransaction(randomNumberHash, revealTimeBlock, null)

  await tick();
}

async function updateCommittedNumber() {
  const currentTimeBlock = currentTimeBlock();

  if (currentTimeBlock && timeBlockAwaited !== currentTimeBlock) {
    await wait(100);

    return updateCommittedNumber();
  }

  await sendCommitTransaction(randomNumberHash, currentTimeBlock, randomNumberToReveal);

  timeBlockAwaited = null;
  randomNumberToReveal = null;
  randomNumberHash = null;

  await tick();
}

async function sendCommitTransaction(hash, timeBlock, randomNumber) {
  const signatureProvider = new JsSignatureProvider([process.env.ORACLE_PRIVATE_KEY]);
  const rpc = new JsonRpc(process.env.ORACLE_API_URL, { fetch });
  const api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });

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
    await wait(50);

    return await sendCommitTransaction(hash, timeBlock, randomNumber)
  }
}

async function currentTimeBlock() {
  const millisecondsSinceEpoch = (new Date).getTime();

  return Math.floor(millisecondsSinceEpoch / 500);
}

async function wait(numberOfMilliseconds) {
  return new Promise(resolve => {
    setInterval(() => {
      resolve();
    }, numberOfMilliseconds)
  })
}

async function generateHash(message) {
  // encode as UTF-8
  const msgBuffer = new TextEncoder('utf-8').encode(message);
  // hash the message
  const hashBuffer = await crypto.subtle.digest('SHA-256', msgBuffer);
  // convert ArrayBuffer to Array
  const hashArray = Array.from(new Uint8Array(hashBuffer));
  // convert bytes to hex string
  return hashArray.map(b => ('00' + b.toString(16)).slice(-2)).join('');
}
