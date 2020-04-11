const { JsonRpc } = require('eosjs');
const fetch = require('node-fetch');           // node only; not needed in browsers

module.exports = async function getTable(table) {
  const rpc = new JsonRpc(process.env.NODE_URL, { fetch });

  const response = await rpc.get_table_rows({
    json: true,                             // Get the response as json
    code: process.env.CONTRACT_ACCOUNT_NAME,     // Contract that we target
    scope: process.env.CONTRACT_ACCOUNT_NAME,    // Account that owns the data
    table,                                  // Table name
    limit: 10,                              // Maximum number of rows that we want to get
  });

  return response.rows;
};

