const commitNumber = require('./wrapppers/commitNumber');
const wait = require('./utils/wait');
const generateHash = require('./utils/generateHash');

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

  await commitNumber(randomNumberHash, revealTimeBlock, null)

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

async function currentTimeBlock() {
  const millisecondsSinceEpoch = (new Date).getTime();

  return Math.floor(millisecondsSinceEpoch / 500);
}
