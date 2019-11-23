const commitNumber = require('./wrappers/commitnumber');
const revealNumber = require('./wrappers/revealnumber');
const regOracle = require('./wrappers/regoracle');

const generateHash = require('./utils/generateHash');
const wait = require('./utils/wait');

let timeBlockAwaited;
let randomNumberToReveal;
let randomNumberHash;
let secondsInterval;

(async () => {
  secondsInterval = Number(process.argv[2]) || 2;

  console.log(`Starting Oracle with "${secondsInterval}" seconds interval.`);

  require('dotenv').config();

  await waitForBeginningOfNextIntervalBlock();

  await registerOracle();

  await tick()
})();

async function tick() {
  if (timeBlockAwaited) {
    await revealCommittedNumber();
  } else {
    await commitNewNumber();
  }
}

async function commitNewNumber() {
  // Generate a new random number and store it and it's hash in memory
  randomNumberToReveal = Math.floor(Math.random() * 998 + 1);
  randomNumberHash = generateHash(randomNumberToReveal);
  timeBlockAwaited = getCurrentTimeBlock() + secondsInterval;

  console.log(`Committing "${randomNumberToReveal}" to be revealed on "${timeBlockAwaited}" time block.`);

  const commitSucceeded = await commitNumber(randomNumberHash, timeBlockAwaited);

  if (!commitSucceeded) {
    timeBlockAwaited = null;
    randomNumberToReveal = null;
    randomNumberHash = null;

    await registerOracle();
  }

  await tick();
}

async function revealCommittedNumber() {
  if (timeBlockAwaited && timeBlockAwaited !== getCurrentTimeBlock()) {
    await wait(50);

    return await revealCommittedNumber();
  }

  console.log(`Starting reveal at block ${getCurrentTimeBlock()}.`);

  const revealSucceeded = await revealNumber(getCurrentTimeBlock(), randomNumberToReveal);

  if (revealSucceeded) {
    console.log(`Revealed "${randomNumberToReveal}" number at "${getCurrentTimeBlock()}" time block.`);
  } else {
    await wait(10000);
  }


  timeBlockAwaited = null;
  randomNumberToReveal = null;
  randomNumberHash = null;

  // wait 500 ms for next block
  await wait(500);

  await tick();
}

async function waitForBeginningOfNextIntervalBlock() {
  console.log('Waiting for beginning of next interval block.');
  const currentTimeBlock = getCurrentTimeBlock();

  // Looking for reminder of time until next interval starts
  const secondsInInterval = (currentTimeBlock) % secondsInterval;

  const nextTimeBlock = secondsInterval - secondsInInterval + currentTimeBlock - 1 ;

  while (nextTimeBlock !== getCurrentTimeBlock()) {
    await wait(50);
  }
}

function getCurrentTimeBlock() {
  const millisecondsSinceEpoch = (new Date).getTime();

  return Math.floor(millisecondsSinceEpoch / 1000);
}

async function registerOracle() {
  console.log(`Registering Oracle.`);
  await regOracle();
  console.log(`Waiting "${process.env.ORACLE_REGISTRATION_DELAY}" blocks to become a valid oracle.`);
  await wait(process.env.ORACLE_REGISTRATION_DELAY * 1100);
}
