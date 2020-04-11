const eosjsAccountName = require("eosjs-account-name")

const commitNumber = require('./wrappers/commitnumber');
const revealNumber = require('./wrappers/revealnumber');
const regOracle = require('./wrappers/regoracle');

const generateHash = require('./utils/generateHash');
const wait = require('./utils/wait');
const random64 = require('./utils/random64');

let timeBlockAwaited;
let lastTimeBlock;
let randomNumberToReveal;
let randomNumberHash;
let secondsInterval;

(async () => {
  secondsInterval = Number(process.argv[2]) || 10;

  console.log(`Starting Oracle with "${secondsInterval}" seconds interval.`);

  require('dotenv').config();

  // await registerOracle();

  await waitForBeginningOfNextIntervalBlock();

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
  randomNumberToReveal = random64();

  const oracleNameBase64Int = eosjsAccountName.nameToUint64(process.env.ORACLE_ACCOUNT_NAME);

  randomNumberHash = generateHash(oracleNameBase64Int +randomNumberToReveal.toString());
  timeBlockAwaited = (lastTimeBlock || getCurrentTimeBlock()) + secondsInterval;

  lastTimeBlock = null;

  console.log(`Committing "${randomNumberToReveal}" to be revealed on "${timeBlockAwaited}" time block.`);

  const commitSucceeded = await commitNumber(randomNumberHash, timeBlockAwaited);

  if (!commitSucceeded) {
    timeBlockAwaited = null;
    randomNumberToReveal = null;
    randomNumberHash = null;

    await registerOracle();

    await waitForBeginningOfNextIntervalBlock();
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

    lastTimeBlock = timeBlockAwaited;

    // wait 500 ms for next block
    await wait(500);
  }

  timeBlockAwaited = null;
  randomNumberToReveal = null;
  randomNumberHash = null;

  await tick();
}

async function waitForBeginningOfNextIntervalBlock() {
  console.log('Waiting for beginning of next interval block.');
  const currentTimeBlock = getCurrentTimeBlock();

  // Looking for reminder of time until next interval starts
  const secondsInInterval = (currentTimeBlock) % secondsInterval;

  const nextTimeBlock = secondsInterval - secondsInInterval + currentTimeBlock - 1;

  console.log({nextTimeBlock});

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


