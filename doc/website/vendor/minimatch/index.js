'use strict';

// minimatch >= 9 exports { minimatch }; serve-handler calls the module itself.
module.exports = require('minimatch-upstream').minimatch;
