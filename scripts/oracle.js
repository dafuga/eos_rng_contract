const commitNumber = require('./wrappers/commitNumber');
const wait = require('./utils/wait');
const generateHash = require('./utils/generateHash');

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
  console.log('ticking!')
  console.log({timeBlockAwaited});
  console.log({randomNumberToReveal});
  console.log({randomNumberHash});
  console.log({secondsInterval});

  if (timeBlockAwaited) {
    await updateCommittedNumber();
  } else {
    await commitNewNumber();
  }
}

async function commitNewNumber() {
  console.log('commitNumber');

  // Generate a new random number and store it and it's hash in memory
  randomNumberToReveal = Math.floor(Math.random() * 999);
  randomNumberHash = generateHash(randomNumberToReveal);
  timeBlockAwaited= getCurrentTimeBlock() + (secondsInterval * 2 - 1);

  await commitNumber(randomNumberHash, timeBlockAwaited, null);

  await tick();
}

async function updateCommittedNumber() {
  const currentTimeBlock = getCurrentTimeBlock();

  if (currentTimeBlock && timeBlockAwaited !== currentTimeBlock) {
    await wait(100);

    return updateCommittedNumber();
  }

  console.log('updateCommitNumber');
  await commitNumber(randomNumberHash, currentTimeBlock, randomNumberToReveal);

  timeBlockAwaited = null;
  randomNumberToReveal = null;
  randomNumberHash = null;

  await tick();
}

function getCurrentTimeBlock() {
  const millisecondsSinceEpoch = (new Date).getTime();

  return Math.floor(millisecondsSinceEpoch / 500);
}
