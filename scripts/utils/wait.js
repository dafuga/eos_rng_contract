module.exports = async function wait(numberOfMilliseconds) {
  return new Promise(resolve => {
    setInterval(() => {
      resolve();
    }, numberOfMilliseconds)
  })
};
