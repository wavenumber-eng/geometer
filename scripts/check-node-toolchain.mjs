import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";

const packageJson = JSON.parse(
  readFileSync(fileURLToPath(new URL("../package.json", import.meta.url)), "utf8"),
);
const expectedNode = packageJson.engines.node;
const expectedNpm = packageJson.engines.npm;
if (process.versions.node !== expectedNode) {
  throw new Error(`Node ${expectedNode} is required; found ${process.versions.node}.`);
}

const userAgent = process.env.npm_config_user_agent ?? "";
const npmMatch = /^npm\/([^ ]+)/u.exec(userAgent);
if (!npmMatch) {
  throw new Error("Run contract commands through npm so the npm version can be verified.");
}
if (npmMatch[1] !== expectedNpm || packageJson.packageManager !== `npm@${expectedNpm}`) {
  throw new Error(`npm ${expectedNpm} is required; found ${npmMatch[1]}.`);
}

process.stdout.write(`Node ${process.versions.node}; npm ${npmMatch[1]}\n`);
