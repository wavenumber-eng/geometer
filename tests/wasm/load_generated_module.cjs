const fs = require("fs");
const path = require("path");
const vm = require("vm");
const { createRequire } = require("module");

module.exports = function loadGeneratedModule(modulePath) {
  const generatedModule = { exports: {} };
  const wrapper = vm.runInThisContext(
    `(function(module, exports, require, __dirname, __filename) {${fs.readFileSync(modulePath, "utf8")}\n})`,
    { filename: modulePath },
  );
  wrapper(
    generatedModule,
    generatedModule.exports,
    createRequire(modulePath),
    path.dirname(modulePath),
    modulePath,
  );
  return generatedModule.exports;
};
