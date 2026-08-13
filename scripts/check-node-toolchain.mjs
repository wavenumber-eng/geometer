const EXPECTED_NODE_MAJOR = 24;
const EXPECTED_NPM = "11.16.0";

const nodeMajor = Number.parseInt(process.versions.node.split(".", 1)[0], 10);
if (nodeMajor !== EXPECTED_NODE_MAJOR) {
  throw new Error(`Node ${EXPECTED_NODE_MAJOR} is required; found ${process.versions.node}.`);
}

const userAgent = process.env.npm_config_user_agent ?? "";
const npmMatch = /^npm\/([^ ]+)/u.exec(userAgent);
if (!npmMatch) {
  throw new Error("Run contract commands through npm so the npm version can be verified.");
}
if (npmMatch[1] !== EXPECTED_NPM) {
  throw new Error(`npm ${EXPECTED_NPM} is required; found ${npmMatch[1]}.`);
}

process.stdout.write(`Node ${process.versions.node}; npm ${npmMatch[1]}\n`);
