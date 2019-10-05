
(async () => {
  const secondsInterval = process.argv.seconds_interval;

  console.log(`Starting Oracle with "${secondsInterval}" seconds interval.`);

  require('dotenv').config();



})();

function wait(numberOfMilliseconds) {
  return new Promise(resolve => {
    setInterval(() => {
      resolve();
    }, numberOfMilliseconds)
  })
}
