const commitNumber = require('./wrappers/commitNumber');
const wait = require('./utils/wait');
const generateHash = require('./utils/generateHash');

let currentTimeBlock;
let timeBlockAwaited;
let randomNumberToReveal;
let randomNumberHash;
let secondsInterval;

(async () => {
  secondsInterval = Number(process.argv[2]) || 1;
  console.log({secondsInterval})

  console.log(`Starting Oracle with "${secondsInterval}" seconds interval.`);

  require('dotenv').config();

  await tick()
})();

async function tick() {
  currentTimeBlock =  getCurrentTimeBlock();

  if (timeBlockAwaited) {
    await updateCommittedNumber();
  } else {
    await commitNewNumber();
  }
}

async function commitNewNumber() {
  // Generate a new random number and store it and it's hash in memory
  randomNumberToReveal = Math.floor(Math.random() * 999);
  randomNumberHash = generateHash(randomNumberToReveal);
  timeBlockAwaited = getCurrentTimeBlock() + secondsInterval;

  await commitNumber(randomNumberHash, timeBlockAwaited, null);

  await tick();
}

async function updateCommittedNumber() {
  if (timeBlockAwaited && timeBlockAwaited !== currentTimeBlock) {
    await wait(100);

    return tick();
  }

  await commitNumber(randomNumberHash, currentTimeBlock, randomNumberToReveal);

  timeBlockAwaited = null;
  randomNumberToReveal = null;
  randomNumberHash = null;

  await tick();
}

function getCurrentTimeBlock() {
  const millisecondsSinceEpoch = (new Date).getTime();

  return Math.floor(millisecondsSinceEpoch / 1000);
}
