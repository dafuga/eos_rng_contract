const commitNumber = require('./wrappers/commitNumber');
const revealNumber = require('./wrappers/revealNumber');

const generateHash = require('./utils/generateHash');
const wait = require('./utils/wait');

let currentTimeBlock;
let timeBlockAwaited;
let randomNumberToReveal;
let randomNumberHash;
let secondsInterval;

(async () => {
  secondsInterval = Number(process.argv[2]) || 1;

  console.log(`Starting Oracle with "${secondsInterval}" seconds interval.`);

  require('dotenv').config();

  await tick()
})();

async function tick() {
  currentTimeBlock =  getCurrentTimeBlock();

  if (timeBlockAwaited) {
    await revealCommittedNumber();
  } else {
    await commitNewNumber();
  }
}

async function commitNewNumber() {
  // Generate a new random number and store it and it's hash in memory
  randomNumberToReveal = Math.floor(Math.random() * 999);
  randomNumberHash = generateHash(randomNumberToReveal);
  timeBlockAwaited = getCurrentTimeBlock() + secondsInterval;

  console.log(`Committing "${randomNumberToReveal}" to be revealed on "${currentTimeBlock}" time block.`);

  await commitNumber(randomNumberHash, timeBlockAwaited);

  await tick();
}

async function revealCommittedNumber() {
  console.log({currentTimeBlock})
  console.log({timeBlockAwaited})
  if (timeBlockAwaited && timeBlockAwaited !== currentTimeBlock) {
    await wait(100);

    return tick();
  }

  await revealNumber(currentTimeBlock, randomNumberToReveal);

  console.log(`Revealed "${randomNumberToReveal}" number at "${currentTimeBlock}" time block.`);

  timeBlockAwaited = null;
  randomNumberToReveal = null;
  randomNumberHash = null;

  // wait 500 ms for next block
  await wait(500);

  await tick();
}

function getCurrentTimeBlock() {
  const millisecondsSinceEpoch = (new Date).getTime();

  return Math.floor(millisecondsSinceEpoch / 1000);
}
